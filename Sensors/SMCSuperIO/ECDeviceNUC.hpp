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
	// Intel EC V1 - MKKBLY35 / MKKBLi5v
	// Intel EC V2 - AYAPLCEL
	// Intel EC V3 - BNKBL357
	// Intel EC V4 - CYCNLi35
	// Intel EC V5 - HNKBLi70
	// Intel EC V6 - DNKBLi7v / DNKBLi30
	// Intel EC V7 - JYGLKCPX
	// Intel EC V8 - BECFL357 (NUC8i7BE, NUC8i5BE, NUC8i3BE)
	// Intel EC V9 - FNCML357 / PNWHL357 / PNWHLV57 / QNCFLX70 / QXCFL579 / CHAPLCEL / CBWHL357 / CBWHLMIV / EBTGL357 / EBTGLMIV
	// Intel EC VA - PATGL357 / TNTGLV57 / TNTGL357 / PHTGL579
	// Intel EC VB - INWHL357

	// Information about Intel NUC EC address space.
	static constexpr uint32_t R_NUC_EC_V1_ACTIVATION = 0x41C; // 32-bit.
	static constexpr uint32_t B_NUC_EC_V1_ACTIVATION_EN = 1;  // OR with this if not set.
	static constexpr uint32_t R_NUC_EC_V1_CPU_FAN = 0x0E;

	static constexpr uint32_t B_NUC_EC_V8_PCH_TEMP_U8         = 0x503;
	static constexpr uint32_t B_NUC_EC_V8_VR_TEMP_U8          = 0x504;
	static constexpr uint32_t B_NUC_EC_V8_DIMM_TEMP_U8        = 0x505;
	static constexpr uint32_t B_NUC_EC_V8_MOTHERBOARD_TEMP_U8 = 0x506;

	static constexpr uint32_t B_NUC_EC_V8_CPUFAN_U16          = 0x508;

	static constexpr uint32_t B_NUC_EC_V8_V5VSB_U16           = 0x510;
	static constexpr uint32_t B_NUC_EC_V8_VCORE_U16           = 0x512;
	static constexpr uint32_t B_NUC_EC_V8_VDIMM_U16           = 0x514;
	static constexpr uint32_t B_NUC_EC_V8_VCCIO_U16           = 0x518;

	static constexpr uint32_t NUC_EC_SPACE_SIZE = 0x580;

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
