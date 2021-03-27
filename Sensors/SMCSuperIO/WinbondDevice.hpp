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
	static constexpr uint8_t WINBOND_MAX_VOLTAGE_COUNT = 10;
	static constexpr uint8_t WINBOND_ADDRESS_REGISTER_OFFSET = 0x05;
	static constexpr uint8_t WINBOND_DATA_REGISTER_OFFSET = 0x06;
	static constexpr uint8_t WINBOND_BANK_SELECT_REGISTER = 0x4E;
	static constexpr uint16_t WINBOND_TACHOMETER[] = { 0x0028, 0x0029, 0x002A, 0x003F, 0x0553 };
	static constexpr uint16_t WINBOND_TACHOMETER_DIVISOR[] = { 0x0047, 0x004B, 0x004C, 0x0059, 0x005D };
	static constexpr uint8_t WINBOND_TACHOMETER_DIVISOR0[] = {     36,     38,     30,      8,     10 };
	static constexpr uint8_t WINBOND_TACHOMETER_DIVISOR1[] = {     37,     39,     31,      9,     11 };
	static constexpr uint8_t WINBOND_TACHOMETER_DIVISOR2[] = {      5,      6,      7,     23,     15 };
	static constexpr uint16_t WINBOND_VOLTAGE_REG[] = { 0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0550, 0x0551, 0x0552 };
	static constexpr uint16_t WINBOND_VOLTAGE_REG1[] = { 0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0550, 0x0551, /* next are the dummy values */ 0x0551, 0x0551, 0x0551 };
	static constexpr uint16_t WINBOND_VOLTAGE_VBAT = 0x0551;

	class WinbondDevice : public WindbondFamilyDevice {
	protected:
		/**
		 * This is a stub to keep the code generator happy since updateTachometers is overridden.
		 */
		uint16_t tachometerRead(uint8_t) { return 0; };
		void updateTachometers() override;

		/**
		 * Reads voltage data. Invoked from update() only.
		 */
		float voltageRead(uint8_t);
		float voltageReadVrmCheck(uint8_t);

	public:
		/**
		 *  Device access
		 */
		uint8_t readByte(uint16_t reg);
		void writeByte(uint16_t reg, uint8_t value);
		
		/**
		 *  Overrides
		 */
		void setupKeys(VirtualSMCAPI::Plugin &vsmcPlugin) override;
		
		/**
		 *  Ctor
		 */
		WinbondDevice() = default;
	};
}

#endif // _WINBONDDEVICE_HPP
