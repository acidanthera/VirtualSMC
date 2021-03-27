//
//  ECDeviceNUC.cpp
//  SMCSuperIO
//
//  Copyright © 2021 vit9696. All rights reserved.
//

#include "ECDeviceNUC.hpp"


namespace EC {
	const char* ECDeviceNUC::getModelName() {
		return "Intel NUC Embedded Controller";
	}

	uint8_t ECDeviceNUC::getTachometerCount() {
		switch (nucGeneration) {
			case NucEcGenerationV5:
				return 2;
			case NucEcGenerationV9:
				return 3;
			case NucEcGenerationVA:
				return 6;
			default:
				return 1;
		}
	}

	uint16_t ECDeviceNUC::updateTachometer(uint8_t index) {
		switch (nucGeneration) {
			case NucEcGenerationV1:
				return readBigWordMMIO(B_NUC_EC_V1_SLOT_FAN_U16);
			case NucEcGenerationV2:
				return readBigWordMMIO(B_NUC_EC_V2_CPU_FAN_U16);
			case NucEcGenerationV3:
				return readBigWordMMIO(B_NUC_EC_V3_CPU_FAN_U16);
			case NucEcGenerationV4:
				return readBigWordMMIO(B_NUC_EC_V4_FAN_U16);
			case NucEcGenerationV5:
				return index == 0 ? readBigWordMMIO(B_NUC_EC_V5_FAN1_U16) : readBigWordMMIO(B_NUC_EC_V5_FAN2_U16);
			case NucEcGenerationV6:
				return readBigWordMMIO(B_NUC_EC_V6_FAN_U16);
			case NucEcGenerationV7:
				return readBigWordMMIO(B_NUC_EC_V7_CPU_FAN_U16);
			case NucEcGenerationV8:
				return readBigWordMMIO(B_NUC_EC_V8_CPU_FAN_U16);
			case NucEcGenerationV9:
				if (index == 0) return readBigWordMMIO(B_NUC_EC_V9_FAN1_U16);
				if (index == 1) return readBigWordMMIO(B_NUC_EC_V9_FAN2_U16);
				if (index == 2) return readBigWordMMIO(B_NUC_EC_V9_FAN3_U16);
				return 0;
			case NucEcGenerationVA:
				if (index == 0) return readBigWordMMIO(B_NUC_EC_VA_FAN1_U16);
				if (index == 1) return readBigWordMMIO(B_NUC_EC_VA_FAN2_U16);
				if (index == 2) return readBigWordMMIO(B_NUC_EC_VA_FAN3_U16);
				if (index == 3) return readBigWordMMIO(B_NUC_EC_VA_FAN4_U16);
				if (index == 4) return readBigWordMMIO(B_NUC_EC_VA_FAN5_U16);
				if (index == 5) return readBigWordMMIO(B_NUC_EC_VA_FAN6_U16);
				return 0;
			case NucEcGenerationVB:
				return readBigWordMMIO(B_NUC_EC_VB_FAN_U16);
			default:
				return 0;
		}
	}

	const char *ECDeviceNUC::getTachometerName(uint8_t index) {
		// TODO: Better names.
		if (getTachometerCount() == 1) return "FAN";
		switch (index) {
			case 0: return "FAN1";
			case 1: return "FAN2";
			case 2: return "FAN3";
			case 3: return "FAN4";
			case 4: return "FAN5";
			case 5: return "FAN6";
			default: return "FAN";
		}
	}

	uint8_t ECDeviceNUC::getVoltageCount() {
		// TODO: Implement.
		return 0;
	}

	float ECDeviceNUC::updateVoltage(uint8_t index) {
		// TODO: Implement.
		if (mmioArea != nullptr)
			return 0;
		return 0;
	}

	const char *ECDeviceNUC::getVoltageName(uint8_t index) {
		return "";
	}

	void ECDeviceNUC::setupExtraKeys(VirtualSMCAPI::Plugin &vsmcPlugin) {
		
	}

	ECDevice* ECDeviceNUC::detect(SMCSuperIO* sio, const char *name) {
		if (name[0] == '\0') {
			SYSLOG("ssio", "please inject ec-device property into LPC with the name");
			return nullptr;
		}

		if (strncmp(name, "Intel_EC_V", strlen("Intel_EC_V")) != 0 || name[strlen("Intel_EC_VX")] != '\0') {
			return nullptr;
		}

		uint32_t gen = 0;
		char genChar = name[strlen("Intel_EC_V")];
		if (genChar >= '1' && gen <= '9') {
			gen = (genChar - '1') + NucEcGenerationV1;
		} else if (genChar >= 'A' && gen <= 'B') {
			gen = (genChar - 'A') + NucEcGenerationVA;
		} else {
			SYSLOG("ssio", "unknown NUC EC generation %c", gen);
			return nullptr;
		}

		DBGLOG("ssio", "initialising NUC EC %d", gen);
		return new ECDeviceNUC(gen);
	}
}
