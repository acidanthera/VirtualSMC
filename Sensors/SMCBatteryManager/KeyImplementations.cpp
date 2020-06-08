//
//  KeyImplementations.cpp
//  SMCBatteryManager
//
//  Copyright © 2018 usrsse2. All rights reserved.
//

#include "KeyImplementations.hpp"
#include "SMCBatteryManager.hpp"

SMC_RESULT ACID::readAccess() {
	IOSimpleLockLock(BatteryManager::getShared()->stateLock);
	auto extConnected = BatteryManager::getShared()->externalPowerConnected();
	IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);
	if (extConnected) {
		// Have some dummy value here for now, because ACPI has no means of getting adapter info
		// like power, voltage, serial number through only 2 pins - Vcc and GND.
		data[0] = 0xba;
		data[1] = 0xbe;
		data[2] = 0x3c;
		data[3] = 0x45;
		data[4] = 0xc0;
		data[5] = 0x03;
		data[6] = 0x10;
		data[7] = 0x43;
	} else {
		memset(data, 0, size);
	}
	return SmcSuccess;
}

SMC_RESULT ACIN::readAccess() {
	bool *ptr = reinterpret_cast<bool *>(data);
	IOSimpleLockLock(BatteryManager::getShared()->stateLock);
	*ptr = BatteryManager::getShared()->externalPowerConnected();
	IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);
	return SmcSuccess;
}

SMC_RESULT AC_N::readAccess() {
	data[0] = BatteryManager::getShared()->adapterCount;
	return SmcSuccess;
}

SMC_RESULT B0AC::readAccess() {
	int16_t *ptr = reinterpret_cast<int16_t *>(data);
	IOSimpleLockLock(BatteryManager::getShared()->stateLock);
	*ptr = OSSwapHostToBigInt16(BatteryManager::getShared()->state.btInfo[index].state.signedPresentRate);
	IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);
	return SmcSuccess;
}

SMC_RESULT B0AV::readAccess() {
	uint16_t *ptr = reinterpret_cast<uint16_t *>(data);
	IOSimpleLockLock(BatteryManager::getShared()->stateLock);
	*ptr = OSSwapHostToBigInt16(BatteryManager::getShared()->state.btInfo[index].state.presentVoltage);
	IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);
	return SmcSuccess;
}

SMC_RESULT B0BI::readAccess() {
	IOSimpleLockLock(BatteryManager::getShared()->stateLock);
	data[0] = BatteryManager::getShared()->state.btInfo[index].connected;
	IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);
	return SmcSuccess;
}

SMC_RESULT B0CT::readAccess() {
	uint16_t *ptr = reinterpret_cast<uint16_t *>(data);
	IOSimpleLockLock(BatteryManager::getShared()->stateLock);
	*ptr = OSSwapHostToBigInt16(BatteryManager::getShared()->state.btInfo[index].cycle);
	IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);
	return SmcSuccess;
}


SMC_RESULT B0FC::readAccess() {
	uint16_t *ptr = reinterpret_cast<uint16_t *>(data);
	IOSimpleLockLock(BatteryManager::getShared()->stateLock);
	*ptr = OSSwapHostToBigInt16(BatteryManager::getShared()->state.btInfo[index].state.lastFullChargeCapacity);
	IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);
	return SmcSuccess;
}

SMC_RESULT B0PS::readAccess() {
	//TODO: find what is its value when battery is the active power source
	data[0] = data[1] = 0;
	return SmcSuccess;
}

SMC_RESULT B0RM::readAccess() {
	uint16_t *ptr = reinterpret_cast<uint16_t *>(data);
	IOSimpleLockLock(BatteryManager::getShared()->stateLock);
	*ptr = OSSwapHostToBigInt16(BatteryManager::getShared()->state.btInfo[index].state.remainingCapacity);
	IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);
	return SmcSuccess;
}

SMC_RESULT B0St::readAccess() {
	uint16_t *ptr = reinterpret_cast<uint16_t *>(data);
	IOSimpleLockLock(BatteryManager::getShared()->stateLock);
	*ptr = OSSwapHostToBigInt16(BatteryManager::getShared()->calculateBatteryStatus(index));
	IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);
	return SmcSuccess;
}

SMC_RESULT B0TF::readAccess() {
	uint16_t *ptr = reinterpret_cast<uint16_t *>(data);
	IOSimpleLockLock(BatteryManager::getShared()->stateLock);
	auto state = BatteryManager::getShared()->state.btInfo[index].state.state & ACPIBattery::BSTStateMask;
	if (state == ACPIBattery::BSTCharging)
		*ptr = OSSwapHostToBigInt16(BatteryManager::getShared()->state.btInfo[index].state.timeToFull);
	else
		*ptr = 0xffff;
	IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);
	return SmcSuccess;
}

