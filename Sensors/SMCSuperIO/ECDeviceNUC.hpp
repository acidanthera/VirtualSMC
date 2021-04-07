//
//  ECDevice.hpp
//
//  Sensors implementation for EC SuperIO device
//
//

#ifndef _ECDEVICE_NUC_HPP
#define _ECDEVICE_NUC_HPP

#include "SuperIODevice.hpp"
#include "ECDevice.hpp"

namespace EC {
	// Intel EC Temperature types connected to newer NUCs
	static constexpr uint32_t B_NUC_EC_TEMP_TYPE_INTERNAL_AMBIENT = 1;
	static constexpr uint32_t B_NUC_EC_TEMP_TYPE_CPU_VRM          = 2;
	static constexpr uint32_t B_NUC_EC_TEMP_TYPE_DGPU_VRM         = 3;
	static constexpr uint32_t B_NUC_EC_TEMP_TYPE_SSD_M2_SLOT_1    = 4;
	static constexpr uint32_t B_NUC_EC_TEMP_TYPE_MEMORY           = 6;
	static constexpr uint32_t B_NUC_EC_TEMP_TYPE_FAN_AIR_VENT     = 8;
	static constexpr uint32_t B_NUC_EC_TEMP_TYPE_SSD_M2_SLOT_2    = 9;
	static constexpr uint32_t B_NUC_EC_TEMP_TYPE_SSD_M2_SLOT_3    = 16;
	static constexpr uint32_t B_NUC_EC_TEMP_TYPE_SSD_M2_SLOT_4    = 17;

	// Intel EC FAN types connected to newer NUCs
	static constexpr uint32_t B_NUC_EC_FAN_TYPE_CPU         = 1;
	static constexpr uint32_t B_NUC_EC_FAN_TYPE_DGPU        = 2;
	static constexpr uint32_t B_NUC_EC_FAN_TYPE_SYSTEM_FAN1 = 3;
	static constexpr uint32_t B_NUC_EC_FAN_TYPE_SYSTEM_FAN2 = 4;
	static constexpr uint32_t B_NUC_EC_FAN_TYPE_COOLER_PUMP = 5; // Liquid Cooler Pump
	static constexpr uint32_t B_NUC_EC_FAN_TYPE_THERMAL_FAN = 6; // Thermal Solution Fan

	// Intel EC Voltage types connected to newer NUCs
	static constexpr uint32_t B_NUC_EC_VOLTAGE_TYPE_CPU_VCCIN   = 1;
	static constexpr uint32_t B_NUC_EC_VOLTAGE_TYPE_CPU_IO      = 2;
	static constexpr uint32_t B_NUC_EC_VOLTAGE_TYPE_33V_VSB     = 3;
	static constexpr uint32_t B_NUC_EC_VOLTAGE_TYPE_33V         = 4;
	static constexpr uint32_t B_NUC_EC_VOLTAGE_TYPE_5V_VSB      = 5;
	static constexpr uint32_t B_NUC_EC_VOLTAGE_TYPE_5V          = 6;
	static constexpr uint32_t B_NUC_EC_VOLTAGE_TYPE_DIMM        = 7;
	static constexpr uint32_t B_NUC_EC_VOLTAGE_TYPE_DC_IN       = 10;

	// Intel EC V1 - MKKBLY35 / MKKBLi5v
	static constexpr uint32_t R_NUC_EC_V1_ACTIVATION_U32      = 0x41C; // 32-bit.
	static constexpr uint32_t B_NUC_EC_V1_ACTIVATION_EN       = 1;  // OR with this if not set.

	static constexpr uint32_t B_NUC_EC_V1_CPU_TEMP_U8         = 0x402;
	static constexpr uint32_t B_NUC_EC_V1_PCH_TEMP_U8         = 0x403;
	static constexpr uint32_t B_NUC_EC_V1_HDD_TEMP_U8         = 0x404;
	static constexpr uint32_t B_NUC_EC_V1_DIMM_TEMP_U8        = 0x405;

	static constexpr uint32_t B_NUC_EC_V1_SLOT_FAN_U16        = 0x412;

