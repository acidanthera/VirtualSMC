//
//  ECDeviceNUC.cpp
//  SMCSuperIO
//
//  Copyright © 2021 vit9696. All rights reserved.
//

#include "ECDeviceNUC.hpp"


namespace EC {
	const char* ECDeviceNUC::getModelName() {
		return "NUC";
	}

	uint8_t ECDeviceNUC::getTachometerCount() {
		return 1;
	}

	uint16_t ECDeviceNUC::updateTachometer(uint8_t index) {
		if (mmioArea != nullptr)
			return (mmioArea[B_NUC_EC_V8_CPUFAN_U16] << 8) | (mmioArea[B_NUC_EC_V8_CPUFAN_U16 + 1]);
		return 0;
	}

	const char *ECDeviceNUC::getTachometerName(uint8_t index) {
		return "CPU FAN";
	}

	uint8_t ECDeviceNUC::getVoltageCount() {
		return 0;
	}

	float ECDeviceNUC::updateVoltage(uint8_t index) {
		if (mmioArea != nullptr)
			return 0;
		return 0;
	}

	const char *ECDeviceNUC::getVoltageName(uint8_t index) {
		return "";
	}

	ECDevice* ECDeviceNUC::detect(SMCSuperIO* sio) {
		return new ECDeviceNUC;
	}
}
