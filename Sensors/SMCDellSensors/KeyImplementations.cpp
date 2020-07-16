//
//  KeyImplementations.cpp
//  SMCDellSensors
//
//  Copyright © 2020 lsv1974. All rights reserved.
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

static inline UInt16 decode_fpe2(UInt16 value) {
	return (swap_value(value) >> 2);
}

static int setTargetSpeed(size_t index, UInt16 value) {
	IOLockLock(SMIMonitor::getShared()->mainLock);
	int status = 1;
	int range = SMIMonitor::getShared()->state.fanInfo[index].maxSpeed - SMIMonitor::getShared()->state.fanInfo[index].minSpeed;
	if (value > SMIMonitor::getShared()->state.fanInfo[index].minSpeed + range/2)
		status = 2;
	int rc = SMIMonitor::getShared()->i8k_set_fan(SMIMonitor::getShared()->state.fanInfo[index].index, status);
	if (rc != 0)
		SYSLOG("sdell", "Set target speed for fan %d to %d failed: %d", index, value, rc);
	IOLockUnlock(SMIMonitor::getShared()->mainLock);
	return rc;
}

SMC_RESULT F0Ac::readAccess() {
	UInt16 value = SMIMonitor::getShared()->state.fanInfo[index].speed;
	//*reinterpret_cast<uint16_t *>(data) = VirtualSMCAPI::encodeFp(SmcKeyTypeFpe2, val);
	*reinterpret_cast<uint16_t *>(data) = encode_fpe2(value);
	return SmcSuccess;
}

SMC_RESULT F0Mn::readAccess() {
	UInt16 value = SMIMonitor::getShared()->state.fanInfo[index].minSpeed;
	//*reinterpret_cast<uint16_t *>(data) = VirtualSMCAPI::encodeFp(SmcKeyTypeFpe2, val);
	*reinterpret_cast<uint16_t *>(data) = encode_fpe2(value);
	return SmcSuccess;
}

SMC_RESULT F0Mn::update(const SMC_DATA *src) {
	SMIIdxKey::update(src);
	UInt16 value = decode_fpe2(*reinterpret_cast<const uint16_t *>(src));
	SYSLOG("sdell", "Set new minimum speed for fan %d to %d", index, value);
	return SmcSuccess;
}

SMC_RESULT F0Mx::readAccess() {
	UInt16 value = SMIMonitor::getShared()->state.fanInfo[index].maxSpeed;
	//*reinterpret_cast<uint16_t *>(data) = VirtualSMCAPI::encodeFp(SmcKeyTypeFpe2, val);
	*reinterpret_cast<uint16_t *>(data) = encode_fpe2(value);
	return SmcSuccess;
}

SMC_RESULT F0Mx::update(const SMC_DATA *src) {
	SMIIdxKey::update(src);
	UInt16 value = decode_fpe2(*reinterpret_cast<const uint16_t *>(src));
	SYSLOG("sdell", "Set new maximum speed for fan %d to %d", index, value);
	return SmcSuccess;
}

SMC_RESULT F0Md::readAccess() {
	UInt16 val = (SMIMonitor::getShared()->fansStatus & (1 << index)) >> index;
	*reinterpret_cast<uint8_t *>(data) = val;
	return SmcSuccess;
}

SMC_RESULT F0Md::update(const SMC_DATA *src) {
	UInt16 val = src[0];
	DBGLOG("sdell", "Set manual mode for fan %d to %d", index, val);
	
	int rc = 0;
	if (val != (SMIMonitor::getShared()->fansStatus & (1 << index))>>index) {
		IOLockLock(SMIMonitor::getShared()->mainLock);
		rc = val ? SMIMonitor::getShared()->i8k_set_fan_control_manual(SMIMonitor::getShared()->state.fanInfo[index].index) :
					SMIMonitor::getShared()->i8k_set_fan_control_auto(SMIMonitor::getShared()->state.fanInfo[index].index);
		IOLockUnlock(SMIMonitor::getShared()->mainLock);
	}
	if (rc == 0) {
		SMIMonitor::getShared()->fansStatus = val ? (SMIMonitor::getShared()->fansStatus | (1 << index)) :
													(SMIMonitor::getShared()->fansStatus & ~(1 << index));
		DBGLOG("sdell", "Set manual mode for fan %d to %d, fansStatus = 0x%02x", index, val, SMIMonitor::getShared()->fansStatus);
	}
	else
		SYSLOG("sdell", "Set manual mode for fan %d to %d failed: %d", index, val, rc);
	
	return SmcSuccess;
}

