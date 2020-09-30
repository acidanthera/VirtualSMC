//
//  SuperIODevice.cpp
//
//  SuperIO Chip data
//
//  Based on https://github.com/kozlek/HWSensors/blob/master/SuperIOSensors/SuperIODevice.cpp
//  @author joedm
//

#include <VirtualSMCSDK/kern_vsmcapi.hpp>

#include "SMCSuperIO.hpp"
#include "SuperIODevice.hpp"

void SuperIODevice::update() {
	updateTachometers();
	updateVoltages();
	updateIORegistry();
}

void SuperIODevice::updateIORegistry() {
	auto obj = OSDynamicCast(IORegistryEntry, getSmcSuperIO());
	if (obj) {
		OSDictionary *dict = OSDictionary::withCapacity(32);
		for (uint8_t index = 0; index < getTachometerCount(); ++index) {
			uint16_t value = getTachometerValue(index);
			auto key = OSString::withCString(getTachometerName(index));
			auto val = OSNumber::withNumber(value, 16);
			dict->setObject(key, val);
			val->release();
			key->release();
		}
		for (uint8_t index = 0; index < getVoltageCount(); ++index) {
			float value = getVoltageValue(index);
			auto key = OSString::withCString(getVoltageName(index));
			auto data = OSData::withBytes(reinterpret_cast<const void*>(&value), sizeof(value));
			dict->setObject(key, data);
			data->release();
			key->release();
		}
		obj->setProperty("Sensors", dict);
		dict->release();
	}
}

/**
 *  Keys
 */
SMC_RESULT TachometerKey::readAccess() {
	double val = device->getTachometerValue(index);
	const_cast<SMCSuperIO*>(sio)->quickReschedule();
	*reinterpret_cast<uint16_t *>(data) = VirtualSMCAPI::encodeIntFp(SmcKeyTypeFpe2, val);
	return SmcSuccess;
}
