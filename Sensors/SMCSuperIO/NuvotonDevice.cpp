//
//  NuvotonDevice.cpp
//
//  Sensors implementation for Nuvoton SuperIO device
//
//  Based on https://github.com/kozlek/HWSensors/blob/master/SuperIOSensors/NCT677xSensors.cpp
//  @author joedm.
//

#include "NuvotonDevice.hpp"
#include "SMCSuperIO.hpp"

namespace Nuvoton {
	
	uint8_t Device::readByte(uint16_t reg) {
		uint8_t bank = reg >> 8;
		uint8_t regi = reg & 0xFF;
		uint16_t address = getDeviceAddress();
		
		outb((uint16_t)(address + NUVOTON_ADDRESS_REGISTER_OFFSET), NUVOTON_BANK_SELECT_REGISTER);
		outb((uint16_t)(address + NUVOTON_DATA_REGISTER_OFFSET), bank);
		outb((uint16_t)(address + NUVOTON_ADDRESS_REGISTER_OFFSET), regi);
		
		return inb((uint16_t)(address + NUVOTON_DATA_REGISTER_OFFSET));
	}
	
	void Device::writeByte(uint16_t reg, uint8_t value) {
		uint8_t bank = reg >> 8;
		uint8_t regi = reg & 0xFF;
		uint16_t address = getDeviceAddress();
		
		outb((uint16_t)(address + NUVOTON_ADDRESS_REGISTER_OFFSET), NUVOTON_BANK_SELECT_REGISTER);
		outb((uint16_t)(address + NUVOTON_DATA_REGISTER_OFFSET), bank);
		outb((uint16_t)(address + NUVOTON_ADDRESS_REGISTER_OFFSET), regi);
		outb((uint16_t)(address + NUVOTON_DATA_REGISTER_OFFSET), value);
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
			tachometers[index] = CALL_MEMBER_FUNC(*this, deviceDescriptor.updateTachometer)(index);
		}
	}
	
	uint16_t Device::tachometerReadDefault(uint8_t index) {
		uint8_t high = readByte(deviceDescriptor.tachometerRpmBaseRegister + (index << 1));
		uint8_t low = readByte(deviceDescriptor.tachometerRpmBaseRegister + (index << 1) + 1);
	
		uint16_t value = (high << 8) | low;
		
		return value > deviceDescriptor.tachometerMinRPM ? value : 0;
	}
	
	void Device::initialize679xx() {
		i386_ioport_t port = getDevicePort();
		// disable the hardware monitor i/o space lock on NCT679xD chips
		winbond_family_enter(port);
		superio_select_logical_device(port, kWinbondHardwareMonitorLDN);
		/* Activate logical device if needed */
		uint8_t options = superio_listen_port_byte(port, NUVOTON_REG_ENABLE);
		if (!(options & 0x01)) {
			superio_write_port_byte(port, NUVOTON_REG_ENABLE, options | 0x01);
		}
		options = superio_listen_port_byte(port, NUVOTON_HWMON_IO_SPACE_LOCK);
		// if the i/o space lock is enabled
		if (options & 0x10) {
			// disable the i/o space lock
			superio_write_port_byte(port, NUVOTON_HWMON_IO_SPACE_LOCK, (uint8_t)(options & ~0x10));
		}
		winbond_family_exit(port);
	}
	
	void Device::powerStateChanged(unsigned long state) {
		if (state == SMCSuperIO::PowerStateOn) {
			 CALL_MEMBER_FUNC(*this, deviceDescriptor.initialize)();
		 }
	}

	/**
	 *  Supported devices
	 */
	const Device::DeviceDescriptor Device::_NCT6771F = { NCT6771F, 3, RPM_THRESHOLD1, 0x656, &Device::tachometerReadDefault, &Device::stub };
	const Device::DeviceDescriptor Device::_NCT6776F = { NCT6776F, 3, RPM_THRESHOLD2, 0x656, &Device::tachometerReadDefault, &Device::stub };
	const Device::DeviceDescriptor Device::_NCT6779D = { NCT6779D, 5, RPM_THRESHOLD2, 0x4C0, &Device::tachometerReadDefault, &Device::stub };
	const Device::DeviceDescriptor Device::_NCT6791D = { NCT6791D, 6, RPM_THRESHOLD2, 0x4C0, &Device::tachometerReadDefault, &Device::initialize679xx };
	const Device::DeviceDescriptor Device::_NCT6792D = { NCT6792D, 6, RPM_THRESHOLD2, 0x4C0, &Device::tachometerReadDefault, &Device::initialize679xx };
	const Device::DeviceDescriptor Device::_NCT6793D = { NCT6793D, 6, RPM_THRESHOLD2, 0x4C0, &Device::tachometerReadDefault, &Device::initialize679xx };
	const Device::DeviceDescriptor Device::_NCT6795D = { NCT6795D, 6, RPM_THRESHOLD2, 0x4C0, &Device::tachometerReadDefault, &Device::initialize679xx };
	const Device::DeviceDescriptor Device::_NCT6796D = { NCT6796D, 6, RPM_THRESHOLD2, 0x4C0, &Device::tachometerReadDefault, &Device::initialize679xx };
} // namespace Nuvoton
