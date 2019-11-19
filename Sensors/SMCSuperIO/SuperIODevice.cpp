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
	IOSimpleLockLock(getSmcSuperIO()->counterLock);
	updateTachometers();
	updateVoltages();
	IOSimpleLockUnlock(getSmcSuperIO()->counterLock);
	updateIORegistry();
}

void SuperIODevice::updateIORegistry() {
	auto obj = OSDynamicCast(IORegistryEntry, getSmcSuperIO());
	if (obj) {
		OSDictionary *dict = OSDictionary::withCapacity(32);
		for (uint8_t index = 0; index < getTachometerCount(); ++index) {
			uint16_t value = getTachometerValue(index);
			dict->setObject(OSString::withCString(getTachometerName(index)), OSNumber::withNumber(value, 16));
		}
		for (uint8_t index = 0; index < getVoltageCount(); ++index) {
			float value = getVoltageValue(index);
			OSData *data = OSData::withBytes(reinterpret_cast<const void*>(&value), sizeof(value));
			dict->setObject(OSString::withCString(getVoltageName(index)), data);
		}
		obj->setProperty("Sensors", dict);
	}
}

/**
 *  Keys
 */
SMC_RESULT TachometerKey::readAccess() {
	IOSimpleLockLock(sio->counterLock);
	double val = device->getTachometerValue(index);
	const_cast<SMCSuperIO*>(sio)->quickReschedule();
	IOSimpleLockUnlock(sio->counterLock);
	*reinterpret_cast<uint16_t *>(data) = VirtualSMCAPI::encodeFp(SmcKeyTypeFpe2, val);
	return SmcSuccess;
}
