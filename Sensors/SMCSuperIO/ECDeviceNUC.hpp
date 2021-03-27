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
	// Intel EC FAN types connected to newer NUCs
	static constexpr uint32_t B_NUC_EC_FAN_TYPE_CPU         = 1;
	static constexpr uint32_t B_NUC_EC_FAN_TYPE_DGPU        = 2;
	static constexpr uint32_t B_NUC_EC_FAN_TYPE_SYSTEM_FAN1 = 3;
	static constexpr uint32_t B_NUC_EC_FAN_TYPE_SYSTEM_FAN2 = 4;
	static constexpr uint32_t B_NUC_EC_FAN_TYPE_COOLER_PUMP = 5; // Liquid Cooler Pump
	static constexpr uint32_t B_NUC_EC_FAN_TYPE_THERMAL_FAN = 6; // Thermal Solution Fan

	// Intel EC V1 - MKKBLY35 / MKKBLi5v
	static constexpr uint32_t R_NUC_EC_V1_ACTIVATION_U32      = 0x41C; // 32-bit.
	static constexpr uint32_t B_NUC_EC_V1_ACTIVATION_EN       = 1;  // OR with this if not set.

	static constexpr uint32_t B_NUC_EC_V1_SLOT_FAN_U16        = 0x412;

	// Intel EC V2 - AYAPLCEL
	static constexpr uint32_t B_NUC_EC_V2_CPU_FAN_U16         = 0x508;

	// Intel EC V3 - BNKBL357
	static constexpr uint32_t B_NUC_EC_V3_CPU_FAN_U16         = 0x508;

	// Intel EC V4 - CYCNLi35
	static constexpr uint32_t B_NUC_EC_V4_FAN_U16             = 0x411;

	// Intel EC V5 - HNKBLi70
	static constexpr uint32_t B_NUC_EC_V5_FAN1_U16            = 0x412;
	static constexpr uint32_t B_NUC_EC_V5_FAN2_U16            = 0x414;

	// Intel EC V6 - DNKBLi7v / DNKBLi30
	static constexpr uint32_t B_NUC_EC_V6_FAN_U16             = 0x411;

	// Intel EC V7 - JYGLKCPX
	static constexpr uint32_t B_NUC_EC_V7_CPU_FAN_U16         = 0x508;

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

	static constexpr uint32_t B_NUC_EC_V9_FAN1_TYPE_U8        = 0x41A; // Refer to B_NUC_EC_FAN_TYPE.
	static constexpr uint32_t B_NUC_EC_V9_FAN1_U16            = 0x41B;
	static constexpr uint32_t B_NUC_EC_V9_FAN2_TYPE_U8        = 0x41D;
	static constexpr uint32_t B_NUC_EC_V9_FAN2_U16            = 0x41E;
	static constexpr uint32_t B_NUC_EC_V9_FAN3_TYPE_U8        = 0x420;
	static constexpr uint32_t B_NUC_EC_V9_FAN3_U16            = 0x421;

	// Intel EC VA - PATGL357 / TNTGLV57 / TNTGL357 / PHTGL579
	static constexpr uint32_t B_NUC_EC_VA_IDENTIFIER_U48      = 0x400; // Only identifiers specified below are supported.
	static constexpr const char *B_NUC_EC_VA_IDENTIFIER_NUC   = "NUC_EC";

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

	// Intel EC VB - INWHL357
	static constexpr uint32_t B_NUC_EC_VB_FAN_U16             = 0x411;

	class ECDeviceNUC : public ECDevice {
		enum {
			NucEcGenerationV1,
			NucEcGenerationV2,
			NucEcGenerationV3,
			NucEcGenerationV4,
			NucEcGenerationV5,
			NucEcGenerationV6,
			NucEcGenerationV7,
			NucEcGenerationV8,
			NucEcGenerationV9,
			NucEcGenerationVA,
			NucEcGenerationVB,
		};

	public:
		const char* getModelName() override;

		uint8_t getTachometerCount() override;
		uint16_t updateTachometer(uint8_t index) override;
		const char* getTachometerName(uint8_t index) override;

		uint8_t getVoltageCount() override;
		float updateVoltage(uint8_t index) override;
		const char* getVoltageName(uint8_t index) override;

		/**
		 *  Ctor
		 */
		ECDeviceNUC() = default;

		/**
		 *  Device factory
		 */
		static ECDevice* detect(SMCSuperIO* sio);
	};
}

#endif // _ECDEVICE_NUC_HPP
