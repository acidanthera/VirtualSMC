/*
 *  SMIMonitor.cpp
 *  SMCDellSensors
 *
 *  Copyright 2014 Slice. All rights reserved.
 *  Adapted for VirtualSMC by lvs1974, 2020
 *  Original sources: https://github.com/CloverHackyColor/FakeSMC3_with_plugins
 *
 */

#include "SMIMonitor.hpp"

SMIMonitor *SMIMonitor::instance = nullptr;

OSDefineMetaClassAndStructors(SMIMonitor, OSObject)

static int smm(SMMRegisters *regs) {
	int rc;
	int eax = regs->eax;  //input value

#if __LP64__
	asm volatile("pushq %%rax\n\t"
			"movl 0(%%rax),%%edx\n\t"
			"pushq %%rdx\n\t"
			"movl 4(%%rax),%%ebx\n\t"
			"movl 8(%%rax),%%ecx\n\t"
			"movl 12(%%rax),%%edx\n\t"
			"movl 16(%%rax),%%esi\n\t"
			"movl 20(%%rax),%%edi\n\t"
			"popq %%rax\n\t"
			"out %%al,$0xb2\n\t"
			"out %%al,$0x84\n\t"
			"xchgq %%rax,(%%rsp)\n\t"
			"movl %%ebx,4(%%rax)\n\t"
			"movl %%ecx,8(%%rax)\n\t"
			"movl %%edx,12(%%rax)\n\t"
			"movl %%esi,16(%%rax)\n\t"
			"movl %%edi,20(%%rax)\n\t"
			"popq %%rdx\n\t"
			"movl %%edx,0(%%rax)\n\t"
			"pushfq\n\t"
			"popq %%rax\n\t"
			"andl $1,%%eax\n"
			: "=a"(rc)
			: "a"(regs)
			: "%ebx", "%ecx", "%edx", "%esi", "%edi", "memory");
#else
	asm volatile("pushl %%eax\n\t"
			"movl 0(%%eax),%%edx\n\t"
			"push %%edx\n\t"
			"movl 4(%%eax),%%ebx\n\t"
			"movl 8(%%eax),%%ecx\n\t"
			"movl 12(%%eax),%%edx\n\t"
			"movl 16(%%eax),%%esi\n\t"
			"movl 20(%%eax),%%edi\n\t"
			"popl %%eax\n\t"
			"out %%al,$0xb2\n\t"
			"out %%al,$0x84\n\t"
			"xchgl %%eax,(%%esp)\n\t"
			"movl %%ebx,4(%%eax)\n\t"
			"movl %%ecx,8(%%eax)\n\t"
			"movl %%edx,12(%%eax)\n\t"
			"movl %%esi,16(%%eax)\n\t"
			"movl %%edi,20(%%eax)\n\t"
			"popl %%edx\n\t"
			"movl %%edx,0(%%eax)\n\t"
			"lahf\n\t"
			"shrl $8,%%eax\n\t"
			"andl $1,%%eax\n"
			: "=a"(rc)
			: "a"(regs)
			: "%ebx", "%ecx", "%edx", "%esi", "%edi", "memory");
#endif

	if ((rc != 0) || ((regs->eax & 0xffff) == 0xffff) || (regs->eax == eax)) {
		return -1;
	}

	return 0;
}

int SMIMonitor::i8k_smm(SMMRegisters *regs) {
	static int gRc;
	gRc = -1;
	mp_rendezvous_no_intrs([](void *arg) {
		if (cpu_number() == 0) { /* SMM requires CPU 0 */
			gRc = smm(static_cast<SMMRegisters *>(arg));
		}
	}, regs);
	return gRc;
}

/*
 * Read the CPU temperature in Celcius.
 */
int SMIMonitor::i8k_get_temp(int sensor) {
	SMMRegisters regs {};
	int rc;
	int temp;

	regs.eax = I8K_SMM_GET_TEMP;
	regs.ebx = sensor & 0xFF;
	if ((rc=i8k_smm(&regs)) < 0) {
		return rc;
	}

	temp = regs.eax & 0xff;
	if (temp == 0x99) {
		IOSleep(100);
		regs.eax = I8K_SMM_GET_TEMP;
		regs.ebx = sensor & 0xFF;
		if ((rc=i8k_smm(&regs)) < 0) {
			return rc;
		}
		temp = regs.eax & 0xff;
	}
	return temp;
}

