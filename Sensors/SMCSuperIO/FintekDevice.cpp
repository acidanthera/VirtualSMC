//
//  FintekDevice.cpp
//
//  Sensors implementation for ITE SuperIO device
//
//  Based on https://github.com/kozlek/HWSensors/blob/master/SuperIOSensors/F718xxSensors.cpp
//  @author joedm
//

#include "FintekDevice.hpp"
#include "SMCSuperIO.hpp"

namespace Fintek {
	
	uint8_t FintekDevice::readByte(uint8_t reg) {
		uint16_t address = getDeviceAddress();
		::outb(address + FINTEK_ADDRESS_REGISTER_OFFSET, reg);
		return ::inb(address + FINTEK_DATA_REGISTER_OFFSET);
	}
	
	void FintekDevice::setupKeys(VirtualSMCAPI::Plugin &vsmcPlugin) {
		VirtualSMCAPI::addKey(KeyFNum, vsmcPlugin.data,
			VirtualSMCAPI::valueWithUint8(getTachometerCount(), nullptr, SMC_KEY_ATTRIBUTE_CONST | SMC_KEY_ATTRIBUTE_READ));
		for (uint8_t index = 0; index < getTachometerCount(); ++index) {
			VirtualSMCAPI::addKey(KeyF0Ac(index), vsmcPlugin.data,
				VirtualSMCAPI::valueWithFp(0, SmcKeyTypeFpe2, new TachometerKey(getSmcSuperIO(), this, index)));
		}
	}
	
	uint16_t FintekDevice::tachometerRead(uint8_t index) {
		int16_t value = readByte(FINTEK_FAN_TACHOMETER_REG[index]) << 8;
		value |= readByte(FINTEK_FAN_TACHOMETER_REG[index] + 1);
		
		if (value > 0) {
			value = (value < 0x0fff) ? 1.5e6f / value : 0;
		}
		return value;
	}

	float FintekDevice::voltageRead(uint8_t index) {
		uint8_t v = readByte(FINTEK_VOLTAGE_REG[index]);
		return static_cast<float>(v) * 0.008f;
	}

	float FintekDevice::voltageRead71808E(uint8_t index) {
		if (index != 6) {
			return voltageRead(index);
		}
		return 0.0f; // port 0x26 is reserved
	}

} // namespace Fintek
