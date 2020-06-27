//
//  KeyImplementations.cpp
//  SMCBatteryManager
//
//  Copyright © 2018 usrsse2. All rights reserved.
//

#include "KeyImplementations.hpp"
#include "SMCDellSensors.hpp"

static inline UInt16 swap_value(UInt16 value)
{
	return ((value & 0xff00) >> 8) | ((value & 0xff) << 8);
}
static inline UInt16 encode_fpe2(UInt16 value) {
	return swap_value(value << 2);
}

SMC_RESULT F0Ac::readAccess() {
	IOSimpleLockLock(SMIMonitor::getShared()->stateLock);
	UInt16 value = SMIMonitor::getShared()->state.fanInfo[index].speed;
	IOSimpleLockUnlock(SMIMonitor::getShared()->stateLock);
	//*reinterpret_cast<uint16_t *>(data) = VirtualSMCAPI::encodeFp(SmcKeyTypeFpe2, val);
	*reinterpret_cast<uint16_t *>(data) = encode_fpe2(value);
	return SmcSuccess;
}

SMC_RESULT F0As::readAccess() {
	int16_t *ptr = reinterpret_cast<int16_t *>(data);
	IOSimpleLockLock(SMIMonitor::getShared()->stateLock);
	*ptr = OSSwapHostToBigInt16(SMIMonitor::getShared()->state.fanInfo[index].status);
	IOSimpleLockUnlock(SMIMonitor::getShared()->stateLock);
	return SmcSuccess;
}

SMC_RESULT F0Mn::readAccess() {
	IOSimpleLockLock(SMIMonitor::getShared()->stateLock);
	UInt16 value = SMIMonitor::getShared()->state.fanInfo[index].minSpeed;
	IOSimpleLockUnlock(SMIMonitor::getShared()->stateLock);
	//*reinterpret_cast<uint16_t *>(data) = VirtualSMCAPI::encodeFp(SmcKeyTypeFpe2, val);
	*reinterpret_cast<uint16_t *>(data) = encode_fpe2(value);
	return SmcSuccess;
}

SMC_RESULT F0Mx::readAccess() {
	IOSimpleLockLock(SMIMonitor::getShared()->stateLock);
	UInt16 value = SMIMonitor::getShared()->state.fanInfo[index].maxSpeed;
	//*reinterpret_cast<uint16_t *>(data) = VirtualSMCAPI::encodeFp(SmcKeyTypeFpe2, val);
	IOSimpleLockUnlock(SMIMonitor::getShared()->stateLock);
	*reinterpret_cast<uint16_t *>(data) = encode_fpe2(value);
	return SmcSuccess;
}

SMC_RESULT TC0P::readAccess() {
	IOSimpleLockLock(SMIMonitor::getShared()->stateLock);
	double val = SMIMonitor::getShared()->state.tempInfo[index].temp;
	*reinterpret_cast<uint16_t *>(data) = VirtualSMCAPI::encodeSp(SmcKeyTypeSp78, val);
	IOSimpleLockUnlock(SMIMonitor::getShared()->stateLock);
	return SmcSuccess;
}

SMC_RESULT TG0P::readAccess() {
	IOSimpleLockLock(SMIMonitor::getShared()->stateLock);
	double val = SMIMonitor::getShared()->state.tempInfo[index].temp;
	*reinterpret_cast<uint16_t *>(data) = VirtualSMCAPI::encodeSp(SmcKeyTypeSp78, val);
	IOSimpleLockUnlock(SMIMonitor::getShared()->stateLock);
	return SmcSuccess;
}

SMC_RESULT Tm0P::readAccess() {
	IOSimpleLockLock(SMIMonitor::getShared()->stateLock);
	double val = SMIMonitor::getShared()->state.tempInfo[index].temp;
	*reinterpret_cast<uint16_t *>(data) = VirtualSMCAPI::encodeSp(SmcKeyTypeSp78, val);
	IOSimpleLockUnlock(SMIMonitor::getShared()->stateLock);
	return SmcSuccess;
}

SMC_RESULT TN0P::readAccess() {
	IOSimpleLockLock(SMIMonitor::getShared()->stateLock);
	double val = SMIMonitor::getShared()->state.tempInfo[index].temp;
	*reinterpret_cast<uint16_t *>(data) = VirtualSMCAPI::encodeSp(SmcKeyTypeSp78, val);
	IOSimpleLockUnlock(SMIMonitor::getShared()->stateLock);
	return SmcSuccess;
}

SMC_RESULT TA0P::readAccess() {
	IOSimpleLockLock(SMIMonitor::getShared()->stateLock);
	double val = SMIMonitor::getShared()->state.tempInfo[index].temp;
	*reinterpret_cast<uint16_t *>(data) = VirtualSMCAPI::encodeSp(SmcKeyTypeSp78, val);
	IOSimpleLockUnlock(SMIMonitor::getShared()->stateLock);
	return SmcSuccess;
}

SMC_RESULT TW0P::readAccess() {
	IOSimpleLockLock(SMIMonitor::getShared()->stateLock);
	double val = SMIMonitor::getShared()->state.tempInfo[index].temp;
	*reinterpret_cast<uint16_t *>(data) = VirtualSMCAPI::encodeSp(SmcKeyTypeSp78, val);
	IOSimpleLockUnlock(SMIMonitor::getShared()->stateLock);
	return SmcSuccess;
}
