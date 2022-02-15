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
#include "kern_hooks.hpp"

#include <Headers/kern_cpu.hpp>

extern "C" {
#include <i386/pmCPU.h>
}

extern "C" int dell_smm_lowlevel(SMBIOS_PKG *smm_pkg);

SMIMonitor *SMIMonitor::instance = nullptr;
atomic_bool SMIMonitor::busy = 0;

OSDefineMetaClassAndStructors(SMIMonitor, OSObject)


int SMIMonitor::i8k_smm(SMBIOS_PKG *sc, bool force_access) {
	
	int attempts = 50;
	while (atomic_load_explicit(&KERNELHOOKS::active_output, memory_order_acquire) && --attempts >= 0) { IOSleep(4); }
	if (attempts < 0 && !force_access)
		return -1;

	atomic_store_explicit(&busy, true, memory_order_release);
	
	if (atomic_load_explicit(&KERNELHOOKS::active_output, memory_order_acquire) && !force_access) {
		DBGLOG("sdell", "stop accessing smm, active_outputs = %d", atomic_load_explicit(&KERNELHOOKS::active_output, memory_order_acquire));
		atomic_store_explicit(&busy, false, memory_order_release);
		return -1;
	}

	int result = dell_smm_lowlevel(sc);
	
	atomic_store_explicit(&busy, false, memory_order_release);
	
	return result;
}


/*
 * Read the CPU temperature in Celcius.
 */
int SMIMonitor::i8k_get_temp(int sensor, bool force_access) {
	SMBIOS_PKG smm_pkg {};
	int rc;
	int temp;

	smm_pkg.cmd = I8K_SMM_GET_TEMP;
	smm_pkg.data = sensor & 0xFF;
	if ((rc=i8k_smm(&smm_pkg, force_access)) < 0) {
		return rc;
	}

	temp = smm_pkg.cmd & 0xff;
	if (temp == 0x99) {
		IOSleep(100);
		smm_pkg.cmd = I8K_SMM_GET_TEMP;
		smm_pkg.data = sensor & 0xFF;
		if ((rc=i8k_smm(&smm_pkg, force_access)) < 0) {
			return rc;
		}
		temp = smm_pkg.cmd & 0xff;
	}
	return temp;
}

int SMIMonitor::i8k_get_temp_type(int sensor) {
	SMBIOS_PKG smm_pkg {};
	int rc;
	int type;

	smm_pkg.cmd = I8K_SMM_GET_TEMP_TYPE;
	smm_pkg.data = sensor & 0xFF;
	if ((rc=i8k_smm(&smm_pkg, true)) < 0) {
		return rc;
	}

	type = smm_pkg.cmd & 0xff;
	return type;
}

bool SMIMonitor::i8k_get_dell_sig_aux(int fn) {
	SMBIOS_PKG smm_pkg {};

	smm_pkg.cmd = fn;
	if (i8k_smm(&smm_pkg, true) < 0) {
		DBGLOG("sdell", "No function 0x%x", fn);
		return false;
	}
	DBGLOG("sdell", "Got sigs 0x%X and 0x%X", smm_pkg.cmd, smm_pkg.stat2);
	return ((smm_pkg.cmd == 0x44494147 /*DIAG*/) &&
			(smm_pkg.stat2 == 0x44454C4C /*DELL*/));
}

bool SMIMonitor::i8k_get_dell_signature() {
	return (i8k_get_dell_sig_aux(I8K_SMM_GET_DELL_SIG1) ||
		    i8k_get_dell_sig_aux(I8K_SMM_GET_DELL_SIG2));
}

/*
 * Read the fan speed in RPM.
 */
int SMIMonitor::i8k_get_fan_speed(int fan, bool force_access) {
	SMBIOS_PKG smm_pkg {};
	int rc;
	int speed = 0;

	smm_pkg.cmd = I8K_SMM_GET_SPEED;
	smm_pkg.data = fan & 0xff;
	if ((rc=i8k_smm(&smm_pkg, force_access)) < 0) {
		return rc;
	}
	speed = (smm_pkg.cmd & 0xffff) * fanMult;
	return speed;
}

/*
 * Read the fan status.
 */
int SMIMonitor::i8k_get_fan_status(int fan) {
	SMBIOS_PKG smm_pkg {};
	int rc;

	smm_pkg.cmd = I8K_SMM_GET_FAN;
	smm_pkg.data = fan & 0xff;
	if ((rc=i8k_smm(&smm_pkg, true)) < 0) {
		return rc;
	}

	return (smm_pkg.cmd & 0xff);
}

