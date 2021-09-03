//
//  ECDeviceGeneric.cpp
//  SMCSuperIO
//
//  Copyright © 2021 vit9696. All rights reserved.
//

#include "ECDeviceGeneric.hpp"

namespace EC {
	const char* ECDeviceGeneric::getModelName() {
		return modelName;
	}

	uint8_t ECDeviceGeneric::getTachometerCount() {
		return tachometerCount;
	}

	uint16_t ECDeviceGeneric::updateTachometer(uint8_t index) {
		auto &t = tachometers[index];
		auto val = readValue(t.addr, t.size == sizeof(uint16_t), t.bigEndian);
		if (t.inverse)
			val = (t.size == sizeof(uint16_t) ? 0xFFFF : 0xFF) - val;
		auto r = val * t.mul / t.div;
		if (t.dividend != 0 && r != 0)
			return t.dividend / r;
		return r;
	}

	const char *ECDeviceGeneric::getTachometerName(uint8_t index) {
		return tachometers[index].name != nullptr ? tachometers[index].name : "FAN";
	}

	uint8_t ECDeviceGeneric::getVoltageCount() {
		return 0;
	}

	float ECDeviceGeneric::updateVoltage(uint8_t index) {
		return 0;
	}

	const char *ECDeviceGeneric::getVoltageName(uint8_t index) {
		return "";
	}

	ECDeviceGeneric::ECDeviceGeneric(IORegistryEntry *lpc) {
		device = lpc;

		if (WIOKit::getOSDataValue(lpc, "fan-count", tachometerCount) && (tachometerCount == 0 || tachometerCount >= MaxTachometerCount)) {
			tachometerCount = 1;
		}

		// TODO: Support MMIO
		supportsPMIO = true;

		for (uint32_t i = 0; i < tachometerCount; i++) {
			char name[64];
			snprintf(name, sizeof(name), "fan%u-addr", i);
			WIOKit::getOSDataValue(lpc, name, tachometers[i].addr);

			snprintf(name, sizeof(name), "fan%u-size", i);
			uint32_t tmp = 1;
			if (WIOKit::getOSDataValue(lpc, name, tmp) && (tmp == 0 || tmp > sizeof(uint16_t)))
				tmp = 1;
			tachometers[i].size = tmp;

			tmp = 0;
			snprintf(name, sizeof(name), "fan%u-big", i);
			if (WIOKit::getOSDataValue(lpc, name, tmp) && (tmp != 0 && tmp != 1))
				tmp = 0;
			tachometers[i].bigEndian = tmp != 0;

			tmp = 0;
			snprintf(name, sizeof(name), "fan%u-inverse", i);
			if (WIOKit::getOSDataValue(lpc, name, tmp) && (tmp != 0 && tmp != 1))
				tmp = 0;
			tachometers[i].inverse = tmp != 0;

			snprintf(name, sizeof(name), "fan%u-mul", i);
			if (WIOKit::getOSDataValue(lpc, name, tachometers[i].mul) && tachometers[i].mul == 0)
				tachometers[i].mul = 1;

			snprintf(name, sizeof(name), "fan%u-div", i);
			if (WIOKit::getOSDataValue(lpc, name, tachometers[i].div) && tachometers[i].div == 0)
				tachometers[i].div = 1;
			
			snprintf(name, sizeof(name), "fan%u-dividend", i);
			WIOKit::getOSDataValue(lpc, name, tachometers[i].dividend);

			DBGLOG("ssio", "added tach%u at 0x%04X (%d bytes, %s)", i, tachometers[i].addr, tachometers[i].size,
				   tachometers[i].bigEndian ? "big" : "little");
		}
	}

	ECDevice* ECDeviceGeneric::detect(SMCSuperIO* sio, const char *name, IORegistryEntry *lpc) {
		if (strcmp(name, "generic") != 0)
			return nullptr;

		DBGLOG("ssio", "initialising Generic EC");
		return new ECDeviceGeneric(lpc);
	}
}
