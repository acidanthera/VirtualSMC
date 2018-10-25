//
//  WinbondDevice.hpp
//
//  Sensors implementation for Winbond SuperIO device
//
//  Based on https://github.com/kozlek/HWSensors/blob/master/SuperIOSensors/W836xxSensors.cpp
//  @author joedm
//

#ifndef _WINBONDDEVICE_HPP
#define _WINBONDDEVICE_HPP

#include "SuperIODevice.hpp"
#include "WinbondFamilyDevice.hpp"

namespace Winbond {
	// Winbond Hardware Monitor
	static constexpr uint8_t WINBOND_MAX_TACHOMETER_COUNT = 5;
	static constexpr uint8_t WINBOND_ADDRESS_REGISTER_OFFSET = 0x05;
	static constexpr uint8_t WINBOND_DATA_REGISTER_OFFSET = 0x06;
	static constexpr uint8_t WINBOND_BANK_SELECT_REGISTER = 0x4E;
	static constexpr uint16_t WINBOND_TACHOMETER[] = { 0x0028, 0x0029, 0x002A, 0x003F, 0x0553 };
	static constexpr uint16_t WINBOND_TACHOMETER_DIVISOR[] = { 0x0047, 0x004B, 0x004C, 0x0059, 0x005D };
	static constexpr uint8_t WINBOND_TACHOMETER_DIVISOR0[] = {     36,     38,     30,      8,     10 };
	static constexpr uint8_t WINBOND_TACHOMETER_DIVISOR1[] = {     37,     39,     31,      9,     11 };
	static constexpr uint8_t WINBOND_TACHOMETER_DIVISOR2[] = {      5,      6,      7,     23,     15 };

	class Device final : public WindbondFamilyDevice {
	private:
		/**
		 *  Tachometer
		 */
		uint32_t tachometers[WINBOND_MAX_TACHOMETER_COUNT] = { 0 };
		
		/**
		 * Reads tachometers data. Invoked from update() only.
		 */
		void updateTachometers();
		
		/**
		 *  Struct for describing supported devices
		 */
		struct DeviceDescriptor {
			const SuperIOModel ID;
			const uint8_t tachometerCount;
		};
		
		/**
		 *  The descriptor instance for this device
		 */
		const DeviceDescriptor& deviceDescriptor;

		/**
		 *  Supported devices
		 */
		static const DeviceDescriptor _W83627EHF;
		static const DeviceDescriptor _W83627DHG;
		static const DeviceDescriptor _W83627DHGP;
		static const DeviceDescriptor _W83667HG;
		static const DeviceDescriptor _W83667HGB;
		static const DeviceDescriptor _W83627HF;
		static const DeviceDescriptor _W83627THF;
		static const DeviceDescriptor _W83687THF;
		
	public:
		/**
		 *  Device access
		 */
		uint8_t readByte(uint16_t reg);
		void writeByte(uint16_t reg, uint8_t value);
		
		/**
		 *  Overrides
		 */
		const char* getModelName() override { return SuperIODevice::getModelName(deviceDescriptor.ID); }
		void setupKeys(VirtualSMCAPI::Plugin &vsmcPlugin) override;
		void update() override;
		uint16_t getTachometerValue(uint8_t index) override { return tachometers[index]; }
		
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

#endif // _WINBONDDEVICE_HPP
