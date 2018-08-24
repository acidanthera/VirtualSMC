//
//  kern_keyvalue.cpp
//  VirtualSMC
//
//  Copyright © 2017 vit9696. All rights reserved.
//

#include <Headers/kern_iokit.hpp>
#include <Headers/kern_util.hpp>
#include <VirtualSMCSDK/kern_value.hpp>

bool VirtualSMCValue::init(const SMC_DATA *d, SMC_DATA_SIZE sz, SMC_KEY_TYPE t, SMC_KEY_ATTRIBUTES a, SerializeLevel s) {
	if (sz <= SMC_MAX_DATA_SIZE) {
		if (d) lilu_os_memcpy(data, d, sz);
		size = sz;
		type = t;
		attr = a;
		serializeLevel = s;
		return true;
	}
	
	DBGLOG("value", "data size too large");
	return false;
}

bool VirtualSMCValue::init(const OSDictionary *dict) {
	if (!dict) {
		DBGLOG("value", "null dictionary");
		return false;
	}
	
	auto value = OSDynamicCast(OSData, dict->getObject("value"));
	if ((!value && !WIOKit::getOSDataValue(dict, "size", size)) ||
		!WIOKit::getOSDataValue(dict, "type", type) ||
		!WIOKit::getOSDataValue(dict, "attr", attr)) {
		DBGLOG("value", "mandatory data missing in dictionary");
		return false;
	}
	
	if (size == 0 && value)
		size = value->getLength();
	
	if (size > SMC_MAX_DATA_SIZE) {
		DBGLOG("value", "data length %u exceeds max %u", size, SMC_MAX_DATA_SIZE);
		return false;
	}
	
	if (size > 0 && value)
		lilu_os_memcpy(data, value->getBytesNoCopy(), size);
	
	auto ser = OSDynamicCast(OSBoolean, dict->getObject("serialize"));
	serializeLevel = ser && ser->isTrue() ? SerializeLevel::Normal : SerializeLevel::None;
	
	return true;
}

const SMC_DATA *VirtualSMCValue::get(SMC_DATA_SIZE &sz) const {
	sz = size;
	return data;
}

SMC_RESULT VirtualSMCValue::update(const SMC_DATA *src) {
	lilu_os_memcpy(data, src, size);
	return SmcSuccess;
}