SMC_RESULT F0Tg::readAccess() {
	UInt16 value = SMIMonitor::getShared()->state.fanInfo[index].targetSpeed;
	//*reinterpret_cast<uint16_t *>(data) = VirtualSMCAPI::encodeFp(SmcKeyTypeFpe2, val);
	*reinterpret_cast<uint16_t *>(data) = encode_fpe2(value);
	return SmcSuccess;
}

SMC_RESULT F0Tg::update(const SMC_DATA *src) {
	UInt16 value = decode_fpe2(*reinterpret_cast<const uint16_t *>(src));
	DBGLOG("sdell", "Set target speed for fan %d to %d", index, value);
	SMIMonitor::getShared()->state.fanInfo[index].targetSpeed = value;

	if (SMIMonitor::getShared()->fansStatus & (1 << index))
		setTargetSpeed(index, value);
	else
		SYSLOG("sdell", "Set target speed for fan %d to %d ignored since auto control is active", index, value);

	return SmcSuccess;
}

SMC_RESULT FS__::readAccess() {
	size_t value = SMIMonitor::getShared()->fansStatus;
	UInt8 *bytes = reinterpret_cast<uint8_t *>(data);
	bytes[0] = value >> 8;
	bytes[1] = value & 0xFF;
	return SmcSuccess;
}

SMC_RESULT FS__::update(const SMC_DATA *src) {
	UInt16 val = (src[0] << 8) + src[1]; //big endian data
	DBGLOG("sdell", "Set force fan mode to %d", val);
	
	IOLockLock(SMIMonitor::getShared()->mainLock);
	int rc = 0;
	for (int i = 0; i < SMIMonitor::getShared()->fanCount; i++) {
		if ((val & (1 << i)) != (SMIMonitor::getShared()->fansStatus & (1 << i))) {
			rc |= (val & (1 << i)) ? SMIMonitor::getShared()->i8k_set_fan_control_manual(SMIMonitor::getShared()->state.fanInfo[i].index) :
									 SMIMonitor::getShared()->i8k_set_fan_control_auto(SMIMonitor::getShared()->state.fanInfo[i].index);
		}
	}

	if (rc == 0)
		SMIMonitor::getShared()->fansStatus = val;
	else
		SYSLOG("sdell", "Set force fan mode to %d failed: %d", val, rc);
	IOLockUnlock(SMIMonitor::getShared()->mainLock);
	
	return SmcSuccess;
}

SMC_RESULT TC0P::readAccess() {
	double val = SMIMonitor::getShared()->state.tempInfo[index].temp;
	*reinterpret_cast<uint16_t *>(data) = VirtualSMCAPI::encodeSp(SmcKeyTypeSp78, val);
	return SmcSuccess;
}

SMC_RESULT TG0P::readAccess() {
	double val = SMIMonitor::getShared()->state.tempInfo[index].temp;
	*reinterpret_cast<uint16_t *>(data) = VirtualSMCAPI::encodeSp(SmcKeyTypeSp78, val);
	return SmcSuccess;
}

SMC_RESULT Tm0P::readAccess() {
	double val = SMIMonitor::getShared()->state.tempInfo[index].temp;
	*reinterpret_cast<uint16_t *>(data) = VirtualSMCAPI::encodeSp(SmcKeyTypeSp78, val);
	return SmcSuccess;
}

SMC_RESULT TN0P::readAccess() {
	double val = SMIMonitor::getShared()->state.tempInfo[index].temp;
	*reinterpret_cast<uint16_t *>(data) = VirtualSMCAPI::encodeSp(SmcKeyTypeSp78, val);
	return SmcSuccess;
}

SMC_RESULT TA0P::readAccess() {
	double val = SMIMonitor::getShared()->state.tempInfo[index].temp;
	*reinterpret_cast<uint16_t *>(data) = VirtualSMCAPI::encodeSp(SmcKeyTypeSp78, val);
	return SmcSuccess;
}

SMC_RESULT TW0P::readAccess() {
	double val = SMIMonitor::getShared()->state.tempInfo[index].temp;
	*reinterpret_cast<uint16_t *>(data) = VirtualSMCAPI::encodeSp(SmcKeyTypeSp78, val);
	return SmcSuccess;
}
