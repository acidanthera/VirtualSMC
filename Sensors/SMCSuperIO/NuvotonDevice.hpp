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
	static constexpr uint8_t NUVOTON_MAX_TACHOMETER_COUNT		= 7;
	static constexpr uint8_t NUVOTON_ADDRESS_REGISTER_OFFSET     = 0x05;
	static constexpr uint8_t NUVOTON_DATA_REGISTER_OFFSET        = 0x06;
	static constexpr uint8_t NUVOTON_BANK_SELECT_REGISTER        = 0x4E;
	static constexpr uint8_t NUVOTON_REG_ENABLE                  = 0x30;
	static constexpr uint8_t NUVOTON_HWMON_IO_SPACE_LOCK         = 0x28;
	static constexpr uint16_t NUVOTON_VENDOR_ID                  = 0x5CA3;
	
	static constexpr uint16_t NUVOTON_VENDOR_ID_HIGH_REGISTER    = 0x804F;
	static constexpr uint16_t NUVOTON_VENDOR_ID_LOW_REGISTER     = 0x004F;
	
	static constexpr uint16_t NUVOTON_3_FANS_RPM_REGS[] = { 0x656, 0x658, 0x65A };
	static constexpr uint16_t NUVOTON_5_FANS_RPM_REGS[] = { 0x4C0, 0x4C2, 0x4C4, 0x4C6, 0x4C8 };
	static constexpr uint16_t NUVOTON_6_FANS_RPM_REGS[] = { 0x4C0, 0x4C2, 0x4C4, 0x4C6, 0x4C8, 0x4CA };
	static constexpr uint16_t NUVOTON_7_FANS_RPM_REGS[] = { 0x4C0, 0x4C2, 0x4C4, 0x4C6, 0x4C8, 0x4CA, 0x4CE };

	class Device final : public WindbondFamilyDevice {
	private:
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
		 *  Struct for describing supported devices
		 */
		struct DeviceDescriptor {
			const SuperIOModel ID;
			/* Maximum tachometer sensors this Nuvoton device has */
			const uint8_t tachometerCount;
			/* A pointer to array of registers to read tachometer values from */
			const uint16_t *tachometerRpmRegisters;
			/* Init proc */
			const InitializeFunc initialize;
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
		const char* getModelName() override { return SuperIODevice::getModelName(deviceDescriptor.ID); }
		void setupKeys(VirtualSMCAPI::Plugin &vsmcPlugin) override;
		void update() override;
		void powerStateChanged(unsigned long state) override;
		uint16_t getTachometerValue(uint8_t index) override { return tachometers[index]; }

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
		static const DeviceDescriptor _NCT6798D;
		
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
