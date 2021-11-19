//
//  KeyImplementations.cpp
//  SMCDellSensors
//
//  Copyright © 2020 lsv1974. All rights reserved.
//

#include "KeyImplementations.hpp"
#include "SMCDellSensors.hpp"

SMC_RESULT F0Ac::readAccess() {
	auto value = SMIMonitor::getShared()->state.fanInfo[index].speed;
	*reinterpret_cast<uint16_t *>(data) = VirtualSMCAPI::encodeIntFp(SmcKeyTypeFpe2, value);
	return SmcSuccess;
}

SMC_RESULT F0Mn::readAccess() {
	UInt16 value = SMIMonitor::getShared()->state.fanInfo[index].minSpeed;
	*reinterpret_cast<uint16_t *>(data) = VirtualSMCAPI::encodeIntFp(SmcKeyTypeFpe2, value);
	return SmcSuccess;
}

SMC_RESULT F0Mn::update(const SMC_DATA *src) {
	SMIIdxKey::update(src);
	DBGLOG("sdell", "Set new minimum speed for fan %d to %d", index, VirtualSMCAPI::decodeIntFp(SmcKeyTypeFpe2, *reinterpret_cast<const uint16_t *>(src)));
	return SmcSuccess;
}

SMC_RESULT F0Mx::readAccess() {
	auto value = SMIMonitor::getShared()->state.fanInfo[index].maxSpeed;
	*reinterpret_cast<uint16_t *>(data) = VirtualSMCAPI::encodeIntFp(SmcKeyTypeFpe2, value);
	return SmcSuccess;
}

SMC_RESULT F0Mx::update(const SMC_DATA *src) {
	SMIIdxKey::update(src);
	DBGLOG("sdell", "Set new maximum speed for fan %d to %d", index, VirtualSMCAPI::decodeIntFp(SmcKeyTypeFpe2, *reinterpret_cast<const uint16_t *>(src)));
	return SmcSuccess;
}

SMC_RESULT F0Md::readAccess() {
	auto val = (SMIMonitor::getShared()->fansStatus & (1 << index)) >> index;
	*reinterpret_cast<uint8_t *>(data) = val;
	return SmcSuccess;
}

SMC_RESULT F0Md::update(const SMC_DATA *src) {
	SMIIdxKey::update(src);
	SMIMonitor::getShared()->postSmcUpdate(KeyF0Md, index, src, sizeof(UInt8));
	return SmcSuccess;
}

SMC_RESULT F0Tg::readAccess() {
	auto value = SMIMonitor::getShared()->state.fanInfo[index].targetSpeed;
	*reinterpret_cast<uint16_t *>(data) = VirtualSMCAPI::encodeIntFp(SmcKeyTypeFpe2, value);
	return SmcSuccess;
}

SMC_RESULT F0Tg::update(const SMC_DATA *src) {
	SMIIdxKey::update(src);
	SMIMonitor::getShared()->postSmcUpdate(KeyF0Tg, index, src, sizeof(UInt16));
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
	SMIKey::update(src);
	SMIMonitor::getShared()->postSmcUpdate(KeyFS__, -1, src, sizeof(UInt16));
	return SmcSuccess;
}

SMC_RESULT TG0P::readAccess() {
	auto val = SMIMonitor::getShared()->state.tempInfo[index].temp;
	*reinterpret_cast<uint16_t *>(data) = VirtualSMCAPI::encodeIntSp(SmcKeyTypeSp78, val);
	return SmcSuccess;
}

SMC_RESULT Tm0P::readAccess() {
	auto val = SMIMonitor::getShared()->state.tempInfo[index].temp;
	*reinterpret_cast<uint16_t *>(data) = VirtualSMCAPI::encodeIntSp(SmcKeyTypeSp78, val);
	return SmcSuccess;
}

SMC_RESULT TN0P::readAccess() {
	auto val = SMIMonitor::getShared()->state.tempInfo[index].temp;
	*reinterpret_cast<uint16_t *>(data) = VirtualSMCAPI::encodeIntSp(SmcKeyTypeSp78, val);
	return SmcSuccess;
}

SMC_RESULT TA0P::readAccess() {
	auto val = SMIMonitor::getShared()->state.tempInfo[index].temp;
	*reinterpret_cast<uint16_t *>(data) = VirtualSMCAPI::encodeIntSp(SmcKeyTypeSp78, val);
	return SmcSuccess;
}

SMC_RESULT TW0P::readAccess() {
	auto val = SMIMonitor::getShared()->state.tempInfo[index].temp;
	*reinterpret_cast<uint16_t *>(data) = VirtualSMCAPI::encodeIntSp(SmcKeyTypeSp78, val);
	return SmcSuccess;
}
