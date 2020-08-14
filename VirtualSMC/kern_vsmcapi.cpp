//
//  kern_vsmcapi.cpp
//  VirtualSMC
//
//  Copyright © 2018 vit9696. All rights reserved.
//

#include <VirtualSMCSDK/kern_vsmcapi.hpp>
#include <Headers/kern_util.hpp>
#include "kern_vsmc.hpp"

IONotifier *VirtualSMCAPI::registerHandler(IOServiceMatchingNotificationHandler handler, void *context) {
	auto vsmcMatching = IOService::nameMatching(ServiceName);
	if (vsmcMatching) {
		auto vsmcNotifier = IOService::addMatchingNotification(gIOFirstPublishNotification, vsmcMatching, handler, context);
		vsmcMatching->release();
		if (vsmcNotifier) {
			DBGLOG("vsmcapi", "created vsmc notifier");
			return vsmcNotifier;
		} else {
			SYSLOG("vsmcapi", "failed to create vsmc notifier");
		}
	} else {
		SYSLOG("vsmcapi", "failed to allocate vsmc matching dict");
	}

	return nullptr;
}

bool VirtualSMCAPI::postInterrupt(SMC_EVENT_CODE code, const void *data, uint32_t dataSize) {
	return VirtualSMC::postInterrupt(code, data, dataSize);
}

bool VirtualSMCAPI::getDeviceInfo(SMCInfo &info) {
	if (!VirtualSMC::isServicingReady())
		return false;
	info = VirtualSMC::getKeystore()->getDeviceInfo();
	return true;
}

bool VirtualSMCAPI::addKey(SMC_KEY key, VirtualSMCAPI::KeyStorage &data, VirtualSMCValue *val) {
	if (val) {
		if (data.push_back<4>(VirtualSMCKeyValue::create(key, val))) {
			DBGLOG("vsmcapi", "inserted key [%08X]", key);
			return true;
		} else {
			DBGLOG("vsmcapi", "failed to insert key [%08X]", key);
			delete val;
		}
	} else {
		DBGLOG("vsmcapi", "no value given for key [%08X]", key);
	}
	
	return false;
}

VirtualSMCValue *VirtualSMCAPI::valueWithData(const SMC_DATA *smcData, SMC_DATA_SIZE smcDataSize, SMC_KEY_TYPE smcKeyType, VirtualSMCValue *thisValue, SMC_KEY_ATTRIBUTES smcKeyAttrs, SerializeLevel serializeLevel) {
	if (smcDataSize == 0) {
		DBGLOG("vsmcapi", "invalid SMC_DATA size");
		return nullptr;
	}
	if (!thisValue) {
		thisValue = new VirtualSMCValue();
		if (!thisValue) {
			DBGLOG("vsmcapi", "alloc failure for a key value, see subsequent error messages");
			return nullptr;
		}
	}
	if (!thisValue->init(smcData, smcDataSize, smcKeyType, smcKeyAttrs, serializeLevel)) {
		SYSLOG("vsmcapi", "failed to init value, see subsequent error messages");
		delete thisValue;
		return nullptr;
	}
	return thisValue;
}

static uint32_t getSpIntegral(uint32_t type) {
	switch (type) {
		case SmcKeyTypeSp1e:
			return 0x1;
		case SmcKeyTypeSp2d:
			return 0x2;
		case SmcKeyTypeSp3c:
			return 0x3;
		case SmcKeyTypeSp4b:
			return 0x4;
		case SmcKeyTypeSp5a:
			return 0x5;
		case SmcKeyTypeSp69:
			return 0x6;
		case SmcKeyTypeSp78:
			return 0x7;
		case SmcKeyTypeSp87:
			return 0x8;
		case SmcKeyTypeSp96:
			return 0x9;
		case SmcKeyTypeSpa5:
			return 0xa;
		case SmcKeyTypeSpb4:
			return 0xb;
		case SmcKeyTypeSpc3:
			return 0xc;
		case SmcKeyTypeSpd2:
			return 0xd;
		case SmcKeyTypeSpe1:
			return 0xe;
		case SmcKeyTypeSpf0:
			return 0xf;
		default:
			return 0;
	}
}