	static constexpr uint32_t B_NUC_EC_V1_VIN_U16             = 0x406;
	static constexpr uint32_t B_NUC_EC_V1_VCORE_U16           = 0x408;
	static constexpr uint32_t B_NUC_EC_V1_VDIMM_U16           = 0x40A;
	static constexpr uint32_t B_NUC_EC_V1_VCC3_U16            = 0x40C;
	static constexpr uint32_t B_NUC_EC_V1_VCCIO_U16           = 0x40E;

	// Intel EC V2 - AYAPLCEL
	static constexpr uint32_t B_NUC_EC_V2_VR_TEMP_U8          = 0x504;
	static constexpr uint32_t B_NUC_EC_V2_MOTHERBOARD_TEMP_U8 = 0x505;

	static constexpr uint32_t B_NUC_EC_V2_CPU_FAN_U16         = 0x508;

	static constexpr uint32_t B_NUC_EC_V2_CPU1_INPUT_U16      = 0x512;
	static constexpr uint32_t B_NUC_EC_V2_SDRAM_U16           = 0x514;
	static constexpr uint32_t B_NUC_EC_V2_V33_U16             = 0x516;
	static constexpr uint32_t B_NUC_EC_V2_V5_U16              = 0x518;

	// Intel EC V3 - BNKBL357
	static constexpr uint32_t B_NUC_EC_V3_CPU_TEMP_U8         = 0x502;
	static constexpr uint32_t B_NUC_EC_V3_PCH_TEMP_U8         = 0x503;
	static constexpr uint32_t B_NUC_EC_V3_MEMORY_TEMP_U8      = 0x504;
	static constexpr uint32_t B_NUC_EC_V3_MOTHERBOARD_TEMP_U8 = 0x505;

	static constexpr uint32_t B_NUC_EC_V3_CPU_FAN_U16         = 0x508;

	static constexpr uint32_t B_NUC_EC_V3_CPU1_INPUT_U16      = 0x512;
	static constexpr uint32_t B_NUC_EC_V3_SDRAM_U16           = 0x514;
	static constexpr uint32_t B_NUC_EC_V3_V33_U16             = 0x516;
	static constexpr uint32_t B_NUC_EC_V3_CPU_IO_U16          = 0x518;

	// Intel EC V4 - CYCNLi35
	static constexpr uint32_t B_NUC_EC_V4_CPU_TEMP_U8         = 0x413;
	static constexpr uint32_t B_NUC_EC_V4_PCH_TEMP_U8         = 0x414;
	static constexpr uint32_t B_NUC_EC_V4_MOTHERBOARD_TEMP_U8 = 0x41C;
	static constexpr uint32_t B_NUC_EC_V4_CPU_VR_TEMP_U8      = 0x41D;
	static constexpr uint32_t B_NUC_EC_V4_GPU_TEMP_U8         = 0x41E; // Bug?

	static constexpr uint32_t B_NUC_EC_V4_FAN_U16             = 0x411;

	static constexpr uint32_t B_NUC_EC_V4_CPU_CORE_U16        = 0x408;
	static constexpr uint32_t B_NUC_EC_V4_DIMM_U16            = 0x40A;
	static constexpr uint32_t B_NUC_EC_V4_DC_IN_U16           = 0x40C;

	// Intel EC V5 - HNKBLi70
	static constexpr uint32_t B_NUC_EC_V5_CPU_TEMP_U8         = 0x416;
	static constexpr uint32_t B_NUC_EC_V5_PCH_TEMP_U8         = 0x417;
	static constexpr uint32_t B_NUC_EC_V5_MEMORY_TEMP_U8      = 0x418; // SSD1 / Memory
	static constexpr uint32_t B_NUC_EC_V5_MOTHERBOARD_TEMP_U8 = 0x419; // SSD2 / Motherboard

	static constexpr uint32_t B_NUC_EC_V5_FAN1_U16            = 0x412;
	static constexpr uint32_t B_NUC_EC_V5_FAN2_U16            = 0x414;

	static constexpr uint32_t B_NUC_EC_V5_CPU_U16             = 0x406;
	static constexpr uint32_t B_NUC_EC_V5_DIMM_U16            = 0x408;
	static constexpr uint32_t B_NUC_EC_V5_DC_IN_U16           = 0x40A;
	static constexpr uint32_t B_NUC_EC_V5_DC_IN_ALT_U16       = 0x40C; // Bug?

