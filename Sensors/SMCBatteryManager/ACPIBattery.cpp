//
//  ACPIBattery.cpp
//  SMCBatteryManager
//
//  Copyright © 2018 usrsse2. All rights reserved.
//

#include "ACPIBattery.hpp"
#include <Sensors/Private/AppleSmartBatteryCommands.h>
#include <Headers/kern_util.hpp>
#include <Headers/kern_compat.hpp>

uint32_t ACPIBattery::getNumberFromArray(OSArray *array, uint32_t index) {
	auto number = OSDynamicCast(OSNumber, array->getObject(index));
	if (number)
		return number->unsigned32BitValue();
	return BatteryInfo::ValueUnknown;
}

void ACPIBattery::getStringFromArray(OSArray *array, uint32_t index, char *dst, uint32_t dstSize) {
	if (dstSize == 0)
		return;

	const char *src = nullptr;
	auto object = array->getObject(index);
	if (object) {
		if (auto data = OSDynamicCast(OSData, object)) {
			src = reinterpret_cast<const char *>(data->getBytesNoCopy());
			dstSize = min(dstSize - 1, data->getLength());
		} else if (auto string = OSDynamicCast(OSString, object)) {
			src = string->getCStringNoCopy();
			dstSize = min(dstSize - 1, string->getLength());
		}
	}

	if (src) {
		lilu_os_strncpy(dst, src, dstSize);
		dst[dstSize] = '\0';
	} else {
		dst[0] = '\0';
	}
}

bool ACPIBattery::getBatteryInfo(BatteryInfo &bi, bool extended) {
	OSObject *object = nullptr;
	if (device->evaluateObject(extended ? AcpiBatteryInformationEx : AcpiBatteryInformation, &object) != kIOReturnSuccess)
		return false;

	OSArray *info = OSDynamicCast(OSArray, object);
	if (!info) {
		object->release();
		return false;
	}

	if (extended) {
		if (getNumberFromArray(info, BIXPowerUnit) == 0)
			bi.state.powerUnitIsWatt = true;
		bi.designCapacity               = getNumberFromArray(info, BIXDesignCapacity);
		bi.state.lastFullChargeCapacity = getNumberFromArray(info, BIXLastFullChargeCapacity);
		bi.technology                   = getNumberFromArray(info, BIXBatteryTechnology);
		bi.state.designVoltage          = getNumberFromArray(info, BIXDesignVoltage);
		bi.state.designCapacityWarning  = getNumberFromArray(info, BIXDesignCapacityOfWarning);
		bi.state.designCapacityLow      = getNumberFromArray(info, BIXDesignCapacityOfLow);
		bi.cycle                        = getNumberFromArray(info, BIXCycleCount);
		getStringFromArray(info, BIXModelNumber, bi.deviceName, BatteryInfo::MaxStringLen);
		getStringFromArray(info, BIXSerialNumber, bi.serial, BatteryInfo::MaxStringLen);
		getStringFromArray(info, BIXBatteryType, bi.batteryType, BatteryInfo::MaxStringLen);
		getStringFromArray(info, BIXOEMInformation, bi.manufacturer, BatteryInfo::MaxStringLen);
	} else {
		if (getNumberFromArray(info, BIFPowerUnit) == 0)
			bi.state.powerUnitIsWatt = true;
		bi.designCapacity               = getNumberFromArray(info, BIFDesignCapacity);
		bi.state.lastFullChargeCapacity = getNumberFromArray(info, BIFLastFullChargeCapacity);
		bi.technology                   = getNumberFromArray(info, BIFBatteryTechnology);
		bi.state.designVoltage          = getNumberFromArray(info, BIFDesignVoltage);
		bi.state.designCapacityWarning  = getNumberFromArray(info, BIFDesignCapacityOfWarning);
		bi.state.designCapacityLow      = getNumberFromArray(info, BIFDesignCapacityOfLow);
		getStringFromArray(info, BIFModelNumber, bi.deviceName, BatteryInfo::MaxStringLen);
		getStringFromArray(info, BIFSerialNumber, bi.serial, BatteryInfo::MaxStringLen);
		getStringFromArray(info, BIFBatteryType, bi.batteryType, BatteryInfo::MaxStringLen);
		getStringFromArray(info, BIFOEMInformation, bi.manufacturer, BatteryInfo::MaxStringLen);
	}

	info->release();

	bi.validateData(id);

	if (!extended) {
		// Assume battery designed for 1000 cycles
		if (bi.designCapacity && bi.state.lastFullChargeCapacity)
			bi.cycle = (bi.designCapacity - bi.state.lastFullChargeCapacity) * 1000 / bi.designCapacity;
		else
			bi.cycle = 0;
	}

	return true;
}

