//
//  NuvotonDevice.hpp
//
//  Sensors implementation for Nuvoton SuperIO device
//
//  Based on https://github.com/kozlek/HWSensors/blob/master/SuperIOSensors/NCT677xSensors.cpp
//  @author joedm
//

#ifndef _NUVOTONDEVICE_HPP
#define _NUVOTONDEVICE_HPP

#include "SuperIODevice.hpp"
#include "WinbondFamilyDevice.hpp"

namespace Nuvoton {
	
	class Device : public WindbondFamilyDevice {
	private:
		static constexpr uint8_t NUVOTON_MAX_TACHOMETER_COUNT		= 7;
		static constexpr uint8_t NUVOTON_ADDRESS_REGISTER_OFFSET     = 0x05;
		static constexpr uint8_t NUVOTON_DATA_REGISTER_OFFSET        = 0x06;
		static constexpr uint8_t NUVOTON_BANK_SELECT_REGISTER        = 0x4E;
		static constexpr uint8_t NUVOTON_REG_ENABLE                  = 0x30;
		static constexpr uint8_t NUVOTON_HWMON_IO_SPACE_LOCK         = 0x28;
		static constexpr uint16_t NUVOTON_VENDOR_ID                  = 0x5CA3;
		
		static constexpr uint16_t NUVOTON_VENDOR_ID_HIGH_REGISTER    = 0x804F;
		static constexpr uint16_t NUVOTON_VENDOR_ID_LOW_REGISTER     = 0x004F;
		
		static constexpr int32_t RPM_THRESHOLD1 = (int32_t)(1.35e6 / 0xFFFF);
		static constexpr int32_t RPM_THRESHOLD2 = (int32_t)(1.35e6 / 0x1FFF);

		/**
		 *  Initialize
		 */
		using InitializeFunc = void (Device::*)();
		void initialize679xx();

		/**
		 *  Tachometer
		 */
		uint16_t tachometers[NUVOTON_MAX_TACHOMETER_COUNT] = { 0 };

		/**
		 * Reads tachometers data. Invoked from update() only.
		 */
		void updateTachometers();
		
		/**
		 *  Implementation for tachometer reading. Invoked via descriptor only.
		 */
		uint16_t tachometerReadDefault(uint8_t);
		
		/**
		 *  Struct for describing supported devices
		 */
		struct DeviceDescriptor {
			SuperIOModel ID;
			/* Maximum tachometer sensors this Nuvoton device has */
			uint8_t tachometerCount;
			/* Tachometer RPM threshold: values less than this are invalid */
			uint32_t tachometerMinRPM;
			/* Base register for tachometers reading */
			uint16_t tachometerRpmBaseRegister;
			/* Init proc */
			InitializeFunc initialize;
		};
		
		/**
		 *  The descriptor instance for this device
		 */
		const DeviceDescriptor& deviceDescriptor;
		
	public:
		/**
		 * Reads a byte from device's register
		 */
		uint8_t readByte(uint16_t reg);
		
		/**
		 * Writes a byte into device's register
		 */
		void writeByte(uint16_t reg, uint8_t value);
		
		/**
		 *  Overrides
		 */
		virtual const char* getModelName() override { return SuperIODevice::getModelName(deviceDescriptor.ID); }
		virtual void setupKeys(VirtualSMCAPI::Plugin &vsmcPlugin) override;
		virtual void update() override;
		virtual void powerStateChanged(unsigned long state) override;
		virtual uint16_t getTachometerValue(uint8_t index) override { return tachometers[index]; }

		/**
		 *  Ctors
		 */
		Device(const DeviceDescriptor &desc, uint16_t address, i386_ioport_t port, SMCSuperIO* sio)
			: WindbondFamilyDevice(desc.ID, address, port, sio), deviceDescriptor(desc) {}
		Device() = delete;
		
		/**
		 *  Supported devices
		 */
		static const DeviceDescriptor _NCT6771F;
		static const DeviceDescriptor _NCT6776F;
		static const DeviceDescriptor _NCT6779D;
		static const DeviceDescriptor _NCT6791D;
		static const DeviceDescriptor _NCT6792D;
		static const DeviceDescriptor _NCT6793D;
		static const DeviceDescriptor _NCT6795D;
		static const DeviceDescriptor _NCT6796D;
		
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

#endif // _NUVOTONDEVICE_HPP
