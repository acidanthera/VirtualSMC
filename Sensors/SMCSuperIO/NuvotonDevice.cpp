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

UInt8 NuvotonGenericDevice::readByte(UInt16 reg) {
	UInt8 bank = reg >> 8;
	UInt8 regi = reg & 0xFF;
	UInt16 address = getDeviceAddress();
	
	outb((UInt16)(address + NUVOTON_ADDRESS_REGISTER_OFFSET), NUVOTON_BANK_SELECT_REGISTER);
	outb((UInt16)(address + NUVOTON_DATA_REGISTER_OFFSET), bank);
	outb((UInt16)(address + NUVOTON_ADDRESS_REGISTER_OFFSET), regi);
	
	return inb((UInt16)(address + NUVOTON_DATA_REGISTER_OFFSET));
}

void NuvotonGenericDevice::writeByte(UInt16 reg, UInt8 value) {
	UInt8 bank = reg >> 8;
	UInt8 regi = reg & 0xFF;
	UInt16 address = getDeviceAddress();
	
	outb((UInt16)(address + NUVOTON_ADDRESS_REGISTER_OFFSET), NUVOTON_BANK_SELECT_REGISTER);
	outb((UInt16)(address + NUVOTON_DATA_REGISTER_OFFSET), bank);
	outb((UInt16)(address + NUVOTON_ADDRESS_REGISTER_OFFSET), regi);
	outb((UInt16)(address + NUVOTON_DATA_REGISTER_OFFSET), value);
}

void NuvotonGenericDevice::setupKeys(VirtualSMCAPI::Plugin &vsmcPlugin) {
	VirtualSMCAPI::addKey(KeyFNum, vsmcPlugin.data, VirtualSMCAPI::valueWithUint8(tachometerMaxCount, nullptr, SMC_KEY_ATTRIBUTE_CONST | SMC_KEY_ATTRIBUTE_READ));
	for (UInt8 index = 0; index < tachometerMaxCount; ++index) {
		VirtualSMCAPI::addKey(KeyF0Ac(index), vsmcPlugin.data, VirtualSMCAPI::valueWithFp(0, SmcKeyTypeFpe2, new TachometerKey(getSmcSuperIO(), this, index)));
	}
}

void NuvotonGenericDevice::update() {
	IOSimpleLockLock(getSmcSuperIO()->counterLock);
	updateTachometers();
	IOSimpleLockUnlock(getSmcSuperIO()->counterLock);
}

void NuvotonGenericDevice::updateTachometers() {
	for (UInt8 index = 0; index < tachometerMaxCount; ++index) {
		UInt8 high = readByte(tachometerRpmBaseRegister + (index << 1));
		UInt8 low = readByte(tachometerRpmBaseRegister + (index << 1) + 1);
		
		UInt16 value = (high << 8) | low;
		
		tachometers[index] = value > tachometerMinRPM ? value : 0;
	}
}

void NuvotonNCT679xxDevice::initialize() {
	i386_ioport_t port = getDevicePort();
	// disable the hardware monitor i/o space lock on NCT679xD chips
	winbond_family_enter(port);
	superio_select_logical_device(port, kWinbondHardwareMonitorLDN);
	/* Activate logical device if needed */
	UInt8 options = superio_listen_port_byte(port, NUVOTON_REG_ENABLE);
	if (!(options & 0x01)) {
		superio_write_port_byte(port, NUVOTON_REG_ENABLE, options | 0x01);
	}
	options = superio_listen_port_byte(port, NUVOTON_HWMON_IO_SPACE_LOCK);
	// if the i/o space lock is enabled
	if (options & 0x10) {
		// disable the i/o space lock
		superio_write_port_byte(port, NUVOTON_HWMON_IO_SPACE_LOCK, (UInt8)(options & ~0x10));
	}
	winbond_family_exit(port);
}
