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
#include <Headers/kern_cpu.hpp>

extern "C" {
#include <i386/pmCPU.h>
}

SMIMonitor *SMIMonitor::instance = nullptr;

OSDefineMetaClassAndStructors(SMIMonitor, OSObject)

int SMIMonitor::i8k_smm(SMMRegisters *regs) {
	int rc;
	int eax = regs->eax;  //input value
	
	IOSimpleLockLock(preemptionLock);
	
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
	
	IOSimpleLockUnlock(preemptionLock);

	if ((rc != 0) || ((regs->eax & 0xffff) == 0xffff) || (regs->eax == eax)) {
		return -1;
	}

	return 0;
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

bool SMIMonitor::i8k_get_dell_signature() {
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
	instance->queueLock = IOSimpleLockAlloc();
	if (!instance->queueLock)
		PANIC("sdell", "failed to allocate simple lock");
	// Reserve SMC updates slots
	if (!instance->storedSmcUpdates.reserve(MaxActiveSmcUpdates))
		PANIC("sdell", "failed to reserve SMC updates slots");
	instance->preemptionLock = IOSimpleLockAlloc();
	if (!instance->preemptionLock)
		PANIC("sdell", "failed to allocate simple lock");
}

bool SMIMonitor::probe() {

	bool success = true;
	
	while (!updateCall) {
		updateCall = thread_call_allocate(staticUpdateThreadEntry, this);
		if (!updateCall) {
			DBGLOG("sdell", "Update thread cannot be created");
			success = false;
			break;
		}
		
		IOLockLock(mainLock);
		thread_call_enter(updateCall);
		
		while (initialized == -1) {
			IOLockSleep(mainLock, &initialized, THREAD_UNINT);
		}
		IOLockUnlock(mainLock);

		if (initialized != KERN_SUCCESS) {
			success = false;
			break;
		}

		DBGLOG("sdell", "found %u fan sensors and %u temp sensors", fanCount, tempCount);
		break;
	}

	if (!success) {
		if (updateCall) {
			while (!thread_call_free(updateCall))
				thread_call_cancel(updateCall);
			updateCall = nullptr;
		}
	}
	
	DBGLOG("sdell", "Based on I8kfan project and adopted to VirtualSMC plugin");

	return success;
}

void SMIMonitor::start() {
}

void SMIMonitor::handlePowerOff() {
	if (awake) {
		awake = false;
	}
}

void SMIMonitor::handlePowerOn() {
	if (!awake) {
		awake = true;
	}
}

bool SMIMonitor::postSmcUpdate(SMC_KEY key, size_t index, const void *data, uint32_t dataSize)
{
	IOSimpleLockLock(queueLock);

	bool success = false;
	while (1) {
	
		if (dataSize > sizeof(StoredSmcUpdate::data)) {
			SYSLOG("sdell", "postRequest dataSize overflow %u", dataSize);
			break;
		}

		for (size_t i = 0; i < storedSmcUpdates.size() && !success; i++) {
			auto &si = storedSmcUpdates[i];
			if (si.key == key && si.index == index) {
				si.size = dataSize;
				if (dataSize > 0)
					lilu_os_memcpy(si.data, data, dataSize);
				success = true;
				break;
			}
		}
		
		if (success) break;

		if (storedSmcUpdates.size() == MaxActiveSmcUpdates) {
			SYSLOG("sdell", "postRequest reserve overflow");
			break;
		}

		StoredSmcUpdate si {key, index, dataSize};
		if (dataSize > 0)
			lilu_os_memcpy(si.data, data, dataSize);
		if (storedSmcUpdates.push_back(si)) {
			success = true;
			break;
		}
		break;
	}
	IOSimpleLockUnlock(queueLock);
	
	if (!success)
		SYSLOG("sdell", "unable to store smc update");

	return success;
}

IOReturn SMIMonitor::bindCurrentThreadToCpu0()
{
	// Obtain power management callbacks 10.7+
	pmCallBacks_t callbacks {};
	pmKextRegister(PM_DISPATCH_VERSION, nullptr, &callbacks);
	
	if (!callbacks.LCPUtoProcessor) {
		SYSLOG("sdell", "failed to obtain LCPUtoProcessor");
		return KERN_FAILURE;
	}
	
	if (!callbacks.ThreadBind) {
		SYSLOG("sdell", "failed to obtain ThreadBind");
		return KERN_FAILURE;
	}
	
	if (!IOSimpleLockTryLock(preemptionLock)) {
		SYSLOG("sdell", "Preemption cannot be disabled before performing ThreadBind");
		return KERN_FAILURE;
	}

	bool success = true;
	auto enable = ml_set_interrupts_enabled(FALSE);
	
	while (1)
	{
		auto processor = callbacks.LCPUtoProcessor(0);
		if (processor == nullptr) {
			SYSLOG("sdell", "failed to call LCPUtoProcessor with cpu 0");
			success = false;
			break;
		}
		callbacks.ThreadBind(processor);
		break;
	}

	IOSimpleLockUnlock(preemptionLock);
	ml_set_interrupts_enabled(enable);
	
	return success ? KERN_SUCCESS : KERN_FAILURE;
}

bool SMIMonitor::findFanSensors() {
	fanCount = 0;

	for (int i = 0; i < state.MaxFanSupported; i++) {
		state.fanInfo[i] = {};
		int rc = i8k_get_fan_status(i);
		if (rc < 0) {
			IOSleep(100);
			rc = i8k_get_fan_status(i);
		}
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
		if (rc < 0) {
			IOSleep(100);
			rc = i8k_get_temp(i);
		}
		if (rc >= 0)
		{
			state.tempInfo[tempCount].index = i;
			int type = i8k_get_temp_type(i);
			state.tempInfo[tempCount].type = static_cast<TempInfo::SMMTempSensorType>(type);
			state.tempInfo[i].temp = rc;
			DBGLOG("sdell", "Temp sensor %d has been detected, type %d, temp = %d", i, state.tempInfo[tempCount].type, rc);
			tempCount++;
		}
	}
	
	return tempCount > 0;
}

void SMIMonitor::staticUpdateThreadEntry(thread_call_param_t param0, thread_call_param_t param1)
{
	auto *that = OSDynamicCast(SMIMonitor, reinterpret_cast<OSObject*>(param0));
	if (!that) {
		SYSLOG("sdell", "Failed to get pointer to SMIMonitor");
		return;
	}

	bool success = true;
	while (1) {
			
		IOReturn result = that->bindCurrentThreadToCpu0();
		if (result != KERN_SUCCESS) {
			success = false;
			break;
		}
		
		IOSleep(500);
		
		auto enable = ml_set_interrupts_enabled(FALSE);
		auto cpu_n = cpu_number();
		ml_set_interrupts_enabled(enable);
		
		if (cpu_n != 0) {
			DBGLOG("sdell", "staticUpdateThreadEntry is called in context CPU %d", cpu_n);
			success = false;
			break;
		}
		
		if (!that->i8k_get_dell_signature()) {
			SYSLOG("sdell", "Unable to get Dell SMM signature!");
			success = false;
			break;
		}

		if (!that->findFanSensors() || !that->findTempSensors()) {
			SYSLOG("sdell", "failed to find fans or temp sensors!");
			success = false;
			break;
		}
		
		break;
	}
	
	IOLockLock(that->mainLock);
	that->initialized = success ? KERN_SUCCESS : KERN_FAILURE;
	IOLockWakeup(that->mainLock, &that->initialized, true);
	IOLockUnlock(that->mainLock);
	
	if (success)
		that->updateSensorsLoop();
}

void SMIMonitor::updateSensorsLoop() {
	
	for (int i=0; i<fanCount; ++i)
		i8k_set_fan_control_auto(state.fanInfo[i].index); // force automatic control
		
	while (1) {
		
		for (int i=0; i<fanCount && awake; ++i)
		{
			int sensor = state.fanInfo[i].index;
			int rc = i8k_get_fan_speed(sensor);
			if (rc >= 0)
				state.fanInfo[i].speed = rc;
			else
				DBGLOG("sdell", "SMM reading error %d for fan %d", rc, sensor);
			handleSmcUpdatesInIdle(4);
		}

		for (int i=0; i<tempCount && awake; ++i)
		{
			int sensor = state.tempInfo[i].index;
			int rc = i8k_get_temp(sensor);
			if (rc >= 0)
				state.tempInfo[i].temp = rc;
			else
				DBGLOG("sdell", "SMM reading error %d for temp sensor %d", rc, sensor);
			handleSmcUpdatesInIdle(4);
		}
		
		handleSmcUpdatesInIdle(10);
	}
}

void SMIMonitor::handleSmcUpdatesInIdle(int idle_loop_count)
{
	for (int i=0; i<idle_loop_count; ++i)
	{
		if (awake) {
			IOSimpleLockLock(queueLock);
			if (storedSmcUpdates.size() > 0) {
				StoredSmcUpdate update = storedSmcUpdates[0];
				storedSmcUpdates.erase(0, false);
				IOSimpleLockUnlock(queueLock);
				
				switch (update.key) {
					case KeyF0Md:
						hanldeManualControlUpdate(update.index, update.data);
						break;
					case KeyF0Tg:
						hanldeManualTargetSpeedUpdate(update.index, update.data);
						break;
					case KeyFS__:
						handleManualForceFanControlUpdate(update.data);
						break;
				}
			}
			else {
				IOSimpleLockUnlock(queueLock);
			}
		}

		IOSleep(50);
	}
}

void SMIMonitor::hanldeManualControlUpdate(size_t index, UInt8 *data)
{
	UInt16 val = data[0];
	DBGLOG("sdell", "Set manual mode for fan %d to %d", index, val);

	int rc = 0;
	if (val != (fansStatus & (1 << index))>>index) {
		rc = val ? i8k_set_fan_control_manual(state.fanInfo[index].index) :
					i8k_set_fan_control_auto(state.fanInfo[index].index);
	}
	if (rc == 0) {
		fansStatus = val ? (fansStatus | (1 << index)) : (fansStatus & ~(1 << index));
		DBGLOG("sdell", "Set manual mode for fan %d to %d, fansStatus = 0x%02x", index, val, fansStatus);
	}
	else
		SYSLOG("sdell", "Set manual mode for fan %d to %d failed: %d", index, val, rc);
}

void SMIMonitor::hanldeManualTargetSpeedUpdate(size_t index, UInt8 *data)
{
	auto value = VirtualSMCAPI::decodeIntFp(SmcKeyTypeFpe2, *reinterpret_cast<const uint16_t *>(data));
	DBGLOG("sdell", "Set target speed for fan %d to %d", index, value);
	state.fanInfo[index].targetSpeed = value;

	if (fansStatus & (1 << index)) {
		int status = 1;
		int range = state.fanInfo[index].maxSpeed - state.fanInfo[index].minSpeed;
		if (value > state.fanInfo[index].minSpeed + range/2)
			status = 2;
		int rc = i8k_set_fan(state.fanInfo[index].index, status);
		if (rc != 0)
			SYSLOG("sdell", "Set target speed for fan %d to %d failed: %d", index, value, rc);	}
	else
		SYSLOG("sdell", "Set target speed for fan %d to %d ignored since auto control is active", index, value);
}

void SMIMonitor::handleManualForceFanControlUpdate(UInt8 *data)
{
	auto val = (data[0] << 8) + data[1]; //big endian data
	DBGLOG("sdell", "Set force fan mode to %d", val);

	int rc = 0;
	for (int i = 0; i < fanCount; i++) {
		if ((val & (1 << i)) != (fansStatus & (1 << i))) {
			rc |= (val & (1 << i)) ? i8k_set_fan_control_manual(state.fanInfo[i].index) :
									 i8k_set_fan_control_auto(state.fanInfo[i].index);
		}
	}

	if (rc == 0)
		fansStatus = val;
	else
		SYSLOG("sdell", "Set force fan mode to %d failed: %d", val, rc);
}
