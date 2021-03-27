//
//  ECDeviceDebug.hpp
//  SMCSuperIO
//
//  Copyright © 2021 vit9696. All rights reserved.
//
//

#ifndef _ECDEVICE_DEBUG_HPP
#define _ECDEVICE_DEBUG_HPP

#include "SuperIODevice.hpp"
#include "ECDevice.hpp"

namespace EC {

	class ECDeviceDebug : public ECDevice {
		uint32_t windowSize {0};
		uint32_t windowBase {0};

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
		ECDeviceDebug(uint32_t base, uint32_t size) : windowBase(base), windowSize(size) {
			supportsMMIO = true;
			supportsPMIO = true;
		}

		/**
		 *  Device factory
		 */
		static ECDevice* detect(SMCSuperIO* sio, const char *name);
	};
}

#endif // _ECDEVICE_NUC_HPP
