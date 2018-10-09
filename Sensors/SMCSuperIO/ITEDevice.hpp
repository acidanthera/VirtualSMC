//
//  ITEDevice.h
//
//  Sensors implementation for ITE SuperIO device
//
//  Based on https://github.com/kozlek/HWSensors/blob/master/SuperIOSensors/IT87xxSensors.cpp
//  @author joedm.
//

#ifndef _ITEDEVICE_H
#define _ITEDEVICE_H

#include "SuperIODevice.hpp"

namespace ITE {
	class Device : public SuperIODevice {
	private:
		/**
		 *  Tachometer
		 */
		typedef uint16_t (Device::*TachometerUpdateFunc)(uint8_t);
		static constexpr UInt8 ITE_MAX_TACHOMETER_COUNT	= 5;
		UInt32 tachometers[ITE_MAX_TACHOMETER_COUNT] = { 0 };
		
		/**
		 * Reads tachometers data. Invoked from update() only.
		 */
		virtual void updateTachometers();
		
		/**
		 *  Implementations for tachometer reading. Invoked via descriptor only.
		 */
		uint16_t tachometerRead16(uint8_t);
		uint16_t tachometerRead(uint8_t);

		/**
		 *  Struct for describing supported devices
		 */
		struct DeviceDescriptor {
			uint16_t ID;
			uint8_t tachometerCount;
			TachometerUpdateFunc updateTachometer;
		};

		/**
		 *  The descriptor instance for this device
		 */
		const DeviceDescriptor& deviceDescriptor;

	public:
		/**
		 *  Device access
		 */
		uint8_t readByte(uint8_t reg);
		void writeByte(uint8_t reg, uint8_t value);

		/**
		 *  Overrides
		 */
		virtual const char* getVendor() override { return "ITE"; }
		virtual void setupKeys(VirtualSMCAPI::Plugin &vsmcPlugin) override;
		virtual void update() override;
		virtual void initialize() override { /* Does nothing */ }
		virtual UInt16 getTachometerValue(UInt8 index) override { return tachometers[index]; }

		/**
		 *  Ctors
		 */
		Device(const DeviceDescriptor &desc) : SuperIODevice(desc.ID), deviceDescriptor(desc) {}
		Device() = delete;
		
		/**
		 *  Supported devices
		 */
		static const DeviceDescriptor _IT8512F;
		static const DeviceDescriptor _IT8705F;
		static const DeviceDescriptor _IT8712F;
		static const DeviceDescriptor _IT8716F;
		static const DeviceDescriptor _IT8718F;
		static const DeviceDescriptor _IT8720F;
		static const DeviceDescriptor _IT8721F;
		static const DeviceDescriptor _IT8726F;
		static const DeviceDescriptor _IT8620E;
		static const DeviceDescriptor _IT8628E;
		static const DeviceDescriptor _IT8686E;
		static const DeviceDescriptor _IT8728F;
		static const DeviceDescriptor _IT8752F;
		static const DeviceDescriptor _IT8771E;
		static const DeviceDescriptor _IT8772E;
		static const DeviceDescriptor _IT8792E;
	};
}

#endif // _ITEDEVICE_H
