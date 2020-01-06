//
//  WinbondDevice.cpp
//
//  Sensors implementation for Winbond SuperIO device
//
//  Based on https://github.com/kozlek/HWSensors/blob/master/SuperIOSensors/W836xxSensors.cpp
//  @author joedm
//

#include "WinbondDevice.hpp"
#include "SMCSuperIO.hpp"

namespace Winbond {

	inline uint64_t set_bit(uint64_t target, uint16_t bit, uint32_t value) {
		if (((value & 1) == value) && bit <= 63) {
			uint64_t mask = (1ULL << bit);
			return value > 0 ? target | mask : target & ~mask;
		}
		return value;
	}
	
	uint8_t WinbondDevice::readByte(uint16_t reg) {
		uint16_t address = getDeviceAddress();
		uint8_t bank = reg >> 8;
		uint8_t regi = reg & 0xFF;
		
		::outb(address + WINBOND_ADDRESS_REGISTER_OFFSET, WINBOND_BANK_SELECT_REGISTER);
		::outb(address + WINBOND_DATA_REGISTER_OFFSET, bank);
		::outb(address + WINBOND_ADDRESS_REGISTER_OFFSET, regi);
		return ::inb(address + WINBOND_DATA_REGISTER_OFFSET);
	}
	
	void WinbondDevice::writeByte(uint16_t reg, uint8_t value) {
		uint16_t address = getDeviceAddress();
		uint8_t bank = reg >> 8;
		uint8_t regi = reg & 0xFF;
		
		::outb(address + WINBOND_ADDRESS_REGISTER_OFFSET, WINBOND_BANK_SELECT_REGISTER);
		::outb(address + WINBOND_DATA_REGISTER_OFFSET, bank);
		::outb(address + WINBOND_ADDRESS_REGISTER_OFFSET, regi);
		::outb(address + WINBOND_DATA_REGISTER_OFFSET, value);
	}
	
	void WinbondDevice::setupKeys(VirtualSMCAPI::Plugin &vsmcPlugin) {
		VirtualSMCAPI::addKey(KeyFNum, vsmcPlugin.data,
			VirtualSMCAPI::valueWithUint8(getTachometerCount(), nullptr, SMC_KEY_ATTRIBUTE_CONST | SMC_KEY_ATTRIBUTE_READ));
		for (uint8_t index = 0; index < getTachometerCount(); ++index) {
			VirtualSMCAPI::addKey(KeyF0Ac(index), vsmcPlugin.data,
				VirtualSMCAPI::valueWithFp(0, SmcKeyTypeFpe2, new TachometerKey(getSmcSuperIO(), this, index)));
		}
	}
	
	void WinbondDevice::updateTachometers() {
		uint64_t bits = 0;
		
		for (uint8_t i = 0; i < 5; i++) {
			bits = (bits << 8) | readByte(WINBOND_TACHOMETER_DIVISOR[i]);
		}
		
		uint64_t newBits = bits;
		
		for (uint8_t i = 0; i < getTachometerCount(); i++) {
			// assemble fan divisor
			uint8_t offset = (((bits >> WINBOND_TACHOMETER_DIVISOR2[i]) & 1) << 2) |
			(((bits >> WINBOND_TACHOMETER_DIVISOR1[i]) & 1) << 1) |
			((bits >> WINBOND_TACHOMETER_DIVISOR0[i]) & 1);
			
			uint8_t divisor = 1 << offset;
			uint8_t count = readByte(WINBOND_TACHOMETER[i]);
			
			// update fan divisor
			if (count > 192 && offset < 7) {
				offset++;
			} else if (count < 96 && offset > 0) {
				offset--;
			}
			
			setTachometerValue(i, (count < 0xff) ? 1.35e6f / (count * divisor) : 0);
						
			newBits = set_bit(newBits, WINBOND_TACHOMETER_DIVISOR2[i], (offset >> 2) & 1);
			newBits = set_bit(newBits, WINBOND_TACHOMETER_DIVISOR1[i], (offset >> 1) & 1);
			newBits = set_bit(newBits, WINBOND_TACHOMETER_DIVISOR0[i],  offset       & 1);
		}
		
		// write new fan divisors
		for (int i = 4; i >= 0; i--) {
			uint8_t oldByte = bits & 0xff;
			uint8_t newByte = newBits & 0xff;
			
			if (oldByte != newByte)	{
				writeByte(WINBOND_TACHOMETER_DIVISOR[i], newByte);
			}
			
			bits = bits >> 8;
			newBits = newBits >> 8;
		}
	}

	float WinbondDevice::voltageRead(uint8_t index) {
        // read battery voltage if enabled
		if (WINBOND_VOLTAGE_REG[index] == WINBOND_VOLTAGE_VBAT) {
			if ((readByte(0x005D) & 0x01) > 0) {
				return static_cast<float>(readByte(WINBOND_VOLTAGE_VBAT)) * 0.008f;
			} else {
				return 0.0f;
			}
		}
		return static_cast<float>(readByte(WINBOND_VOLTAGE_REG[index])) * 0.008f;
	}

	float WinbondDevice::voltageReadVrmCheck(uint8_t index) {
		if (index == 0) {
			float v = static_cast<float>(readByte(WINBOND_VOLTAGE_REG1[index]));
            uint8_t vrmConfiguration = readByte(0x0018);
            if ((vrmConfiguration & 0x01) == 0) {
				return 0.016f * v; // VRM8 formula
			} else {
				return 0.00488f * v + 0.69f; // VRM9 formula
			}
		}
        // read battery voltage if enabled
		if (WINBOND_VOLTAGE_REG1[index] == WINBOND_VOLTAGE_VBAT) {
			if ((readByte(0x005D) & 0x01) > 0) {
				return static_cast<float>(readByte(WINBOND_VOLTAGE_VBAT)) * 0.016f;
			} else {
				return 0.0f;
			}
		}
		return static_cast<float>(readByte(WINBOND_VOLTAGE_REG1[index])) * 0.016f;
	}

} // namespace Winbond