	// Intel EC V6 - DNKBLi7v / DNKBLi30
	static constexpr uint32_t B_NUC_EC_V6_CPU_TEMP_U8         = 0x413;
	static constexpr uint32_t B_NUC_EC_V6_PCH_TEMP_U8         = 0x414;
	static constexpr uint32_t B_NUC_EC_V6_MOTHERBOARD_TEMP_U8 = 0x41C;
	static constexpr uint32_t B_NUC_EC_V6_CPU_VR_TEMP_U8      = 0x41D;
	static constexpr uint32_t B_NUC_EC_V6_GPU_TEMP_U8         = 0x41E; // Bug?

	static constexpr uint32_t B_NUC_EC_V6_FAN_U16             = 0x411;

	static constexpr uint32_t B_NUC_EC_V6_CPU_CORE_U16        = 0x408;
	static constexpr uint32_t B_NUC_EC_V6_DIMM_U16            = 0x40A;
	static constexpr uint32_t B_NUC_EC_V6_DC_IN_U16           = 0x40C;

	// Intel EC V7 - JYGLKCPX
	static constexpr uint32_t B_NUC_EC_V7_VR_TEMP_U8          = 0x504;
	static constexpr uint32_t B_NUC_EC_V7_DIMM_TEMP_U8        = 0x505;
	static constexpr uint32_t B_NUC_EC_V7_MOTHERBOARD_TEMP_U8 = 0x506;

	static constexpr uint32_t B_NUC_EC_V7_CPU_FAN_U16         = 0x508;

	static constexpr uint32_t B_NUC_EC_V7_VIN_U16             = 0x510;
	static constexpr uint32_t B_NUC_EC_V7_VCORE_U16           = 0x512;
	static constexpr uint32_t B_NUC_EC_V7_VDIMM_U16           = 0x514;
	static constexpr uint32_t B_NUC_EC_V7_V33_U16             = 0x516;
	static constexpr uint32_t B_NUC_EC_V7_V5_VCC_U16          = 0x518;

	// Intel EC V8 - BECFL357 (NUC8i7BE, NUC8i5BE, NUC8i3BE)
	static constexpr uint32_t B_NUC_EC_V8_PCH_TEMP_U8         = 0x503;
	static constexpr uint32_t B_NUC_EC_V8_VR_TEMP_U8          = 0x504;
	static constexpr uint32_t B_NUC_EC_V8_DIMM_TEMP_U8        = 0x505;
	static constexpr uint32_t B_NUC_EC_V8_MOTHERBOARD_TEMP_U8 = 0x506;

	static constexpr uint32_t B_NUC_EC_V8_CPU_FAN_U16         = 0x508;

	static constexpr uint32_t B_NUC_EC_V8_V5VSB_U16           = 0x510;
	static constexpr uint32_t B_NUC_EC_V8_VCORE_U16           = 0x512;
	static constexpr uint32_t B_NUC_EC_V8_VDIMM_U16           = 0x514;
	static constexpr uint32_t B_NUC_EC_V8_VCCIO_U16           = 0x518;

	// Intel EC V9 - FNCML357 / PNWHL357 / PNWHLV57 / QNCFLX70 / QXCFL579 / CHAPLCEL / CBWHL357 / CBWHLMIV / EBTGL357 / EBTGLMIV
	static constexpr uint32_t B_NUC_EC_V9_IDENTIFIER_U48      = 0x400; // Only identifiers specified below are supported.
	static constexpr const char *B_NUC_EC_V9_IDENTIFIER_ELM   = "ELM_EC";
	static constexpr const char *B_NUC_EC_V9_IDENTIFIER_SPG   = "SPG_EC";

	static constexpr uint32_t B_NUC_EC_V9_TEMP1_TYPE_U8       = 0x40D; // Refer to B_NUC_EC_FAN_TYPE.
	static constexpr uint32_t B_NUC_EC_V9_TEMP1_U8            = 0x40E;
	static constexpr uint32_t B_NUC_EC_V9_TEMP2_TYPE_U8       = 0x40F;
	static constexpr uint32_t B_NUC_EC_V9_TEMP2_U8            = 0x410;
	static constexpr uint32_t B_NUC_EC_V9_TEMP3_TYPE_U8       = 0x411;
	static constexpr uint32_t B_NUC_EC_V9_TEMP3_U8            = 0x412;

