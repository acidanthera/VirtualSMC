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
		return 0;
	}

	const char *ECDeviceNUC::getVoltageName(uint8_t index) {
		return "";
	}

	uint8_t ECDeviceNUC::getTemperatureCount() {
		// TODO: Implement.
		return 0;
	}

	float ECDeviceNUC::updateTemperature(uint8_t index) {
		// TODO: Implement.
		return 0;
	}

	const char *ECDeviceNUC::getTemperatureName(uint8_t index) {
		return "";
	}

	void ECDeviceNUC::setupVoltageKeys(VirtualSMCAPI::Plugin &vsmcPlugin) {
		// TODO: Provide various keys for voltages here.
	}
	
	void ECDeviceNUC::setupTemperatureKeys(VirtualSMCAPI::Plugin &vsmcPlugin) {
		// TODO: Provide various keys for temperatures here.
	}

	uint16_t ECDeviceNUC::readBigWordMMIOCached(uint32_t addr) {
		for (int i = 0; i < kMmioCacheSize; i++) {
			uint16_t key = cachedMmio[i] >> 16;
			if (key == (uint16_t)addr) {
				return cachedMmio[i] & 0xFFFF;
			}
		}
		uint16_t value = readBigWordMMIO(addr);
		int last = (cachedMmioLast + 1) % kMmioCacheSize;
		cachedMmio[last] = ((addr & 0xFFFF) << 16) | value;
		cachedMmioLast = last;
		return value;
	}

	const char *ECDeviceNUC::getTachometerNameForType(uint16_t type){
		switch (type) {
			case B_NUC_EC_FAN_TYPE_CPU: return "FANCPU";
			case B_NUC_EC_FAN_TYPE_DGPU: return "FANGPU";
			case B_NUC_EC_FAN_TYPE_SYSTEM_FAN1: return "FANSYS1";
			case B_NUC_EC_FAN_TYPE_SYSTEM_FAN2: return "FANSYS2";
			case B_NUC_EC_FAN_TYPE_COOLER_PUMP: return "FANCOOLERPUMP";
			case B_NUC_EC_FAN_TYPE_THERMAL_FAN: return "FANTHERMAL";
			default: return "FAN";
		}
	}

	const char *ECDeviceNUC::getVoltageNameForType(uint16_t type) {
		switch (type) {
			case B_NUC_EC_VOLTAGE_TYPE_CPU_VCCIN: return "CPUVCORE";
			case B_NUC_EC_VOLTAGE_TYPE_CPU_IO: return "VIO";
			case B_NUC_EC_VOLTAGE_TYPE_33V_VSB: return "VSB33";
			case B_NUC_EC_VOLTAGE_TYPE_33V: return "VDD33";
			case B_NUC_EC_VOLTAGE_TYPE_5V_VSB: return "VSB5";
			case B_NUC_EC_VOLTAGE_TYPE_5V: return "VDD5";
			case B_NUC_EC_VOLTAGE_TYPE_DIMM: return "VDIMM";
			case B_NUC_EC_VOLTAGE_TYPE_DC_IN: return "VIN";
			default: return "VOLTAGE";
		}
	}

	const char *ECDeviceNUC::getTemperatureNameForType(uint16_t type) {
		switch (type) {
			case B_NUC_EC_TEMP_TYPE_INTERNAL_AMBIENT: return "TAMBIENT";
			case B_NUC_EC_TEMP_TYPE_CPU_VRM: return "TVRMCPU";
			case B_NUC_EC_TEMP_TYPE_DGPU_VRM: return "TVRMGPU";
			case B_NUC_EC_TEMP_TYPE_SSD_M2_SLOT_1: return "TSSD1";
			case B_NUC_EC_TEMP_TYPE_MEMORY: return "TDIMM";
			case B_NUC_EC_TEMP_TYPE_FAN_AIR_VENT: return "TVENT";
			case B_NUC_EC_TEMP_TYPE_SSD_M2_SLOT_2: return "TSSD2";
			case B_NUC_EC_TEMP_TYPE_SSD_M2_SLOT_3: return "TSSD3";
			case B_NUC_EC_TEMP_TYPE_SSD_M2_SLOT_4: return "TSSD4";
			default: return "TEMPERATURE";
		}
	}

	SMC_KEY ECDeviceNUC::getTachometerSMCKeyForType(uint16_t type, int index) {
		return KeyF0Ac(index);
	}

	SMC_KEY ECDeviceNUC::getVoltageSMCKeyForType(uint16_t type, int index) {
		switch (type) {
			case B_NUC_EC_VOLTAGE_TYPE_CPU_VCCIN: return KeyInvalid; // don't override SMCProcessor value
			case B_NUC_EC_VOLTAGE_TYPE_CPU_IO: return KeyInvalid; // don't override SMCProcessor value
			case B_NUC_EC_VOLTAGE_TYPE_33V_VSB: return KeyVR3R;
			case B_NUC_EC_VOLTAGE_TYPE_33V: return KeyVR3R;
			case B_NUC_EC_VOLTAGE_TYPE_5V_VSB: return KeyV50R(index);
			case B_NUC_EC_VOLTAGE_TYPE_5V: return KeyV50R(index);
			case B_NUC_EC_VOLTAGE_TYPE_DIMM: return KeyVM0R(index);
			case B_NUC_EC_VOLTAGE_TYPE_DC_IN: return KeyVD0R(index);
			default: return KeyInvalid;
		}
	}

	SMC_KEY ECDeviceNUC::getTemperatureSMCKeyForType(uint16_t type, int index) {
		switch (type) {
			case B_NUC_EC_TEMP_TYPE_INTERNAL_AMBIENT: return KeyTH0P(index);
			case B_NUC_EC_TEMP_TYPE_CPU_VRM: return KeyTC0P(index);
			case B_NUC_EC_TEMP_TYPE_DGPU_VRM: return KeyTG0P(index);
			case B_NUC_EC_TEMP_TYPE_SSD_M2_SLOT_1: return KeyTH0P(0);
			case B_NUC_EC_TEMP_TYPE_MEMORY: return KeyTM0P(index);
			case B_NUC_EC_TEMP_TYPE_FAN_AIR_VENT: return KeyTV0P(index);
			case B_NUC_EC_TEMP_TYPE_SSD_M2_SLOT_2: return KeyTH0P(1);
			case B_NUC_EC_TEMP_TYPE_SSD_M2_SLOT_3: return KeyTH0P(2);
			case B_NUC_EC_TEMP_TYPE_SSD_M2_SLOT_4: return KeyTH0P(3);
			default: return KeyInvalid;
		}
	}

	ECDevice* ECDeviceNUC::detect(SMCSuperIO* sio, const char *name) {
		if (strncmp(name, "Intel_EC_V", strlen("Intel_EC_V")) != 0 || name[strlen("Intel_EC_VX")] != '\0') {
			return nullptr;
		}

		uint32_t gen = 0;
		char genChar = name[strlen("Intel_EC_V")];
		if (genChar >= '1' && genChar <= '9') {
			gen = (genChar - '1') + NucEcGenerationV1;
		} else if (genChar >= 'A' && genChar <= 'B') {
			gen = (genChar - 'A') + NucEcGenerationVA;
		} else {
			SYSLOG("ssio", "unknown NUC EC generation %c", gen);
			return nullptr;
		}

		DBGLOG("ssio", "initialising NUC EC %d", gen);
		return new ECDeviceNUC(gen);
	}
}