int SMIMonitor::i8k_get_temp_type(int sensor) {
	SMMRegisters regs {};
	int rc;
	int type;

	regs.eax = I8K_SMM_GET_TEMP_TYPE;
	regs.ebx = sensor & 0xFF;
	if ((rc=i8k_smm(&regs)) < 0) {
		return rc;
	}

	type = regs.eax & 0xff;
	return type;
}

bool SMIMonitor::i8k_get_dell_sig_aux(int fn) {
	SMMRegisters regs {};

	regs.eax = fn;
	if (i8k_smm(&regs) < 0) {
		DBGLOG("sdell", "No function 0x%x", fn);
		return false;
	}
	DBGLOG("sdell", "Got sigs %x and %x", regs.eax, regs.edx);
	return ((regs.eax == 0x44494147 /*DIAG*/) &&
			(regs.edx == 0x44454C4C /*DELL*/));
}

bool SMIMonitor::i8k_get_dell_signature(void) {
	return (i8k_get_dell_sig_aux(I8K_SMM_GET_DELL_SIG1) ||
		    i8k_get_dell_sig_aux(I8K_SMM_GET_DELL_SIG2));
}

/*
 * Read the fan speed in RPM.
 */
int SMIMonitor::i8k_get_fan_speed(int fan) {
	SMMRegisters regs {};
	int rc;
	int speed = 0;

	regs.eax = I8K_SMM_GET_SPEED;
	regs.ebx = fan & 0xff;
	if ((rc=i8k_smm(&regs)) < 0) {
		return rc;
	}
	speed = (regs.eax & 0xffff) * fanMult;
	return speed;
}

/*
 * Read the fan status.
 */
int SMIMonitor::i8k_get_fan_status(int fan) {
	SMMRegisters regs {};
	int rc;

	regs.eax = I8K_SMM_GET_FAN;
	regs.ebx = fan & 0xff;
	if ((rc=i8k_smm(&regs)) < 0) {
		return rc;
	}

	return (regs.eax & 0xff);
}

/*
 * Read the fan status.
 */
int SMIMonitor::i8k_get_fan_type(int fan) {
	SMMRegisters regs {};
	int rc;

	regs.eax = I8K_SMM_GET_FAN_TYPE;
	regs.ebx = fan & 0xff;
	if ((rc=i8k_smm(&regs)) < 0) {
		return rc;
	}

	return (regs.eax & 0xff);
}


/*
 * Read the fan nominal rpm for specific fan speed (0,1,2) or zero
 */
int SMIMonitor::i8k_get_fan_nominal_speed(int fan, int speed) {
	SMMRegisters regs {};
	regs.eax = I8K_SMM_GET_NOM_SPEED;
	regs.ebx = (fan & 0xff) | (speed << 8);
	return i8k_smm(&regs) ? 0 : (regs.eax & 0xffff) * fanMult;
}

/*
 * Set the fan speed (off, low, high). Returns the new fan status.
 */
int SMIMonitor::i8k_set_fan(int fan, int speed) {
	SMMRegisters regs {};
	regs.eax = I8K_SMM_SET_FAN;

	speed = (speed < 0) ? 0 : ((speed > I8K_FAN_MAX) ? I8K_FAN_MAX : speed);
	regs.ebx = (fan & 0xff) | (speed << 8);

	return i8k_smm(&regs);
}

int SMIMonitor::i8k_set_fan_control_manual(int fan) {
	SMMRegisters regs {};
	regs.eax = I8K_SMM_IO_DISABLE_FAN_CTL1;
	regs.ebx = (fan & 0xff);
	return i8k_smm(&regs);
}

int SMIMonitor::i8k_set_fan_control_auto(int fan) {
	SMMRegisters regs {};
	regs.eax = I8K_SMM_IO_ENABLE_FAN_CTL1;
	regs.ebx = (fan & 0xff);
	return i8k_smm(&regs);
}

void SMIMonitor::createShared() {
	if (instance)
		PANIC("sdell", "attempted to allocate smi monitor again");
	instance = new SMIMonitor;
	if (!instance)
		PANIC("sdell", "failed to allocate smi monitor");
	instance->mainLock = IOLockAlloc();
	if (!instance->mainLock)
		PANIC("sdell", "failed to allocate smi monitor main lock");
}

