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
	
	static constexpr uint8_t ITE_MAX_TACHOMETER_COUNT = 5;
	static constexpr uint8_t ITE_MAX_VOLTAGE_COUNT = 9;

	static constexpr uint8_t ITE_ADDRESS_REGISTER_OFFSET = 0x05;
	static constexpr uint8_t ITE_DATA_REGISTER_OFFSET = 0x06;
	static constexpr uint8_t ITE_FAN_TACHOMETER_DIVISOR_REGISTER = 0x0B;
	static constexpr uint8_t ITE_FAN_TACHOMETER_REG[ITE_MAX_TACHOMETER_COUNT] = { 0x0d, 0x0e, 0x0f, 0x80, 0x82 };
	static constexpr uint8_t ITE_FAN_TACHOMETER_EXT_REG[ITE_MAX_TACHOMETER_COUNT] = { 0x18, 0x19, 0x1a, 0x81, 0x83 };
	static constexpr uint8_t ITE_VOLTAGE_REG[ITE_MAX_VOLTAGE_COUNT] = { 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28 };
	
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
		/**
		 *  Tachometers
		 */
		uint16_t tachometers[ITE_MAX_TACHOMETER_COUNT] = { 0 };
		/**
		 *  Voltages
		 */
		float voltages[ITE_MAX_VOLTAGE_COUNT] = { 0.0 };
	protected:
		/**
		 *  Implementations for tachometer reading.
		 */
		uint16_t tachometerRead(uint8_t);
		uint16_t tachometerRead8bit(uint8_t);
		void setTachometerValue(uint8_t index, uint16_t value) override {
			if (index < getTachometerCount() && index < ITE_MAX_TACHOMETER_COUNT) {
				tachometers[index] = value;
			}
		}
		uint16_t getTachometerValue(uint8_t index) override {
			if (index < getTachometerCount() && index < ITE_MAX_TACHOMETER_COUNT) {
				return tachometers[index];
			}
			return 0;
		}
		/**
		 * Reads voltage data. Invoked from update() only.
		 */
		float voltageRead(uint8_t);
		float voltageReadOld(uint8_t);
		void setVoltageValue(uint8_t index, float value) override {
			if (index < getVoltageCount() && index < ITE_MAX_VOLTAGE_COUNT) {
				voltages[index] = value;
			}
		}
		float getVoltageValue(uint8_t index) override {
			if (index < getVoltageCount() && index < ITE_MAX_VOLTAGE_COUNT) {
				return voltages[index];
			}
			return 0.0f;
		}
	public:
		/**
		 *  Device access
		 */
		uint8_t readByte(uint8_t reg);
		void writeByte(uint8_t reg, uint8_t value);

		/**
		 *  Overrides
		 */
		void setupKeys(VirtualSMCAPI::Plugin &vsmcPlugin) override;

		/**
		 *  Ctor
		 */
		ITEDevice() = default;
		
		/**
		 *  Device factory
		 */
		static SuperIODevice* detect(SMCSuperIO* sio);
	};
}

#endif // _ITEDEVICE_HPP