/*
 * Read the fan status.
 */
int SMIMonitor::i8k_get_fan_type(int fan) {
	SMBIOS_PKG smm_pkg {};
	int rc;

	smm_pkg.cmd = I8K_SMM_GET_FAN_TYPE;
	smm_pkg.data = fan & 0xff;
	if ((rc=i8k_smm(&smm_pkg, true)) < 0) {
		return rc;
	}

	return (smm_pkg.cmd & 0xff);
}


/*
 * Read the fan nominal rpm for specific fan speed (0,1,2) or zero
 */
int SMIMonitor::i8k_get_fan_nominal_speed(int fan, int speed) {
	SMBIOS_PKG smm_pkg {};
	smm_pkg.cmd = I8K_SMM_GET_NOM_SPEED;
	smm_pkg.data = (fan & 0xff) | (speed << 8);
	return i8k_smm(&smm_pkg, true) ? 0 : (smm_pkg.cmd & 0xffff) * fanMult;
}

/*
 * Set the fan speed (off, low, high). Returns the new fan status.
 */
int SMIMonitor::i8k_set_fan(int fan, int speed) {
	SMBIOS_PKG smm_pkg {};
	smm_pkg.cmd = I8K_SMM_SET_FAN;

	speed = (speed < 0) ? 0 : ((speed > I8K_FAN_MAX) ? I8K_FAN_MAX : speed);
	smm_pkg.data = (fan & 0xff) | (speed << 8);

	return i8k_smm(&smm_pkg, true);
}

int SMIMonitor::i8k_set_fan_control_manual(int fan) {
	// we have to write to both control registers since some Dell models
	// support only one register and smm does not return error for unsupported one
	SMBIOS_PKG smm_pkg {};
	smm_pkg.cmd = I8K_SMM_IO_DISABLE_FAN_CTL1;
	smm_pkg.data = (fan & 0xff);
	int result1 = i8k_smm(&smm_pkg, true);
	
	smm_pkg = {};
	smm_pkg.cmd = I8K_SMM_IO_DISABLE_FAN_CTL2;
	smm_pkg.data = (fan & 0xff);
	int result2 = i8k_smm(&smm_pkg, true);
	
	return (result1 >= 0) ? result1 : result2;
}

int SMIMonitor::i8k_set_fan_control_auto(int fan) {
	// we have to write to both control registers since some Dell models
	// support only one register and smm does not return error for unsupported one
	SMBIOS_PKG smm_pkg {};
	smm_pkg.cmd = I8K_SMM_IO_ENABLE_FAN_CTL1;
	smm_pkg.data = (fan & 0xff);
	int result1 = i8k_smm(&smm_pkg, true);
	
	smm_pkg = {};
	smm_pkg.cmd = I8K_SMM_IO_ENABLE_FAN_CTL2;
	smm_pkg.data = (fan & 0xff);
	int result2 = i8k_smm(&smm_pkg, true);

	return (result1 >= 0) ? result1 : result2;
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

	return success;
}

void SMIMonitor::start() {
	DBGLOG("sdell", "Based on I8kfan project and adopted to VirtualSMC plugin");
}

void SMIMonitor::handlePowerOff() {
	if (awake) {
		ignore_new_smc_updates = true;
		if (fansStatus != 0) {
			// turn off manual control for all fans
			UInt16 data = 0;
			postSmcUpdate(KeyFS__, -1, &data, sizeof(data), true);
		}
		DBGLOG("sdell", "SMIMonitor switched to sleep state, smc updates before sleep: %d", storedSmcUpdates.size());
		while (storedSmcUpdates.size() != 0) { IOSleep(10); }
		awake = false;
	}
}

void SMIMonitor::handlePowerOn() {
	if (!awake) {
		awake = true;
		ignore_new_smc_updates = false;
		DBGLOG("sdell", "SMIMonitor switched to awake state");
	}
}

