//
//  WinbondDevice.cpp
//
//  Sensors implementation for Winbond SuperIO device
//
//  Based on https://github.com/kozlek/HWSensors/blob/master/SuperIOSensors/W836xxSensors.cpp
//  @author joedm.
//

#include "WinbondDevice.hpp"
#include "SMCSuperIO.hpp"

namespace Winbond {

	inline uint64_t set_bit(uint64_t target, uint16_t bit, uint32_t value) {
		if (((value & 1) == value) && bit <= 63) {
			uint64_t mask = (((uint64_t)1) << bit);
			return value > 0 ? target | mask : target & ~mask;
		}
		return value;
	}
	
	uint8_t Device::readByte(uint8_t reg) {
		uint16_t address = getDeviceAddress();
		uint8_t bank = reg >> 8;
		uint8_t regi = reg & 0xFF;
		
		outb((uint16_t)(address + WINBOND_ADDRESS_REGISTER_OFFSET), WINBOND_BANK_SELECT_REGISTER);
		outb((uint16_t)(address + WINBOND_DATA_REGISTER_OFFSET), bank);
		outb((uint16_t)(address + WINBOND_ADDRESS_REGISTER_OFFSET), regi);
		return inb((uint16_t)(address + WINBOND_DATA_REGISTER_OFFSET));
	}
	
	void Device::writeByte(uint8_t reg, uint8_t value) {
		uint16_t address = getDeviceAddress();
		uint8_t bank = reg >> 8;
		uint8_t regi = reg & 0xFF;
		
		outb((uint16_t)(address + WINBOND_ADDRESS_REGISTER_OFFSET), WINBOND_BANK_SELECT_REGISTER);
		outb((uint16_t)(address + WINBOND_DATA_REGISTER_OFFSET), bank);
		outb((uint16_t)(address + WINBOND_ADDRESS_REGISTER_OFFSET), regi);
		outb((uint16_t)(address + WINBOND_DATA_REGISTER_OFFSET), value);
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
		uint64_t bits = 0;
		
		for (uint8_t i = 0; i < 5; i++) {
			bits = (bits << 8) | readByte(WINBOND_TACHOMETER_DIVISOR[i]);
		}
		
		uint64_t newBits = bits;
		
		for (uint8_t i = 0; i < deviceDescriptor.tachometerCount; i++) {
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
			
			tachometers[i] = (count < 0xff) ? 1.35e6f / (float(count * divisor)) : 0;
						
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
	
	/**
	 *  Supported devices
	 */
	const Device::DeviceDescriptor Device::_W83627EHF = { W83627EHF, 5};
	const Device::DeviceDescriptor Device::_W83627DHG = { W83627DHG, 5};
	const Device::DeviceDescriptor Device::_W83627DHGP = { W83627DHGP, 5};
	const Device::DeviceDescriptor Device::_W83667HG = { W83667HG, 5};
	const Device::DeviceDescriptor Device::_W83667HGB = { W83667HGB, 5};
	const Device::DeviceDescriptor Device::_W83627HF = { W83627HF, 3};
	const Device::DeviceDescriptor Device::_W83627THF = { W83627THF, 3};
	const Device::DeviceDescriptor Device::_W83687THF = { W83687THF, 3};

} // namespace Winbond
