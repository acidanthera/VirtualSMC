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
	updateTemperatures();
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
		for (uint8_t index = 0; index < getTemperatureCount(); ++index) {
			float value = getTemperatureValue(index);
			auto key = OSString::withCString(getTemperatureName(index));
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

SMC_RESULT VoltageKey::readAccess() {
	double val = device->getVoltageValue(index);
	const_cast<SMCSuperIO*>(sio)->quickReschedule();
	*reinterpret_cast<uint32_t *>(data) = VirtualSMCAPI::encodeFlt(val);
	return SmcSuccess;
}

SMC_RESULT TemperatureKey::readAccess() {
	double val = device->getTemperatureValue(index);
	const_cast<SMCSuperIO*>(sio)->quickReschedule();
	*reinterpret_cast<uint16_t *>(data) = VirtualSMCAPI::encodeIntSp(SmcKeyTypeSp78, val);
	return SmcSuccess;
}