bool SMIMonitor::postSmcUpdate(SMC_KEY key, size_t index, const void *data, uint32_t dataSize, bool force_update)
{
	if (!force_update && (!awake || ignore_new_smc_updates)) {
		DBGLOG("sdell", "SMIMonitor: postSmcUpdate for key %d has been ignored", key);
		return false;
	}
	
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
			rc = i8k_get_fan_speed(i, true);
			if (rc >= 0)
				state.fanInfo[i].speed = rc;

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
		int rc = i8k_get_temp(i, true);
		if (rc < 0) {
			IOSleep(100);
			rc = i8k_get_temp(i, true);
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
		
		bool force_access = (--force_update_counter >= 0);

		for (int i=0; i<fanCount && awake; ++i)
		{
			int sensor = state.fanInfo[i].index;
			int rc = i8k_get_fan_speed(sensor, force_access);
			if (rc >= 0)
				state.fanInfo[i].speed = rc;
			else
				DBGLOG("sdell", "SMM reading error %d for fan %d", rc, sensor);
			handleSmcUpdatesInIdle(4);
		}

		for (int i=0; i<tempCount && awake; ++i)
		{
			int sensor = state.tempInfo[i].index;
			int rc = i8k_get_temp(sensor, force_access);
			if (rc >= 0)
				state.tempInfo[i].temp = rc;
			else
				DBGLOG("sdell", "SMM reading error %d for temp sensor %d", rc, sensor);
			handleSmcUpdatesInIdle(4);
		}
		
		handleSmcUpdatesInIdle(5);
	}
}

void SMIMonitor::handleSmcUpdatesInIdle(int idle_loop_count)
{
	for (int i=0; i<idle_loop_count; ++i)
	{
		if (awake && storedSmcUpdates.size() != 0) {
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

		IOSleep(100);
	}
}

void SMIMonitor::hanldeManualControlUpdate(size_t index, UInt8 *data)
{
	UInt16 val = data[0];
	int rc = 0;
	if (val != (fansStatus & (1 << index))>>index) {
		rc = val ? i8k_set_fan_control_manual(state.fanInfo[index].index) :
					i8k_set_fan_control_auto(state.fanInfo[index].index);
	}
	if (rc == 0) {
		auto newStatus = val ? (fansStatus | (1 << index)) : (fansStatus & ~(1 << index));
		if (fansStatus != newStatus) {
			fansStatus = newStatus;
			force_update_counter = 10;
			DBGLOG("sdell", "Set manual mode for fan %d to %s, global fansStatus = 0x%02x", index, val ? "enable" : "disable", fansStatus);
		}
	}
	else
		SYSLOG("sdell", "Set manual mode for fan %d to %d failed: %d", index, val, rc);
}

void SMIMonitor::hanldeManualTargetSpeedUpdate(size_t index, UInt8 *data)
{
	auto value = VirtualSMCAPI::decodeIntFp(SmcKeyTypeFpe2, *reinterpret_cast<const uint16_t *>(data));
	state.fanInfo[index].targetSpeed = value;

	if (fansStatus & (1 << index)) {
		int status = 1;
		int range = state.fanInfo[index].maxSpeed - state.fanInfo[index].minSpeed;
		if (value > state.fanInfo[index].minSpeed + range/2)
			status = 2;
		else if (state.fanInfo[index].stopOffset != 0 && value < (state.fanInfo[index].minSpeed + state.fanInfo[index].stopOffset))
			status = 0;		// stop fan
		
		int current_status = i8k_get_fan_status(state.fanInfo[index].index);
		state.fanInfo[index].status = current_status;
		if (current_status < 0 || current_status != status) {
			int rc = i8k_set_fan(state.fanInfo[index].index, status);
			if (rc != 0)
				SYSLOG("sdell", "Set target speed for fan %d to %d failed: %d", index, value, rc);
			else {
				state.fanInfo[index].status = status;
				force_update_counter = 10;
				DBGLOG("sdell", "Set target speed for fan %d to %d, status = %d", index, value, status);
			}
		}
	}
	else
		SYSLOG("sdell", "Set target speed for fan %d to %d ignored since auto control is active", index, value);
}

void SMIMonitor::handleManualForceFanControlUpdate(UInt8 *data)
{
	auto val = (data[0] << 8) + data[1]; //big endian data

	int rc = 0;
	for (int i = 0; i < fanCount; i++) {
		if ((val & (1 << i)) != (fansStatus & (1 << i))) {
			rc |= (val & (1 << i)) ? i8k_set_fan_control_manual(state.fanInfo[i].index) :
									 i8k_set_fan_control_auto(state.fanInfo[i].index);
		}
	}

	if (rc == 0) {
		if (fansStatus != val) {
			fansStatus = val;
			force_update_counter = 10;
			DBGLOG("sdell", "Set force fan mode to %d", val);
		}
	}
	else
		SYSLOG("sdell", "Set force fan mode to %d failed: %d", val, rc);
}
