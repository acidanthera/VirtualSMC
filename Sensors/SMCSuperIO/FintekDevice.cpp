//
//  FintekDevice.cpp
//
//  Sensors implementation for ITE SuperIO device
//
//  Based on https://github.com/kozlek/HWSensors/blob/master/SuperIOSensors/F718xxSensors.cpp
//  @author joedm.
//

#include "FintekDevice.hpp"
#include "SMCSuperIO.hpp"

namespace Fintek {
	
	uint8_t Device::readByte(uint8_t reg) {
		uint16_t address = getDeviceAddress();
		outb(address + FINTEK_ADDRESS_REGISTER_OFFSET, reg);
		return inb(address + FINTEK_DATA_REGISTER_OFFSET);
	}
	
	void Device::setupKeys(VirtualSMCAPI::Plugin &vsmcPlugin) {
		VirtualSMCAPI::addKey(KeyFNum, vsmcPlugin.data, VirtualSMCAPI::valueWithUint8(deviceDescriptor.tachometerCount, nullptr, SMC_KEY_ATTRIBUTE_CONST | SMC_KEY_ATTRIBUTE_READ));
		for (uint8_t index = 0; index < deviceDescriptor.tachometerCount; ++index) {
			VirtualSMCAPI::addKey(KeyF0Ac(index), vsmcPlugin.data, VirtualSMCAPI::valueWithFp(0, SmcKeyTypeFpe2, new TachometerKey(getSmcSuperIO(), this, index)));
		}
	}
	
	void Device::update() {
		IOSimpleLockLock(getSmcSuperIO()->counterLock);
		updateTachometers();
		IOSimpleLockUnlock(getSmcSuperIO()->counterLock);
	}
	
	void Device::updateTachometers() {
		for (uint8_t index = 0; index < deviceDescriptor.tachometerCount; ++index) {
			int16_t value = readByte(FINTEK_FAN_TACHOMETER_REG[index]) << 8;
			value |= readByte(FINTEK_FAN_TACHOMETER_REG[index] + 1);
			
			if (value > 0) {
				value = (value < 0x0fff) ? 1.5e6f / (float)value : 0;
			}
		
			tachometers[index] = value;
		}
	}
	
	/**
	 *  Supported devices
	 */
	const Device::DeviceDescriptor Device::_F71858 = { F71858, 4 };
	const Device::DeviceDescriptor Device::_F71862 = { F71862, 3 };
	const Device::DeviceDescriptor Device::_F71868A = { F71868A, 3 };
	const Device::DeviceDescriptor Device::_F71869 = { F71869, 3 };
	const Device::DeviceDescriptor Device::_F71869A = { F71869A, 3 };
	const Device::DeviceDescriptor Device::_F71882 = { F71882, 4 };
	const Device::DeviceDescriptor Device::_F71889AD = { F71889AD, 3 };
	const Device::DeviceDescriptor Device::_F71889ED = { F71889ED, 3 };
	const Device::DeviceDescriptor Device::_F71889F = { F71889F, 3 };
	const Device::DeviceDescriptor Device::_F71808E = { F71808E, 3 };
	
} // namespace Fintek