bool ACPIBattery::updateRealTimeStatus(bool quickPoll) {
	OSObject *acpi = nullptr;
	if (device->evaluateObject(AcpiBatteryStatus, &acpi) != kIOReturnSuccess) {
		//CHECKME: unsure why this should be done here, this may be a mistake
		// of the original implementation, in which case no code is to be run.
		IOSimpleLockLock(batteryInfoLock);
		batteryInfo->state.lastRemainingCapacity = batteryInfo->state.remainingCapacity;
		IOSimpleLockUnlock(batteryInfoLock);
		return false;
	}

	auto status = OSDynamicCast(OSArray, acpi);
	if (!status) {
		IOSimpleLockLock(batteryInfoLock);
		batteryInfo->connected = false;
		//CHECKME: unsure why this should be done here, this may be a mistake
		// of the original implementation, in which case no code is to be run.
		batteryInfo->state.lastRemainingCapacity = batteryInfo->state.remainingCapacity;
		IOSimpleLockUnlock(batteryInfoLock);
		SYSLOG("acpib", "error in ACPI data");
		acpi->release();
		return false;
	}

	IOSimpleLockLock(batteryInfoLock);
	auto st = batteryInfo->state;
	IOSimpleLockUnlock(batteryInfoLock);

	st.state = getNumberFromArray(status, BSTState);
	st.presentRate = getNumberFromArray(status, BSTPresentRate);
	st.remainingCapacity = getNumberFromArray(status, BSTRemainingCapacity);
	st.presentVoltage = getNumberFromArray(status, BSTPresentVoltage);
	if (st.powerUnitIsWatt) {
		st.presentRate = st.presentRate * 1000 / st.designVoltage;
		st.remainingCapacity = st.remainingCapacity * 1000 / st.designVoltage;
	}

	// Sometimes this value can be either reported incorrectly or miscalculated
	// and exceed the actual capacity. Simply workaround it by capping the value.
	// REF: https://github.com/acidanthera/bugtracker/issues/565
	if (st.remainingCapacity > st.lastFullChargeCapacity)
		st.remainingCapacity = st.lastFullChargeCapacity;

	// Average rate calculation
	if (!st.presentRate || (st.presentRate == BatteryInfo::ValueUnknown)) {
		auto delta = (st.remainingCapacity > st.lastRemainingCapacity ?
					  st.remainingCapacity - st.lastRemainingCapacity :
					  st.lastRemainingCapacity - st.remainingCapacity);
		uint32_t interval = quickPoll ? 3600 / (QuickPollInterval / 1000) : 3600 / (NormalPollInterval / 1000);
		st.presentRate = delta * interval;
	}

	if (!st.averageRate) {
		st.averageRate = st.presentRate;
	} else {
		st.averageRate += st.presentRate;
		st.averageRate >>= 1;
	}

	uint32_t highAverageBound = st.presentRate * (100 + AverageBoundPercent) / 100;
	uint32_t lowAverageBound  = st.presentRate * (100 - AverageBoundPercent) / 100;
	if (st.averageRate > highAverageBound)
		st.averageRate = highAverageBound;
	if (st.averageRate < lowAverageBound)
		st.averageRate = lowAverageBound;

	// Remaining capacity
	if (!st.remainingCapacity || st.remainingCapacity == BatteryInfo::ValueUnknown) {
		SYSLOG("acpib", "battery %d has no remaining capacity reported (%u)", id, st.remainingCapacity);
		// Invalid or zero capacity is not allowed and will lead to boot failure, hack 1 here.
		st.remainingCapacity = st.averageTimeToEmpty = st.runTimeToEmpty = 1;
	} else {
		st.averageTimeToEmpty = st.averageRate ? 60 * st.remainingCapacity / st.averageRate : 60 * st.remainingCapacity;
		st.runTimeToEmpty = st.presentRate ? 60 * st.remainingCapacity / st.presentRate : 60 * st.remainingCapacity;
	}

	// Voltage
	if (!st.presentVoltage || st.presentVoltage == BatteryInfo::ValueUnknown)
		st.presentVoltage = st.designVoltage;

	// Check battery state
	bool bogus = false;
	switch (st.state & BSTStateMask) {
		case BSTNotCharging: {
			if (!st.batteryIsFull) {
				DBGLOG("acpib", "battery %d full, need stats update", id);
				st.needUpdate = true;
			} else {
				DBGLOG("acpib", "battery %d full", id);
			}
			st.calculatedACAdapterConnected = true;
			st.batteryIsFull = true;
			st.timeToFull = 0;
			st.signedPresentRate = st.presentRate;
			st.signedAverageRate = st.averageRate;
			break;
		}
		case BSTDischarging: {
			if (st.calculatedACAdapterConnected) {
				DBGLOG("acpib", "battery %d discharging, need stats update", id);
				st.needUpdate = true;
			} else {
				DBGLOG("acpib", "battery %d discharging", id);
			}
			st.calculatedACAdapterConnected = false;
			st.batteryIsFull = false;
			st.timeToFull = 0;
			st.signedPresentRate = -st.presentRate;
			st.signedAverageRate = -st.averageRate;
			break;
		}
		case BSTCharging: {
			DBGLOG("acpib", "battery %d charging", id);
			st.calculatedACAdapterConnected = true;
			st.batteryIsFull = false;
			int diff = st.remainingCapacity < st.lastFullChargeCapacity ? st.lastFullChargeCapacity - st.remainingCapacity : 0;
			st.timeToFull = st.averageRate ? 60 * diff / st.averageRate : 60 * diff;
			st.signedPresentRate = st.presentRate;
			st.signedAverageRate = st.averageRate;
			break;
		}
		default: {
			SYSLOG("acpib", "bogus status data from battery %d (%x)", id, st.state);
			st.batteryIsFull = false;
			bogus = true;
			break;
		}
	}

	st.lastRemainingCapacity = st.remainingCapacity;

	// bool warning  = st.remainingCapacity <= st.designCapacityWarning || st.runTimeToEmpty < 10;
	bool critical = st.remainingCapacity <= st.designCapacityLow || st.runTimeToEmpty < 5;
	if (st.state & BSTCritical) {
		DBGLOG("acpib", "battery %d is in critical state", id);
		critical = true;
	}

	// When we report battery failure, AppleSmartBatteryManager sets isCharging=false.
	// So we don't report battery failure when it's charging.
	st.bad = (st.state & BSTStateMask) != BSTCharging && st.lastFullChargeCapacity < 2 * st.designCapacityWarning;
	st.bogus = bogus;
	st.critical = critical;

	acpi->release();

	IOSimpleLockLock(batteryInfoLock);
	batteryInfo->state = st;
	IOSimpleLockUnlock(batteryInfoLock);

	// one more poll when a stats update is needed but already full
	return (st.needUpdate ? false : st.batteryIsFull);
}

