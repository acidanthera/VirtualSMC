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
	// NCT6687 specific data
	// RPM register order follows the Linux nct6687 driver's alternative (msi_alt1)
	// fan config, verified on MSI MS-7E66 (NCT6687D-R): CPU Fan, Pump Fan,
	// System Fan #1..#6 at 0x15E..0x154.
	static constexpr uint8_t NUVOTON_6687_FAN_NUMS				= 8;
	static constexpr uint16_t NUVOTON_6687_FAN_REGS[] = {
	0x140, 0x142, // CPU Fan, Pump Fan
    	0x15E, 0x15C, 0x15A, 0x158, 0x156, 0x154 // System Fan #1–6
};
	// Temperature registers: NCT6687_REG_TEMP(x) = 0x100 + x * 2 (7 channels).
	static constexpr uint8_t NUVOTON_6687_TEMP_NUMS				= 7;
	static constexpr uint16_t NUVOTON_6687_TEMP_BASE			= 0x100;

	/*
	 * NCT6687D-R fan control (manual PWM).
	 *
	 * Register map / handshake reverse-engineered by the LibreHardwareMonitor
	 * project (Nct677X.cs, Chip.NCT6687DR branch), cross-checked against the
	 * Linux nct6683/nct6687 drivers and verified end-to-end on an MSI
	 * B850M GAMING PLUS WIFI (MS-7E66, customer_id 0x020D):
	 *   - duty (write) : CPU/Pump 0xA28..0xA29 ; SYS1..6 0x265..0x260 (reversed)
	 *   - duty (read)  : CPU/Pump 0x160..0x161 ; SYS1..6 0xE05..0xE00 (reversed)
	 *   - manual bit   : CPU/Pump.. in 0xA00 (bit 0..3) ; SYS1..6 in 0x80F (bit 7..2)
	 *   - handshake    : 0xA01 REQ=0x80 / DONE=0x40, status 0xCF8
	 * A write is only accepted while the EC is in the fan-config phase.
	 */
	static constexpr uint16_t NUVOTON_6687_REG_ENGINE_STS		= 0xCF8;
	static constexpr uint16_t NUVOTON_6687_REG_CFG_CTRL			= 0xA01;
	static constexpr uint8_t  NUVOTON_6687_CFG_LOCK				= 0x40;
	static constexpr uint8_t  NUVOTON_6687_CFG_PHASE			= 0x08;
	static constexpr uint8_t  NUVOTON_6687_CFG_INVALID			= 0x10;
	static constexpr uint8_t  NUVOTON_6687_CFG_CHECK_DONE		= 0x20;
	static constexpr uint8_t  NUVOTON_6687_CFG_REQ				= 0x80;
	static constexpr uint8_t  NUVOTON_6687_CFG_DONE				= 0x40;
	/* customer id (16-bit, big-endian) live in EC 0x602/0x603; MS-7E66 = 0x020D */
	static constexpr uint16_t NUVOTON_6687_REG_CUSTOMER_ID		= 0x602;
	static constexpr uint16_t NUVOTON_6687_CUSTOMER_ID_MSI4		= 0x020D;

	/* Fan-control table entry for one header. */
	struct Nuvoton6687Fan {
		uint16_t rpm;		/* 16-bit tachometer */
		uint16_t dutyRead;	/* 8-bit duty readback */
		uint16_t dutyWrite;	/* 8-bit direct-PWM command */
		uint16_t modeReg;	/* manual-mode register */
		uint8_t  modeBit;	/* manual-mode bit in modeReg */
	};

	/*
	 * Verified map for NCT6687D-R (MSI AM5/LGA1851). Index == tachometer
	 * index, i.e. the order of NUVOTON_6687_FAN_REGS. dutyWrite == 0 means
	 * the header has no verified write path and is not manually controllable.
	 */
	static constexpr Nuvoton6687Fan NUVOTON_6687_FAN_CTRL[NUVOTON_6687_FAN_NUMS] = {
		{ 0x140, 0x160, 0xA28, 0xA00, 0 }, // CPU Fan
		{ 0x142, 0x161, 0xA29, 0xA00, 1 }, // Pump Fan
		{ 0x15E, 0xE05, 0x265, 0x80F, 7 }, // System Fan #1
		{ 0x15C, 0xE04, 0x264, 0x80F, 6 }, // System Fan #2
		{ 0x15A, 0xE03, 0x263, 0x80F, 5 }, // System Fan #3
		{ 0x158, 0xE02, 0x262, 0x80F, 4 }, // System Fan #4
		{ 0x156, 0xE01, 0x261, 0x80F, 3 }, // System Fan #5
		{ 0x154, 0xE00, 0x260, 0x80F, 2 }, // System Fan #6
	};

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
		uint16_t tachometerRead6687(uint8_t);

		/**
		 * Polls tachometers and keeps the running min/max RPM limits in sync.
		 * Nuvoton chips do not publish a usable firmware minimum/maximum RPM
		 * range, so the F0Mn/F0Mx values are derived from the observed readings.
		 * Invoked from update() only.
		 */
		void updateTachometers() override;

		/**
		 * Reads voltage data. Invoked from update() only.
		 */
		float voltageRead(uint8_t);
		float voltageRead6775(uint8_t);
		float voltageRead6683(uint8_t);

		/**
		 * Reads temperature data for NCT6687. Invoked from update() only.
		 */
		float temperatureRead6687(uint8_t);

		/**
		 * Mapping monitor regs to voltage regs for 6683 devices.
		 */
		void voltageMapping6683();

		/**
		 * Registers temperature SMC keys. Overridden by generated device
		 * classes that declare a Temperature sensor section with SmcKey.
		 */
		virtual void setupTemperatureKeys(VirtualSMCAPI::Plugin &vsmcPlugin) {}

		/**
		 * True for devices whose fan headers accept direct manual PWM control
		 * (NCT6687D-R). Overridden by the generated device class; see
		 * NCT6687D.plist (FanControl key).
		 */
		virtual bool fanControlSupported() { return false; }

		/**
		 * Set once fanControlSupported() and the EC customer id check pass.
		 */
		bool fanControlEnabled {false};

		/**
		 * Applies the current manual/target fan state to the EC. Invoked from
		 * the SMC key update handlers (F0Md/F0Tg) when fan control is enabled.
		 */
		void updateTargets() override;

		/**
		 * Sets raw duty (0..255) on a header. Saves prior EC state on first
		 * use so restoreFanControl() can put it back. Returns true on success.
		 */
		bool setFanDuty(uint8_t index, uint8_t value);

		/**
		 * Enters the EC fan-configuration phase. Returns true once the fan
		 * registers are unlocked and writable.
		 */
		bool startFanConfig();

		/**
		 * Commits the fan configuration; returns false if the EC rejected it.
		 */
		bool finishFanConfig();

		/**
		 * Restores the mode bit and duty captured before manual control.
		 */
		bool restoreFanControl(uint8_t index);

		/**
		 * Serialises all EC-window access (page/index/data). The window is
		 * shared, so the timer's sensor reads (readByte6683) must not interleave
		 * with the fan-config handshake from the SMC-key thread. Recursive, so
		 * the handshake may hold it across nested accessor calls.
		 */
		IORecursiveLock *ecLock {nullptr};

		uint8_t savedModeBit[NUVOTON_6687_FAN_NUMS] = { };
		uint8_t savedDuty[NUVOTON_6687_FAN_NUMS] = { };
		bool    hasSaved[NUVOTON_6687_FAN_NUMS] = { };

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
		 * Allocates the EC-window lock (fan-control path).
		 */
		void initialize(uint16_t address, i386_ioport_t port, SMCSuperIO* sio) override;

		/**
		 *  Ctor
		 */
		NuvotonDevice() = default;
	};
}

#endif // _NUVOTONDEVICE_HPP
