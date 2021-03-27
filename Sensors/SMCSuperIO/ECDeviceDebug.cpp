//
//  ECDeviceDebug.cpp
//  SMCSuperIO
//
//  Copyright © 2021 vit9696. All rights reserved.
//

#include "ECDeviceDebug.hpp"

namespace EC {
	const char* ECDeviceDebug::getModelName() {
		return "DEBUG Embedded Controller";
	}

	uint8_t ECDeviceDebug::getTachometerCount() {
		return windowSize;
	}

	uint16_t ECDeviceDebug::updateTachometer(uint8_t index) {
		if (mmioArea != nullptr)
			return mmioArea[windowBase + index];
		return readBytePMIO(windowBase + index);
	}

	const char *ECDeviceDebug::getTachometerName(uint8_t index) {
		return "FAN";
	}

	uint8_t ECDeviceDebug::getVoltageCount() {
		return 0;
	}

	float ECDeviceDebug::updateVoltage(uint8_t index) {
		if (mmioArea != nullptr)
			return 0;
		return 0;
	}

	const char *ECDeviceDebug::getVoltageName(uint8_t index) {
		return "";
	}

	ECDevice* ECDeviceDebug::detect(SMCSuperIO* sio, const char *name) {
		if (strcmp(name, "debug") != 0)
			return nullptr;

		uint32_t base = 0;
		uint32_t size = 5;
		PE_parse_boot_argn("ssiownd", &base, sizeof(base));
		PE_parse_boot_argn("ssiowndsz", &size, sizeof(size));


		DBGLOG("ssio", "initialising DEBUG EC at 0x%04X with window 0x%04X", base, size);
		return new ECDeviceDebug(base, size);
	}
}
