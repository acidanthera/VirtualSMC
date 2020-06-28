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
	uint8_t *ptr = reinterpret_cast<uint8_t *>(data);
	IOSimpleLockLock(SMIMonitor::getShared()->stateLock);
	*ptr = SMIMonitor::getShared()->state.fanInfo[index].status;
	IOSimpleLockUnlock(SMIMonitor::getShared()->stateLock);
	return SmcSuccess;
}

SMC_RESULT F0As::update(const SMC_DATA *src) {
	VirtualSMCValue::update(src);
	UInt8 status = *reinterpret_cast<const UInt8*>(src);
	status = status & 3; //restrict possible values to 0,1,2,3 {off, low, high}
	IOLockLock(SMIMonitor::getShared()->mainLock);
	IOSimpleLockLock(SMIMonitor::getShared()->stateLock);
	int ret = SMIMonitor::getShared()->i8k_set_fan(SMIMonitor::getShared()->state.fanInfo[index].index, status);
	IOSimpleLockUnlock(SMIMonitor::getShared()->stateLock);
	IOLockUnlock(SMIMonitor::getShared()->mainLock);
	return (ret == status) ? SmcSuccess : SmcError;
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

SMC_RESULT F0Md::readAccess() {
	IOSimpleLockLock(SMIMonitor::getShared()->stateLock);
	UInt16 val = (SMIMonitor::getShared()->fansStatus & (1 << index)) >> index;
	IOSimpleLockUnlock(SMIMonitor::getShared()->stateLock);
	*reinterpret_cast<uint8_t *>(data) = val;
	return SmcSuccess;
}

SMC_RESULT F0Md::update(const SMC_DATA *src) {
	VirtualSMCValue::update(src);
	IOLockLock(SMIMonitor::getShared()->mainLock);
	IOSimpleLockLock(SMIMonitor::getShared()->stateLock);
	
	UInt16 val = src[0];
	int rc = 0;
	if (val != (SMIMonitor::getShared()->fansStatus & (1 << index))>>index) {
		rc |= val ? SMIMonitor::getShared()->i8k_set_fan_control_manual(SMIMonitor::getShared()->state.fanInfo[index].index) :
					SMIMonitor::getShared()->i8k_set_fan_control_auto(SMIMonitor::getShared()->state.fanInfo[index].index);
	}
	if (rc == 0)
		SMIMonitor::getShared()->fansStatus = val ? (SMIMonitor::getShared()->fansStatus | (1 << index)) :
													(SMIMonitor::getShared()->fansStatus & ~(1 << index));
	IOSimpleLockUnlock(SMIMonitor::getShared()->stateLock);
	IOLockUnlock(SMIMonitor::getShared()->mainLock);
	return (rc == 0) ? SmcSuccess : SmcError;
}

SMC_RESULT FFRC::readAccess() {
	IOSimpleLockLock(SMIMonitor::getShared()->stateLock);
	size_t value = SMIMonitor::getShared()->fansStatus;
	IOSimpleLockUnlock(SMIMonitor::getShared()->stateLock);
	UInt8 *bytes = reinterpret_cast<uint8_t *>(data);
	bytes[0] = value >> 8;
	bytes[1] = value & 0xFF;
	return SmcSuccess;
}

SMC_RESULT FFRC::update(const SMC_DATA *src) {
	VirtualSMCValue::update(src);
	UInt16 val = (src[0] << 8) + src[1]; //big endian data
	IOLockLock(SMIMonitor::getShared()->mainLock);
	IOSimpleLockLock(SMIMonitor::getShared()->stateLock);
	
	int rc = 0;
	for (int i = 0; i < SMIMonitor::getShared()->fanCount; i++) {
		if ((val & (1 << i)) != (SMIMonitor::getShared()->fansStatus & (1 << i))) {
			rc |= (val & (1 << i)) ? SMIMonitor::getShared()->i8k_set_fan_control_manual(SMIMonitor::getShared()->state.fanInfo[i].index) :
									 SMIMonitor::getShared()->i8k_set_fan_control_auto(SMIMonitor::getShared()->state.fanInfo[i].index);
		}
	}

	if (rc == 0)
		SMIMonitor::getShared()->fansStatus = val;
	IOSimpleLockUnlock(SMIMonitor::getShared()->stateLock);
	IOLockUnlock(SMIMonitor::getShared()->mainLock);
	
	return (rc == 0) ? SmcSuccess : SmcError;
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
