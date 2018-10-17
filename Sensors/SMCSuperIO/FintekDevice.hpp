//
//  FintekDevice.hpp
//
//  Sensors implementation for FINTEK SuperIO device
//
//  Based on https://github.com/kozlek/HWSensors/blob/master/SuperIOSensors/F718xxSensors.cpp
//  @author joedm
//

#ifndef _FINTEKDEVICE_HPP
#define _FINTEKDEVICE_HPP

#include "SuperIODevice.hpp"
#include "WinbondFamilyDevice.hpp"

namespace Fintek {
	
	constexpr uint8_t FINTEK_MAX_TACHOMETER_COUNT = 4;
	constexpr uint8_t FINTEK_ADDRESS_REGISTER_OFFSET = 0x05;
	constexpr uint8_t FINTEK_DATA_REGISTER_OFFSET = 0x06;
	
	// Hardware Monitor Registers
	constexpr uint8_t FINTEK_TEMPERATURE_CONFIG_REG = 0x69;
	constexpr uint8_t FINTEK_TEMPERATURE_BASE_REG = 0x70;
	constexpr uint8_t FINTEK_VOLTAGE_BASE_REG = 0x20;
	constexpr uint8_t FINTEK_FAN_TACHOMETER_REG[] = { 0xA0, 0xB0, 0xC0, 0xD0 };
	constexpr uint8_t FINTEK_TEMPERATURE_EXT_REG[] = { 0x7A, 0x7B, 0x7C, 0x7E };

	class Device : public WindbondFamilyDevice {
	private:
		/**
		 *  Tachometer
		 */
		using TachometerUpdateFunc = uint16_t (Device::*)(uint8_t);
		uint16_t tachometers[FINTEK_MAX_TACHOMETER_COUNT] = { 0 };
		
		/**
		 * Reads tachometers data. Invoked from update() only.
		 */
		void updateTachometers();
	
		/**
		 *  Struct for describing supported devices
		 */
		struct DeviceDescriptor {
			SuperIOModel ID;
			uint8_t tachometerCount;
		};
		
		/**
		 *  The descriptor instance for this device
		 */
		const DeviceDescriptor& deviceDescriptor;
		
		/**
		 *  Supported devices
		 */
		static const DeviceDescriptor _F71858;
		static const DeviceDescriptor _F71862;
		static const DeviceDescriptor _F71868A;
		static const DeviceDescriptor _F71869;
		static const DeviceDescriptor _F71869A;
		static const DeviceDescriptor _F71882;
		static const DeviceDescriptor _F71889AD;
		static const DeviceDescriptor _F71889ED;
		static const DeviceDescriptor _F71889F;
		static const DeviceDescriptor _F71808E;

	public:
		/**
		 *  Device access
		 */
		uint8_t readByte(uint8_t reg);
		
		/**
		 *  Overrides
		 */
		virtual const char* getModelName() override { return SuperIODevice::getModelName(deviceDescriptor.ID); }
		virtual void setupKeys(VirtualSMCAPI::Plugin &vsmcPlugin) override;
		virtual void update() override;
		virtual uint16_t getTachometerValue(uint8_t index) override { return tachometers[index]; }
		
		/**
		 *  Ctors
		 */
		Device(const DeviceDescriptor &desc, uint16_t address, i386_ioport_t port, SMCSuperIO* sio)
		: WindbondFamilyDevice(desc.ID, address, port, sio), deviceDescriptor(desc) {}
		Device() = delete;
		
		/**
		 *  Device factory
		 */
		static SuperIODevice* detect(SMCSuperIO* sio);

		/**
		 *  Device factory helper
		 */
		static const DeviceDescriptor* detectModel(uint16_t id, uint8_t &ldn);
	};
}

#endif // _FINTEKDEVICE_HPP
