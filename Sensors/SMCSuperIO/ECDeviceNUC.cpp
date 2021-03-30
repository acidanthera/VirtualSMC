//
//  ECDeviceNUC.cpp
//  SMCSuperIO
//
//  Copyright © 2021 vit9696. All rights reserved.
//

#include "ECDeviceNUC.hpp"


namespace EC {
	uint16_t ECDeviceNUC::readBigWordMMIOCached(uint32_t addr) {
		for (int i = 0; i < MmioCacheSize; i++) {
			uint16_t key = cachedMmio[i] >> 16;
			if (key == (uint16_t)addr) {
				return cachedMmio[i] & 0xFFFF;
			}
		}
		uint16_t value = readBigWordMMIO(addr);
		int last = (cachedMmioLast + 1) % MmioCacheSize;
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
}
