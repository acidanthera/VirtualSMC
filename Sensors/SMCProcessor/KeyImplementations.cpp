//
//  KeyImplementations.cpp
//  SMCProcessor
//
//  Copyright © 2018 vit9696. All rights reserved.
//

#include "KeyImplementations.hpp"
#include "SMCProcessor.hpp"

SMC_RESULT TempPackage::readAccess() {
	uint16_t *ptr = reinterpret_cast<uint16_t *>(data);
	IOSimpleLockLock(cp->counterLock);
	*ptr = VirtualSMCAPI::encodeIntSp(type, cp->counters.tjmax[package] - cp->counters.thermalStatusPackage[package]);
	cp->quickReschedule();
	IOSimpleLockUnlock(cp->counterLock);
	return SmcSuccess;
}

SMC_RESULT TempCore::readAccess() {
	uint16_t *ptr = reinterpret_cast<uint16_t *>(data);
	IOSimpleLockLock(cp->counterLock);
	*ptr = VirtualSMCAPI::encodeIntSp(type, cp->counters.tjmax[package] - cp->counters.thermalStatus[core]);
	cp->quickReschedule();
	IOSimpleLockUnlock(cp->counterLock);
	return SmcSuccess;
}

SMC_RESULT VoltagePackage::readAccess() {
	uint16_t *ptr = reinterpret_cast<uint16_t *>(data);
	IOSimpleLockLock(cp->counterLock);
	*ptr = VirtualSMCAPI::encodeSp(type, cp->counters.voltage[package]);
	cp->quickReschedule();
	IOSimpleLockUnlock(cp->counterLock);
	return SmcSuccess;
}

SMC_RESULT CpEnergyKey::readAccess() {
	IOSimpleLockLock(cp->counterLock);
	float val = cp->counters.power[0][index];
	for (size_t i = 1; i < cp->cpuTopology.packageCount; i++)
		val += cp->counters.power[i][index];
	if (type == SmcKeyTypeFloat)
		*reinterpret_cast<uint32_t *>(data) = VirtualSMCAPI::encodeFlt(val);
	else
		*reinterpret_cast<uint16_t *>(data) = VirtualSMCAPI::encodeSp(type, val);
	cp->quickReschedule();
	IOSimpleLockUnlock(cp->counterLock);
	return SmcSuccess;
}
