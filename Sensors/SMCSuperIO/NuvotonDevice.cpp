//
//  NuvotonDevice.cpp
//
//  Sensors implementation for Nuvoton SuperIO device
//
//  Based on https://github.com/kozlek/HWSensors/blob/master/SuperIOSensors/NCT677xSensors.cpp
//  @author joedm
//

#include "NuvotonDevice.hpp"
#include "SMCSuperIO.hpp"

namespace Nuvoton {
	
	uint8_t Device::readByte(uint16_t reg) {
		uint8_t bank = reg >> 8;
		uint8_t regi = reg & 0xFF;
		uint16_t address = getDeviceAddress();
		
		::outb(address + NUVOTON_ADDRESS_REGISTER_OFFSET, NUVOTON_BANK_SELECT_REGISTER);
		::outb(address + NUVOTON_DATA_REGISTER_OFFSET, bank);
		::outb(address + NUVOTON_ADDRESS_REGISTER_OFFSET, regi);
		
		return ::inb(address + NUVOTON_DATA_REGISTER_OFFSET);
	}
	
	void Device::writeByte(uint16_t reg, uint8_t value) {
		uint8_t bank = reg >> 8;
		uint8_t regi = reg & 0xFF;
		uint16_t address = getDeviceAddress();
		
		::outb(address + NUVOTON_ADDRESS_REGISTER_OFFSET, NUVOTON_BANK_SELECT_REGISTER);
		::outb(address + NUVOTON_DATA_REGISTER_OFFSET, bank);
		::outb(address + NUVOTON_ADDRESS_REGISTER_OFFSET, regi);
		::outb(address + NUVOTON_DATA_REGISTER_OFFSET, value);
	}
	
	void Device::setupKeys(VirtualSMCAPI::Plugin &vsmcPlugin) {
		VirtualSMCAPI::addKey(KeyFNum, vsmcPlugin.data,
			VirtualSMCAPI::valueWithUint8(deviceDescriptor.tachometerCount, nullptr, SMC_KEY_ATTRIBUTE_CONST | SMC_KEY_ATTRIBUTE_READ));
		for (uint8_t index = 0; index < deviceDescriptor.tachometerCount; ++index) {
			VirtualSMCAPI::addKey(KeyF0Ac(index), vsmcPlugin.data,
				VirtualSMCAPI::valueWithFp(0, SmcKeyTypeFpe2, new TachometerKey(getSmcSuperIO(), this, index)));
		}
	}
	
	void Device::update() {
		IOSimpleLockLock(getSmcSuperIO()->counterLock);
		updateTachometers();
		IOSimpleLockUnlock(getSmcSuperIO()->counterLock);
	}
	
	void Device::updateTachometers() {
		for (uint8_t index = 0; index < deviceDescriptor.tachometerCount; ++index) {
			uint8_t high = readByte(deviceDescriptor.tachometerRpmRegisters[index]);
			uint8_t low = readByte(deviceDescriptor.tachometerRpmRegisters[index] + 1);
			uint16_t value = (high << 8) | low;
			tachometers[index] = value;
		}
	}
	
	void Device::initialize679xx() {
		i386_ioport_t port = getDevicePort();
		// disable the hardware monitor i/o space lock on NCT679xD chips
		enter(port);
		selectLogicalDevice(port, WinbondHardwareMonitorLDN);
		/* Activate logical device if needed */
		uint8_t options = listenPortByte(port, NUVOTON_REG_ENABLE);
		if (!(options & 0x01)) {
			writePortByte(port, NUVOTON_REG_ENABLE, options | 0x01);
		}
		options = listenPortByte(port, NUVOTON_HWMON_IO_SPACE_LOCK);
		// if the i/o space lock is enabled
		if (options & 0x10) {
			// disable the i/o space lock
			writePortByte(port, NUVOTON_HWMON_IO_SPACE_LOCK, options & ~0x10);
		}
		leave(port);
	}
	
	void Device::powerStateChanged(unsigned long state) {
		if (state == SMCSuperIO::PowerStateOn) {
			auto initProc = deviceDescriptor.initialize;
			if (initProc) {
				CALL_MEMBER_FUNC(*this, initProc)();
			}
		 }
	}

	/**
	 *  Supported devices
	 */
	const Device::DeviceDescriptor Device::_NCT6771F = { NCT6771F, 3, NUVOTON_3_FANS_RPM_REGS, nullptr };
	const Device::DeviceDescriptor Device::_NCT6776F = { NCT6776F, 3, NUVOTON_3_FANS_RPM_REGS, nullptr };
	const Device::DeviceDescriptor Device::_NCT6779D = { NCT6779D, 5, NUVOTON_5_FANS_RPM_REGS, nullptr };
	const Device::DeviceDescriptor Device::_NCT6791D = { NCT6791D, 6, NUVOTON_6_FANS_RPM_REGS, &Device::initialize679xx };
	const Device::DeviceDescriptor Device::_NCT6792D = { NCT6792D, 6, NUVOTON_6_FANS_RPM_REGS, &Device::initialize679xx };
	const Device::DeviceDescriptor Device::_NCT6793D = { NCT6793D, 6, NUVOTON_6_FANS_RPM_REGS, &Device::initialize679xx };
	const Device::DeviceDescriptor Device::_NCT6795D = { NCT6795D, 6, NUVOTON_6_FANS_RPM_REGS, &Device::initialize679xx };
	const Device::DeviceDescriptor Device::_NCT6796D = { NCT6796D, 7, NUVOTON_7_FANS_RPM_REGS, &Device::initialize679xx };
	const Device::DeviceDescriptor Device::_NCT6797D = { NCT6797D, 7, NUVOTON_7_FANS_RPM_REGS, &Device::initialize679xx };
	const Device::DeviceDescriptor Device::_NCT6798D = { NCT6798D, 7, NUVOTON_7_FANS_RPM_REGS, &Device::initialize679xx };
	
	/**
	 *  Device factory helper
	 */
	const Device::DeviceDescriptor* Device::detectModel(uint16_t id, uint8_t &ldn) {
		uint8_t majorId = id >> 8;
		if (majorId == 0xB4 && (id & 0xF0) == 0x70)
			return &_NCT6771F;
		if (majorId == 0xC3 && (id & 0xF0) == 0x30)
			return &_NCT6776F;
		if (majorId == 0xC5 && (id & 0xF0) == 0x60)
			return &_NCT6779D;
		if (majorId == 0xC8 && (id & 0xFF) == 0x03)
			return &_NCT6791D;
		if (majorId == 0xC9 && (id & 0xFF) == 0x11)
			return &_NCT6792D;
		if (majorId == 0xD1 && (id & 0xFF) == 0x21)
			return &_NCT6793D;
		if (majorId == 0xD3 && (id & 0xFF) == 0x52)
			return &_NCT6795D;
		if (majorId == 0xD4 && (id & 0xFF) == 0x23)
			return &_NCT6796D;
		if (majorId == 0xD4 && (id & 0xFF) == 0x51)
			return &_NCT6797D;
		if (majorId == 0xD4 && (id & 0xFF) == 0x59)
			return &_NCT6798D;
		return nullptr;
	}

	/**
	 *  Device factory
	 */
	SuperIODevice* Device::detect(SMCSuperIO* sio) {
		return WindbondFamilyDevice::detect<Device, DeviceDescriptor>(sio);
	}

} // namespace Nuvoton
