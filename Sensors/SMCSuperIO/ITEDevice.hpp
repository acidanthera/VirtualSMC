//
//  ITEDevice.hpp
//
//  Sensors implementation for ITE SuperIO device
//
//  Based on https://github.com/kozlek/HWSensors/blob/master/SuperIOSensors/IT87xxSensors.cpp
//  @author joedm
//

#ifndef _ITEDEVICE_HPP
#define _ITEDEVICE_HPP

#include "SuperIODevice.hpp"

namespace ITE {
	
	static constexpr uint8_t ITE_MAX_TACHOMETER_COUNT = 6;
	static constexpr uint8_t ITE_MAX_VOLTAGE_COUNT = 9;

	static constexpr uint8_t ITE_ADDRESS_REGISTER_OFFSET = 0x05;
	static constexpr uint8_t ITE_DATA_REGISTER_OFFSET = 0x06;
	static constexpr uint8_t ITE_FAN_TACHOMETER_DIVISOR_REGISTER = 0x0B;
	static constexpr uint8_t FAN_MAIN_CTRL_REG = 0x13;

	static uint8_t FAN_PWM_CTRL_REG[ITE_MAX_TACHOMETER_COUNT] = { 0x15, 0x16, 0x17, 0x7f, 0xa7, 0xaf };
	static constexpr uint8_t FAN_PWM_CTRL_REG_ALT[ITE_MAX_TACHOMETER_COUNT] = { 0x15, 0x16, 0x17, 0x1e, 0x1f, 0x92 };

	static constexpr uint8_t FAN_PWM_CTRL_EXT_REG[ITE_MAX_TACHOMETER_COUNT] = { 0x63, 0x6b, 0x73, 0x7b, 0xa3, 0xab };
	static constexpr uint8_t ITE_FAN_TACHOMETER_REG[ITE_MAX_TACHOMETER_COUNT] = { 0x0d, 0x0e, 0x0f, 0x80, 0x82, 0x4c };
	static constexpr uint8_t ITE_FAN_TACHOMETER_EXT_REG[ITE_MAX_TACHOMETER_COUNT] = { 0x18, 0x19, 0x1a, 0x81, 0x83, 0x4d };
	static constexpr uint8_t ITE_VOLTAGE_REG[ITE_MAX_VOLTAGE_COUNT] = { 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28 };

	// ITE Debugger interface for EC memory snooping. Refer to "EC Memory Snoop (ECMS)" section in datasheet.
	static constexpr uint8_t ITE_I2EC_D2ADR_REG = 0x2E;
	static constexpr uint8_t ITE_I2EC_D2DAT_REG = 0x2F;
	static constexpr uint8_t ITE_I2EC_ADDR_L = 0x10;
	static constexpr uint8_t ITE_I2EC_ADDR_H = 0x11;
	static constexpr uint8_t ITE_I2EC_DATA = 0x12;

	// EC PWM function has this base address.
	static constexpr uint16_t ITE_EC_PWM_BASE = 0x1800;
	static constexpr uint16_t ITE_EC_PWM_F1TLRR = 0x1E;
	static constexpr uint16_t ITE_EC_PWM_F1TMRR = 0x1F;
	static constexpr uint16_t ITE_EC_PWM_F2TLRR = 0x20;
	static constexpr uint16_t ITE_EC_PWM_F2TMRR = 0x21;

	static constexpr uint16_t ITE_EC_GCTRL_BASE = 0x2000;
	static constexpr uint16_t ITE_EC_GCTRL_ECHIPID1 = 0x00;
	static constexpr uint16_t ITE_EC_GCTRL_ECHIPID2 = 0x01;
	static constexpr uint16_t ITE_EC_GCTRL_ECHIPVER = 0x02;

	// There are two tachometers in EC space.
	static constexpr uint8_t ITE_EC_MAX_TACHOMETER_COUNT = 2;
	static constexpr uint16_t ITE_EC_FAN_TACHOMETER_REG[ITE_EC_MAX_TACHOMETER_COUNT] = { ITE_EC_PWM_BASE + ITE_EC_PWM_F1TLRR, ITE_EC_PWM_BASE + ITE_EC_PWM_F2TLRR };
	static constexpr uint16_t ITE_EC_FAN_TACHOMETER_EXT_REG[ITE_EC_MAX_TACHOMETER_COUNT] = { ITE_EC_PWM_BASE + ITE_EC_PWM_F1TMRR, ITE_EC_PWM_BASE + ITE_EC_PWM_F2TMRR };

	
	class ITEDevice : public SuperIODevice {
		/**
		 *  Hardware access
		 */
		static inline void enter(i386_ioport_t port) {
			::outb(port, 0x87);
			::outb(port, 0x01);
			::outb(port, 0x55);
			::outb(port, 0x55);
		}
		
		static inline void leave(i386_ioport_t port) {
			::outb(port, SuperIOConfigControlRegister);
			::outb(port + 1, 0x02);
		}

	protected:
		/**
		 *  Implementations for tachometer reading.
		 */
		uint16_t tachometerRead(uint8_t);
		uint16_t tachometerRead8bit(uint8_t);
		uint16_t tachometerReadEC(uint8_t);

		void tachometerWrite(uint8_t index, uint8_t value, bool enabled);
		void tachometerSaveDefault(uint8_t);
		void tachometerRestoreDefault(uint8_t);
		/**
		 * This is a stub to keep the code generator happy since updateTargets is overridden.
		 */
		void updateTargets() override;

		/**
		 * Reads voltage data. Invoked from update() only.
		 */
		float voltageRead(uint8_t);
		float voltageReadOld(uint8_t);

	public:
		/**
		 *  Device access
		 */
		uint8_t readByte(uint8_t reg);
		void writeByte(uint8_t reg, uint8_t value);
		uint8_t readByteEC(uint16_t addr);
		void writeByteEC(uint16_t addr, uint8_t value);

		/**
		 *  Overrides
		 */
		void setupKeys(VirtualSMCAPI::Plugin &vsmcPlugin) override;

		/**
		 *  Ctor
		 */
		ITEDevice() = default;

		/**
		 *  Device factory helper
		 */
		static SuperIODevice* probePort(i386_ioport_t port, SMCSuperIO* sio);

		/**
		 *  Device factory
		 */
		static SuperIODevice* detect(SMCSuperIO* sio);
	};
}

#endif // _ITEDEVICE_HPP
