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
