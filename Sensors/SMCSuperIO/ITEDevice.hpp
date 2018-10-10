//
//  ITEDevice.hpp
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
	
	constexpr uint8_t ITE_MAX_TACHOMETER_COUNT	= 5;
	// ITE Environment Controller
	constexpr uint8_t ITE_ADDRESS_REGISTER_OFFSET = 0x05;
	constexpr uint8_t ITE_DATA_REGISTER_OFFSET = 0x06;
	// ITE Environment Controller Registers
	//	const uint8_t ITE_CONFIGURATION_REGISTER					= 0x00;
	//	const uint8_t ITE_TEMPERATURE_BASE_REG					= 0x29;
	//	const uint8_t ITE_VENDOR_ID_REGISTER						= 0x58;
	constexpr uint8_t ITE_FAN_TACHOMETER_DIVISOR_REGISTER = 0x0B;
	constexpr uint8_t ITE_FAN_TACHOMETER_REG[ITE_MAX_TACHOMETER_COUNT] = { 0x0d, 0x0e, 0x0f, 0x80, 0x82 };
	constexpr uint8_t ITE_FAN_TACHOMETER_EXT_REG[ITE_MAX_TACHOMETER_COUNT] = { 0x18, 0x19, 0x1a, 0x81, 0x83 };
	//	const uint8_t ITE_VOLTAGE_BASE_REG						= 0x20;
	
	class Device : public SuperIODevice {
	private:
		/**
		 *  Tachometer
		 */
		using TachometerUpdateFunc = uint16_t (Device::*)(uint8_t);
		UInt32 tachometers[ITE_MAX_TACHOMETER_COUNT] = { 0 };
		
		/**
		 * Reads tachometers data. Invoked from update() only.
		 */
		void updateTachometers();
		
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
