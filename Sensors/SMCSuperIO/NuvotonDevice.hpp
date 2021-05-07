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
	static constexpr uint8_t NUVOTON_ADDRESS_REGISTER_OFFSET     = 0x05;
	static constexpr uint8_t NUVOTON_DATA_REGISTER_OFFSET        = 0x06;
	static constexpr uint8_t NUVOTON_BANK_SELECT_REGISTER        = 0x4E;
	static constexpr uint8_t NUVOTON_REG_ENABLE                  = 0x30;
	static constexpr uint8_t NUVOTON_HWMON_IO_SPACE_LOCK         = 0x28;
	static constexpr uint16_t NUVOTON_VENDOR_ID                  = 0x5CA3;
	static constexpr uint8_t NUVOTON_MAX_TACHOMETER_COUNT		 = 7;
	static constexpr uint16_t NUVOTON_FAN_6776_REGS[] = { 0x656, 0x658, 0x65A, 0x65C, 0x65E };
	static constexpr uint16_t NUVOTON_FAN_REGS[] = { 0x4C0, 0x4C2, 0x4C4, 0x4C6, 0x4C8, 0x4CA, 0x4CE };
	static constexpr uint8_t NUVOTON_MAX_VOLTAGE_COUNT		 	= 16;
	static constexpr uint16_t NUVOTON_VOLTAGE_6775_REGS[] = { 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x550, 0x551, 0x552	};
	static constexpr uint16_t NUVOTON_VOLTAGE_REGS[] = { 0x480, 0x481, 0x482, 0x483, 0x484, 0x485, 0x486, 0x487, 0x488, 0x489, 0x48A, 0x48B, 0x48C, 0x48D, 0x48E, 0x48F};
	static constexpr uint16_t NUVOTON_VBAT_REG 					= 0x488;
	static constexpr uint16_t NUVOTON_VBAT_6775_REG				= 0x551;
	static constexpr uint16_t NUVOTON_VBAT_CONTROL_REG			= 0x5D;
	// NCT6683 specific data
	static constexpr uint8_t NUVOTON_6683_PAGE_REGISTER_OFFSET	= 0x04;
	static constexpr uint8_t NUVOTON_6683_INDEX_REGISTER_OFFSET	= 0x05;
	static constexpr uint8_t NUVOTON_6683_DATA_REGISTER_OFFSET	= 0x06;
	static constexpr uint8_t NUVOTON_6683_EVENT_REGISTER_OFFSET	= 0x07;
	static constexpr uint8_t NUVOTON_6683_MON_NUMS				= 32;
	static constexpr uint16_t NUVOTON_6683_MON_REGISTER_OFFSET	= 0x100;
	static constexpr uint16_t NUVOTON_6683_MON_CFG_OFFSET		= 0x1A0;
	static constexpr uint16_t NUVOTON_6683_MON_VOLTAGE_START	= 0x60;
	static constexpr uint8_t NUVOTON_6683_FAN_NUMS				= 16;
	static constexpr uint16_t NUVOTON_6683_FAN_REGS[] = { 0x142, 0x140, 0x144, 0x146, 0x148, 0x14A, 0x14C, 0x14E, 0x150, 0x152, 0x154, 0x156, 0x158, 0x15A, 0x15C, 0x15E };
	static constexpr uint8_t NUVOTON_6683_VOLTAGE_NUMS			= 23;

	class NuvotonDevice : public WindbondFamilyDevice {

	protected:
		/**
		 * Mapped voltage register for NCT6683
		 */
		uint16_t nuvoton6683VoltageRegs[NUVOTON_6683_VOLTAGE_NUMS] = {0};

		/**
		 * On power-on init for 679XX devices.
		 */
		void onPowerOn679xx();
		/**
		 * Reads tachometers data. Invoked from update() only.
		 */
		uint16_t tachometerRead(uint8_t);
		uint16_t tachometerRead6776(uint8_t);
		uint16_t tachometerRead6683(uint8_t);

		/**
		 * Reads voltage data. Invoked from update() only.
		 */
		float voltageRead(uint8_t);
		float voltageRead6775(uint8_t);
		float voltageRead6683(uint8_t);

		/**
		 * Mapping monitor regs to voltage regs for 6683 devices.
		 */
		void voltageMapping6683();

	public:
		/**
		 * Reads a byte from device's register
		 */
		uint8_t readByte(uint16_t reg);
		uint8_t readByte6683(uint16_t reg);

		/**
		 * Writes a byte into device's register
		 */
		void writeByte(uint16_t reg, uint8_t value);
		void writeByte6683(uint16_t reg, uint8_t value);

		/**
		 *  Overrides
		 */
		void setupKeys(VirtualSMCAPI::Plugin &vsmcPlugin) override;

		/**
		 *  Ctor
		 */
		NuvotonDevice() = default;
	};
}

#endif // _NUVOTONDEVICE_HPP