	static constexpr uint32_t B_NUC_EC_V9_FAN1_TYPE_U8        = 0x41A; // Refer to B_NUC_EC_FAN_TYPE.
	static constexpr uint32_t B_NUC_EC_V9_FAN1_U16            = 0x41B;
	static constexpr uint32_t B_NUC_EC_V9_FAN2_TYPE_U8        = 0x41D;
	static constexpr uint32_t B_NUC_EC_V9_FAN2_U16            = 0x41E;
	static constexpr uint32_t B_NUC_EC_V9_FAN3_TYPE_U8        = 0x420;
	static constexpr uint32_t B_NUC_EC_V9_FAN3_U16            = 0x421;

	static constexpr uint32_t B_NUC_EC_V9_VOLTAGE1_TYPE_U8    = 0x423;
	static constexpr uint32_t B_NUC_EC_V9_VOLTAGE1_U16        = 0x424;
	static constexpr uint32_t B_NUC_EC_V9_VOLTAGE2_TYPE_U8    = 0x426;
	static constexpr uint32_t B_NUC_EC_V9_VOLTAGE2_U16        = 0x427;
	static constexpr uint32_t B_NUC_EC_V9_VOLTAGE3_TYPE_U8    = 0x429;
	static constexpr uint32_t B_NUC_EC_V9_VOLTAGE3_U16        = 0x42A;
	static constexpr uint32_t B_NUC_EC_V9_VOLTAGE4_TYPE_U8    = 0x42C;
	static constexpr uint32_t B_NUC_EC_V9_VOLTAGE4_U16        = 0x42D;
	static constexpr uint32_t B_NUC_EC_V9_VOLTAGE5_TYPE_U8    = 0x42F;
	static constexpr uint32_t B_NUC_EC_V9_VOLTAGE5_U16        = 0x430;

	// Intel EC VA - PATGL357 / TNTGLV57 / TNTGL357 / PHTGL579
	static constexpr uint32_t B_NUC_EC_VA_IDENTIFIER_U48      = 0x400; // Only identifiers specified below are supported.
	static constexpr const char *B_NUC_EC_VA_IDENTIFIER_NUC   = "NUC_EC";

	static constexpr uint32_t B_NUC_EC_VA_TEMP1_TYPE_U8       = 0x410; // Refer to B_NUC_EC_FAN_TYPE.
	static constexpr uint32_t B_NUC_EC_VA_TEMP1_U8            = 0x411;
	static constexpr uint32_t B_NUC_EC_VA_TEMP2_TYPE_U8       = 0x412;
	static constexpr uint32_t B_NUC_EC_VA_TEMP2_U8            = 0x413;
	static constexpr uint32_t B_NUC_EC_VA_TEMP3_TYPE_U8       = 0x414;
	static constexpr uint32_t B_NUC_EC_VA_TEMP3_U8            = 0x415;
	static constexpr uint32_t B_NUC_EC_VA_TEMP4_TYPE_U8       = 0x416;
	static constexpr uint32_t B_NUC_EC_VA_TEMP4_U8            = 0x417;
	static constexpr uint32_t B_NUC_EC_VA_TEMP5_TYPE_U8       = 0x418;
	static constexpr uint32_t B_NUC_EC_VA_TEMP5_U8            = 0x419;

	static constexpr uint32_t B_NUC_EC_VA_FAN1_TYPE_U8        = 0x41C; // Refer to B_NUC_EC_FAN_TYPE.
	static constexpr uint32_t B_NUC_EC_VA_FAN1_U16            = 0x41D;
	static constexpr uint32_t B_NUC_EC_VA_FAN2_TYPE_U8        = 0x41F;
	static constexpr uint32_t B_NUC_EC_VA_FAN2_U16            = 0x420;
	static constexpr uint32_t B_NUC_EC_VA_FAN3_TYPE_U8        = 0x422;
	static constexpr uint32_t B_NUC_EC_VA_FAN3_U16            = 0x423;
	static constexpr uint32_t B_NUC_EC_VA_FAN4_TYPE_U8        = 0x425;
	static constexpr uint32_t B_NUC_EC_VA_FAN4_U16            = 0x426;
	static constexpr uint32_t B_NUC_EC_VA_FAN5_TYPE_U8        = 0x428;
	static constexpr uint32_t B_NUC_EC_VA_FAN5_U16            = 0x429;
	static constexpr uint32_t B_NUC_EC_VA_FAN6_TYPE_U8        = 0x42B;
	static constexpr uint32_t B_NUC_EC_VA_FAN6_U16            = 0x42C;