bool SMIMonitor::probe() {
	IOLockLock(mainLock);
	if (!i8k_get_dell_signature()) {
		SYSLOG("sdell", "Unable to get Dell SMM signature!");
		IOLockUnlock(mainLock);
		return false;
	}
	
	bool success = true;
	
	if (!workloop) {
		DBGLOG("sdell", "probing smi monitor");

		workloop = IOWorkLoop::workLoop();
		timerEventSource = IOTimerEventSource::timerEventSource(this, [](OSObject *object, IOTimerEventSource *sender) {
			auto bm = OSDynamicCast(SMIMonitor, object);
			if (bm) {
				IOLockLock(bm->mainLock);
				bm->updateSensors();
				IOLockUnlock(bm->mainLock);
			}
		});
		if (!timerEventSource || !workloop) {
			SYSLOG("sdell", "failed to create workloop or timer event source");
			success = false;
		}

		if (success && workloop->addEventSource(timerEventSource) != kIOReturnSuccess) {
			SYSLOG("sdell", "failed to add timer event source");
			success = false;
		}
		
		// timerEventSource must exist before adding ACPI notification handler
		if (success && (!findFanSensors() || !findTempSensors())) {
			SYSLOG("sdell", "failed to find fans or temp sensors!");
			success = false;
		}
		
		DBGLOG("sdell", "found %u fan sensors and %u temp sensors", fanCount, tempCount);

		if (!success) {
			OSSafeReleaseNULL(workloop);
			OSSafeReleaseNULL(timerEventSource);
		}
	}
	
	DBGLOG("sdell", "Based on I8kfan project and adopted to VirtualSMC plugin");

	IOLockUnlock(mainLock);
	return success;
}

void SMIMonitor::start() {
	IOLockLock(mainLock);
	if (!initialUpdateSensors) {
		for (int i=0; i<fanCount; ++i)
			i8k_set_fan_control_auto(state.fanInfo[i].index); // force automatic control
		updateSensors();
		initialUpdateSensors = true;
	}
	IOLockUnlock(mainLock);
}

void SMIMonitor::handlePowerOff() {
	timerEventSource->disable();
}

void SMIMonitor::handlePowerOn() {
	timerEventSource->enable();
}

bool SMIMonitor::findFanSensors() {
	fanCount = 0;

	for (int i = 0; i < state.MaxFanSupported; i++) {
		state.fanInfo[i] = {};
		int rc = i8k_get_fan_status(i);
		if (rc >= 0)
		{
			state.fanInfo[fanCount].index = i;
			state.fanInfo[fanCount].status = rc;
			state.fanInfo[fanCount].minSpeed = i8k_get_fan_nominal_speed(i, 1);
			state.fanInfo[fanCount].maxSpeed = i8k_get_fan_nominal_speed(i, 2);
		
			int type = i8k_get_fan_type(i);
			if ((type > FanInfo::Unsupported) && (type < FanInfo::Last))
			{
				state.fanInfo[fanCount].type = static_cast<FanInfo::SMMFanType>(type);
			}
			DBGLOG("sdell", "Fan %d has been detected, type=%d, status=%d, minSpeed=%d, maxSpeed=%d", i, type, rc, state.fanInfo[fanCount].minSpeed, state.fanInfo[fanCount].maxSpeed);

			fanCount++;
		}
	}
	
	return fanCount > 0;
}

bool SMIMonitor::findTempSensors() {
	tempCount = 0;
	
	for (int i=0; i<state.MaxTempSupported; i++)
	{
		state.tempInfo[i] = {};
		int rc = i8k_get_temp(i);
		if (rc >= 0)
		{
			state.tempInfo[tempCount].index = i;
			int type = i8k_get_temp_type(i);
			state.tempInfo[tempCount].type = static_cast<TempInfo::SMMTempSensorType>(type);
			state.tempInfo[i].temp = rc;
			DBGLOG("sdell", "Temp sensor %d has type %d, temp = %d", i, state.tempInfo[tempCount].type, rc);
			tempCount++;
		}
	}
	
	return tempCount > 0;
}

void SMIMonitor::updateSensors() {
	if (timerEventSource == nullptr) {
		DBGLOG("sdell", "WTF timerEventSource is null");
		return;
	}
	timerEventSource->cancelTimeout();
	
	for (int i=0; i<fanCount; ++i)
	{
		int sensor = state.fanInfo[i].index;
		int rc = i8k_get_fan_speed(sensor);
		if (rc >= 0)
			state.fanInfo[i].speed = rc;
		IOSleep(200);
	}

	for (int i=0; i<tempCount; ++i)
	{
		int sensor = state.tempInfo[i].index;
		int rc = i8k_get_temp(sensor);
		if (rc >= 0)
			state.tempInfo[i].temp = rc;
		IOSleep(200);
	}
	
	timerEventSource->setTimeoutMS(400);
}
