//
//  ITEDevice.cpp
//
//  Sensors implementation for ITE SuperIO device
//
//  Based on https://github.com/kozlek/HWSensors/blob/master/SuperIOSensors/IT87xxSensors.cpp
//  @author joedm.
//

#include "ITEDevice.hpp"
#include "SMCSuperIO.hpp"

namespace ITE {
	
	// ITE Environment Controller
	const UInt8 ITE_ADDRESS_REGISTER_OFFSET					= 0x05;
	const UInt8 ITE_DATA_REGISTER_OFFSET					= 0x06;
	
	// ITE Environment Controller Registers
//	const UInt8 ITE_CONFIGURATION_REGISTER					= 0x00;
//	const UInt8 ITE_TEMPERATURE_BASE_REG					= 0x29;
//	const UInt8 ITE_VENDOR_ID_REGISTER						= 0x58;
	const UInt8 ITE_FAN_TACHOMETER_DIVISOR_REGISTER         = 0x0B;
	const UInt8 ITE_FAN_TACHOMETER_REG[5]					= { 0x0d, 0x0e, 0x0f, 0x80, 0x82 };
	const UInt8 ITE_FAN_TACHOMETER_EXT_REG[5]				= { 0x18, 0x19, 0x1a, 0x81, 0x83 };
//	const UInt8 ITE_VOLTAGE_BASE_REG						= 0x20;
	
	uint16_t Device::tachometerRead(uint8_t index) {
		uint16_t value = readByte(ITE_FAN_TACHOMETER_REG[index]);
		int divisor = 2;
		if (index < 2) {
			divisor = 1 << ((readByte(ITE_FAN_TACHOMETER_DIVISOR_REGISTER) >> (3 * index)) & 0x7);
		}
		return value > 0 && value < 0xff ? 1.35e6f / (float)(value * divisor) : 0;
	}

	uint16_t Device::tachometerRead16(uint8_t index) {
		uint16_t value = readByte(ITE_FAN_TACHOMETER_REG[index]);
		value |= readByte(ITE_FAN_TACHOMETER_EXT_REG[index]) << 8;
		return value > 0x3f && value < 0xffff ? (float)(1.35e6f + value) / (float)(value * 2) : 0;
	}

	uint8_t Device::readByte(uint8_t reg)
	{
		UInt16 address = getDeviceAddress();
		outb(address + ITE_ADDRESS_REGISTER_OFFSET, reg);
		return inb(address + ITE_DATA_REGISTER_OFFSET);
	}
	
	void Device::writeByte(uint8_t reg, uint8_t value)
	{
		UInt16 address = getDeviceAddress();
		outb(address + ITE_ADDRESS_REGISTER_OFFSET, reg);
		outb(address + ITE_DATA_REGISTER_OFFSET, value);
	}
	
	void Device::setupKeys(VirtualSMCAPI::Plugin &vsmcPlugin) {
		VirtualSMCAPI::addKey(KeyFNum, vsmcPlugin.data, VirtualSMCAPI::valueWithUint8(deviceDescriptor.tachometerCount, nullptr, SMC_KEY_ATTRIBUTE_CONST | SMC_KEY_ATTRIBUTE_READ));
		for (UInt8 index = 0; index < deviceDescriptor.tachometerCount; ++index) {
			VirtualSMCAPI::addKey(KeyF0Ac(index), vsmcPlugin.data, VirtualSMCAPI::valueWithFp(0, SmcKeyTypeFpe2, new TachometerKey(getSmcSuperIO(), this, index)));
		}
	}

	void Device::update() {
		IOSimpleLockLock(getSmcSuperIO()->counterLock);
		updateTachometers();
		IOSimpleLockUnlock(getSmcSuperIO()->counterLock);
	}
	
	void Device::updateTachometers() {
		for (UInt8 index = 0; index < deviceDescriptor.tachometerCount; ++index) {
			tachometers[index] = CALL_MEMBER_FUNC(*this, deviceDescriptor.updateTachometer)(index);
		}
	}
	
	/**
	 *  Supported devices
	 */
	const Device::DeviceDescriptor Device::_IT8512F = { IT8512F, 5, &Device::tachometerRead };
	const Device::DeviceDescriptor Device::_IT8705F = { IT8705F, 3, &Device::tachometerRead };
	const Device::DeviceDescriptor Device::_IT8712F = { IT8712F, 5, &Device::tachometerRead };
	const Device::DeviceDescriptor Device::_IT8716F = { IT8716F, 5, &Device::tachometerRead16 };
	const Device::DeviceDescriptor Device::_IT8718F = { IT8718F, 5, &Device::tachometerRead16 };
	const Device::DeviceDescriptor Device::_IT8720F = { IT8720F, 5, &Device::tachometerRead16 };
	const Device::DeviceDescriptor Device::_IT8721F = { IT8721F, 5, &Device::tachometerRead16 };
	const Device::DeviceDescriptor Device::_IT8726F = { IT8726F, 5, &Device::tachometerRead16 };
	const Device::DeviceDescriptor Device::_IT8620E = { IT8620E, 5, &Device::tachometerRead16 };
	const Device::DeviceDescriptor Device::_IT8628E = { IT8628E, 5, &Device::tachometerRead16 };
	const Device::DeviceDescriptor Device::_IT8686E = { IT8686E, 5, &Device::tachometerRead16 };
	const Device::DeviceDescriptor Device::_IT8728F = { IT8728F, 5, &Device::tachometerRead16 };
	const Device::DeviceDescriptor Device::_IT8752F = { IT8752F, 5, &Device::tachometerRead16 };
	const Device::DeviceDescriptor Device::_IT8771E = { IT8771E, 5, &Device::tachometerRead16 };
	const Device::DeviceDescriptor Device::_IT8772E = { IT8772E, 5, &Device::tachometerRead16 };
	const Device::DeviceDescriptor Device::_IT8792E = { IT8792E, 5, &Device::tachometerRead16 };
	
} // namespace ITE