	static constexpr uint32_t B_NUC_EC_VA_VOLTAGE1_TYPE_U8    = 0x42E;
	static constexpr uint32_t B_NUC_EC_VA_VOLTAGE1_U16        = 0x42F;
	static constexpr uint32_t B_NUC_EC_VA_VOLTAGE2_TYPE_U8    = 0x431;
	static constexpr uint32_t B_NUC_EC_VA_VOLTAGE2_U16        = 0x432;
	static constexpr uint32_t B_NUC_EC_VA_VOLTAGE3_TYPE_U8    = 0x434;
	static constexpr uint32_t B_NUC_EC_VA_VOLTAGE3_U16        = 0x435;
	static constexpr uint32_t B_NUC_EC_VA_VOLTAGE4_TYPE_U8    = 0x437;
	static constexpr uint32_t B_NUC_EC_VA_VOLTAGE4_U16        = 0x438;
	static constexpr uint32_t B_NUC_EC_VA_VOLTAGE5_TYPE_U8    = 0x43A;
	static constexpr uint32_t B_NUC_EC_VA_VOLTAGE5_U16        = 0x43B;
	static constexpr uint32_t B_NUC_EC_VA_VOLTAGE6_TYPE_U8    = 0x43D;
	static constexpr uint32_t B_NUC_EC_VA_VOLTAGE6_U16        = 0x43E;

	// Intel EC VB - INWHL357
	static constexpr uint32_t B_NUC_EC_VB_CPU_TEMP_U8         = 0x413;
	static constexpr uint32_t B_NUC_EC_VB_PCH_TEMP_U8         = 0x414;
	static constexpr uint32_t B_NUC_EC_VB_MOTHERBOARD_TEMP_U8 = 0x41C;
	static constexpr uint32_t B_NUC_EC_VB_CPU_VR_TEMP_U8      = 0x41D;
	static constexpr uint32_t B_NUC_EC_VB_GPU_TEMP_U8         = 0x41E;
	static constexpr uint32_t B_NUC_EC_VB_GPU_VR_U8           = 0x404;

	static constexpr uint32_t B_NUC_EC_VB_FAN_U16             = 0x411;

	static constexpr uint32_t B_NUC_EC_VB_CPU_CORE_U16        = 0x408;
	static constexpr uint32_t B_NUC_EC_VB_DIMM_U16            = 0x40A;
	static constexpr uint32_t B_NUC_EC_VB_DC_IN_U16           = 0x40C;
	static constexpr uint32_t B_NUC_EC_VB_GPU_CORE_U16        = 0x40E;
	static constexpr uint32_t B_NUC_EC_VB_DC_IN_ALT_U16       = 0x440; // Bug?

	class ECDeviceNUC : public ECDevice {
		static constexpr int MmioCacheSize = 32;
		uint32_t cachedMmio[MmioCacheSize] {};
		int cachedMmioLast {0};

	protected:
		/**
		 * Cached MMIO read for type fields.
		 */
		uint16_t readBigWordMMIOCached(uint32_t addr);

		/**
		 * Decode name from type fields
		 */
		const char *getTachometerNameForType(uint16_t type);
		const char *getVoltageNameForType(uint16_t type);
		const char *getTemperatureNameForType(uint16_t type);

		/**
		 * Decode SMC key from type fields
		 */
		SMC_KEY getTachometerSMCKeyForType(uint16_t type, int index = 0);
		SMC_KEY getVoltageSMCKeyForType(uint16_t type, int index = 0);
		SMC_KEY getTemperatureSMCKeyForType(uint16_t type, int index = 0);

		/**
		 *  Ctor
		 */
		ECDeviceNUC() {
			supportsMMIO = true;
		}
	};
}

#endif // _ECDEVICE_NUC_HPP