SMC_RESULT BATP::readAccess() {
	bool *ptr = reinterpret_cast<bool *>(data);
	IOSimpleLockLock(BatteryManager::getShared()->stateLock);
	*ptr = BatteryManager::getShared()->externalPowerConnected() == false;
	IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);
	return SmcSuccess;
}

SMC_RESULT BBAD::readAccess() {
	// TODO: what's with multiple batteries?
	bool *ptr = reinterpret_cast<bool *>(data);
	IOSimpleLockLock(BatteryManager::getShared()->stateLock);
	*ptr = BatteryManager::getShared()->state.btInfo[0].state.bad;
	IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);
	return SmcSuccess;
}

SMC_RESULT BBIN::readAccess() {
	bool *ptr = reinterpret_cast<bool *>(data);
	IOSimpleLockLock(BatteryManager::getShared()->stateLock);
	*ptr = BatteryManager::getShared()->batteriesConnected();
	IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);
	return SmcSuccess;
}

SMC_RESULT BFCL::readAccess() {
	//TODO: implement this
	data[0] = 100;
	return SmcSuccess;
}

SMC_RESULT BNum::readAccess() {
	data[0] = BatteryManager::getShared()->batteriesCount;
	return SmcSuccess;
}

SMC_RESULT BSIn::readAccess() {
	enum {
		BSInCharging          = 1,
		BSInACPresent         = 2,
		BSInACPresenceChanged = 4,
		BSInOSStopCharge      = 8,
		BSInOSCalibrationReq  = 16,
		BSInBTQueryInProgress = 32,
		BSInBTOk              = 64,
		BSInAdcInProgress     = 128
	};

	data[0] = BSInBTOk;
	IOSimpleLockLock(BatteryManager::getShared()->stateLock);
	if (BatteryManager::getShared()->externalPowerConnected()) {
		if (!BatteryManager::getShared()->batteriesAreFull())
			data[0] |= BSInCharging;
		data[0] |= BSInACPresent;
	}
	IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);
	return SmcSuccess;
}

SMC_RESULT BRSC::readAccess() {
	// TODO: what's with multiple batteries?
	data[0] = 0;
	IOSimpleLockLock(BatteryManager::getShared()->stateLock);
	if (BatteryManager::getShared()->batteriesCount > 0 &&
		BatteryManager::getShared()->state.btInfo[0].connected &&
		BatteryManager::getShared()->state.btInfo[0].state.lastFullChargeCapacity > 0 &&
		BatteryManager::getShared()->state.btInfo[0].state.lastFullChargeCapacity != BatteryInfo::ValueUnknown &&
		BatteryManager::getShared()->state.btInfo[0].state.lastFullChargeCapacity <= BatteryInfo::ValueMax)
		data[1] = BatteryManager::getShared()->state.btInfo[0].state.remainingCapacity * 100 / BatteryManager::getShared()->state.btInfo[0].state.lastFullChargeCapacity;
	else
		data[1] = 0;
	IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);
	return SmcSuccess;
}

SMC_RESULT CHLC::readAccess() {
	// TODO: does it have any other values?
	IOSimpleLockLock(BatteryManager::getShared()->stateLock);
	if (BatteryManager::getShared()->batteriesCount > 0 &&
		BatteryManager::getShared()->state.btInfo[0].connected &&
		BatteryManager::getShared()->state.btInfo[0].state.lastFullChargeCapacity > 0 &&
		BatteryManager::getShared()->state.btInfo[0].state.lastFullChargeCapacity != BatteryInfo::ValueUnknown &&
		BatteryManager::getShared()->state.btInfo[0].state.lastFullChargeCapacity <= BatteryInfo::ValueMax &&
		BatteryManager::getShared()->state.btInfo[0].state.remainingCapacity > 0 &&
		BatteryManager::getShared()->state.btInfo[0].state.remainingCapacity != BatteryInfo::ValueUnknown &&
		BatteryManager::getShared()->state.btInfo[0].state.remainingCapacity <= BatteryInfo::ValueMax &&
		BatteryManager::getShared()->state.btInfo[0].state.remainingCapacity >= BatteryManager::getShared()->state.btInfo[0].state.lastFullChargeCapacity)
		data[0] = 2;
	else
		data[0] = 1;
	IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);
	return SmcSuccess;
}
