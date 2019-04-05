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
	// ITE Environment Controller
	static constexpr uint8_t ITE_ADDRESS_REGISTER_OFFSET = 0x05;
	static constexpr uint8_t ITE_DATA_REGISTER_OFFSET = 0x06;
	// ITE Environment Controller Registers
	static constexpr uint8_t ITE_FAN_TACHOMETER_DIVISOR_REGISTER = 0x0B;
	static constexpr uint8_t ITE_FAN_TACHOMETER_REG[ITE_MAX_TACHOMETER_COUNT] = { 0x0d, 0x0e, 0x0f, 0x80, 0x82 };
	static constexpr uint8_t ITE_FAN_TACHOMETER_EXT_REG[ITE_MAX_TACHOMETER_COUNT] = { 0x18, 0x19, 0x1a, 0x81, 0x83 };
	
	class Device final : public SuperIODevice {
	private:
		/**
		 *  Tachometer
		 */
		using TachometerUpdateFunc = uint16_t (Device::*)(uint8_t);
		uint16_t tachometers[ITE_MAX_TACHOMETER_COUNT] = { 0 };
		
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
			const SuperIOModel ID;
			const uint8_t tachometerCount;
			const TachometerUpdateFunc updateTachometer;
		};

		/**
		 *  The descriptor instance for this device
		 */
		const DeviceDescriptor& deviceDescriptor;

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
		static const DeviceDescriptor _IT8688E;
		static const DeviceDescriptor _IT8795E;

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

	public:
		/**
		 *  Device access
		 */
		uint8_t readByte(uint8_t reg);
		void writeByte(uint8_t reg, uint8_t value);

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
		: SuperIODevice(desc.ID, address, port, sio), deviceDescriptor(desc) {}
		Device() = delete;
		
		/**
		 *  Device factory
		 */
		static SuperIODevice* detect(SMCSuperIO* sio);
	};
}

#endif // _ITEDEVICE_HPP