static uint32_t getFpIntegral(uint32_t type) {
	switch (type) {
		case SmcKeyTypeFp1f:
			return 0x1;
		case SmcKeyTypeFp2e:
			return 0x2;
		case SmcKeyTypeFp3d:
			return 0x3;
		case SmcKeyTypeFp4c:
			return 0x4;
		case SmcKeyTypeFp5b:
			return 0x5;
		case SmcKeyTypeFp6a:
			return 0x6;
		case SmcKeyTypeFp79:
			return 0x7;
		case SmcKeyTypeFp88:
			return 0x8;
		case SmcKeyTypeFp97:
			return 0x9;
		case SmcKeyTypeFpa6:
			return 0xa;
		case SmcKeyTypeFpb5:
			return 0xb;
		case SmcKeyTypeFpc4:
			return 0xc;
		case SmcKeyTypeFpd3:
			return 0xd;
		case SmcKeyTypeFpe2:
			return 0xe;
		case SmcKeyTypeFpf1:
			return 0xf;
		default:
			return 0;
	}
}

double VirtualSMCAPI::decodeSp(uint32_t type, uint16_t value) {
	uint32_t integral = getSpIntegral(type);
	if (integral == 0)
		return 0;
	value = OSSwapInt16(value);
	double ret = (value & 0x7FFF) / static_cast<double>(getBit<uint16_t>(15 - integral));
	return (value & 0x8000) ? -ret : ret;
}

uint16_t VirtualSMCAPI::encodeSp(uint32_t type, double value) {
	uint32_t integral = getSpIntegral(type);
	if (integral == 0)
		return 0;
	uint16_t ret = static_cast<uint16_t>(__builtin_fabs(value) * getBit<uint16_t>(15 - integral)) & 0x7FFF;
	return OSSwapInt16(value < 0 ? (ret | 0x8000) : ret);
}

double VirtualSMCAPI::decodeFp(uint32_t type, uint16_t value) {
	uint32_t integral = getFpIntegral(type);
	if (integral == 0)
		return 0;
	value = OSSwapInt16(value);
	double ret = value / static_cast<double>(getBit<uint16_t>(16 - integral));
	return ret;
}

uint16_t VirtualSMCAPI::encodeFp(uint32_t type, double value) {
	uint32_t integral = getFpIntegral(type);
	if (integral == 0)
		return 0;
	uint16_t ret = static_cast<uint16_t>(__builtin_fabs(value) * getBit<uint16_t>(16 - integral));
	return OSSwapInt16(ret);
}

int16_t VirtualSMCAPI::decodeIntSp(uint32_t type, uint16_t value) {
	uint32_t integral = getSpIntegral(type);
	if (integral == 0)
		return 0;
	value = OSSwapInt16(value);
	int16_t ret = (value & 0x7FFF) >> (15U - integral);
	return (value & 0x8000) ? -ret : ret;
}

uint16_t VirtualSMCAPI::encodeIntSp(uint32_t type, int16_t value) {
	uint32_t integral = getSpIntegral(type);
	if (integral == 0)
		return 0;
	uint16_t ret = static_cast<uint16_t>(__builtin_abs(value) << (15U - integral)) & 0x7FFF;
	return OSSwapInt16(value < 0 ? (ret | 0x8000) : ret);
}

uint16_t VirtualSMCAPI::decodeIntFp(uint32_t type, uint16_t value) {
	uint32_t integral = getFpIntegral(type);
	if (integral == 0)
		return 0;
	value = OSSwapInt16(value);
	int16_t ret = value >> (16U - integral);
	return ret;
}

uint16_t VirtualSMCAPI::encodeIntFp(uint32_t type, uint16_t value) {
	uint32_t integral = getFpIntegral(type);
	if (integral == 0)
		return 0;
	uint16_t ret = static_cast<uint16_t>(value << (16U - integral));
	return OSSwapInt16(ret);
}