bool ACPIBattery::updateStaticStatus(bool *calculatedACAdapterConnection) {
	if (device == nullptr) {
		PANIC("acpib", "checkStatus found device to be null");
		return false;
	}

	uint32_t acpi = 0;
	if (device->evaluateInteger(AcpiStatus, &acpi) == kIOReturnSuccess) {
		DBGLOG("acpib", "return status %x", acpi);

		bool connected = (acpi & 0x10) ? true : false;
		IOSimpleLockLock(batteryInfoLock);
		if (connected == batteryInfo->connected && !batteryInfo->state.needUpdate) {
			// No status change, most likely, just continue
			if (calculatedACAdapterConnection)
				*calculatedACAdapterConnection = batteryInfo->state.calculatedACAdapterConnected;
			IOSimpleLockUnlock(batteryInfoLock);
		} else if (!connected) {
			// Disconnected, reset the state
			*batteryInfo = BatteryInfo{};
			IOSimpleLockUnlock(batteryInfoLock);
			if (calculatedACAdapterConnection)
				*calculatedACAdapterConnection = false;
			DBGLOG("acpib", "battery %d disconnected", id);
		} else {
			// Connected, most expensive
			IOSimpleLockUnlock(batteryInfoLock);
			BatteryInfo bi;
			bi.connected = true;
			if (!getBatteryInfo(bi, true) && !getBatteryInfo(bi, false))
				bi.connected = false;

			if (calculatedACAdapterConnection)
				*calculatedACAdapterConnection = bi.state.calculatedACAdapterConnected;

			IOSimpleLockLock(batteryInfoLock);
			*batteryInfo = bi;
			IOSimpleLockUnlock(batteryInfoLock);

			DBGLOG("acpib", "battery %d connected or updated -> %d", id, bi.connected);
		}

		return connected;
	}

	// Failure to obtain battery status, this is very bad
	IOSimpleLockLock(batteryInfoLock);
	batteryInfo->connected = false;
	batteryInfo->state.calculatedACAdapterConnected = false;
	if (calculatedACAdapterConnection) *calculatedACAdapterConnection = false;
	IOSimpleLockUnlock(batteryInfoLock);
	return false;
}

uint16_t ACPIBattery::calculateBatteryStatus() {
	uint16_t value = 0;
	if (batteryInfo->connected) {
		value = kBInitializedStatusBit;
		auto st = batteryInfo->state.state;
		if (st & BSTDischarging)
			value |= kBDischargingStatusBit;
		if (st & BSTCritical)
			value |= kBFullyDischargedStatusBit;
		if ((st & BSTStateMask) == BSTNotCharging) {
			if (batteryInfo->state.lastFullChargeCapacity > 0 &&
				batteryInfo->state.lastFullChargeCapacity != BatteryInfo::ValueUnknown &&
				batteryInfo->state.lastFullChargeCapacity <= BatteryInfo::ValueMax &&
				batteryInfo->state.remainingCapacity > 0 &&
				batteryInfo->state.remainingCapacity != BatteryInfo::ValueUnknown &&
				batteryInfo->state.remainingCapacity <= BatteryInfo::ValueMax &&
				batteryInfo->state.remainingCapacity >= batteryInfo->state.lastFullChargeCapacity)
				value |= kBFullyChargedStatusBit;
			value |= kBTerminateChargeAlarmBit;
		}

		if (batteryInfo->state.bad) {
			value |= kBTerminateChargeAlarmBit;
			value |= kBTerminateDischargeAlarmBit;
		}
	}

	return value;
}
