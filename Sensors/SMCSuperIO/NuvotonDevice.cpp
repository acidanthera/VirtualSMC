//
//  NuvotonDevice.cpp
//
//  Sensors implementation for Nuvoton SuperIO device
//
//  Based on https://github.com/kozlek/HWSensors/blob/master/SuperIOSensors/NCT677xSensors.cpp
//  @Author joedm.
//

#include "NuvotonDevice.hpp"
#include "SMCSuperIO.hpp"

UInt8 NuvotonGenericDevice::readByte(UInt16 reg)
{
	UInt8 bank = reg >> 8;
	UInt8 regi = reg & 0xFF;
	UInt16 address = getDeviceAddress();
	
	outb((UInt16)(address + NUVOTON_ADDRESS_REGISTER_OFFSET), NUVOTON_BANK_SELECT_REGISTER);
	outb((UInt16)(address + NUVOTON_DATA_REGISTER_OFFSET), bank);
	outb((UInt16)(address + NUVOTON_ADDRESS_REGISTER_OFFSET), regi);
	
	return inb((UInt16)(address + NUVOTON_DATA_REGISTER_OFFSET));
}

void NuvotonGenericDevice::writeByte(UInt16 reg, UInt8 value)
{
	UInt8 bank = reg >> 8;
	UInt8 regi = reg & 0xFF;
	UInt16 address = getDeviceAddress();
	
	outb((UInt16)(address + NUVOTON_ADDRESS_REGISTER_OFFSET), NUVOTON_BANK_SELECT_REGISTER);
	outb((UInt16)(address + NUVOTON_DATA_REGISTER_OFFSET), bank);
	outb((UInt16)(address + NUVOTON_ADDRESS_REGISTER_OFFSET), regi);
	outb((UInt16)(address + NUVOTON_DATA_REGISTER_OFFSET), value);
}

void NuvotonGenericDevice::setupKeys(VirtualSMCAPI::Plugin &vsmcPlugin)
{
	VirtualSMCAPI::addKey(KeyFNum, vsmcPlugin.data, VirtualSMCAPI::valueWithUint8(tachometerMaxCount, nullptr, SMC_KEY_ATTRIBUTE_CONST | SMC_KEY_ATTRIBUTE_READ));
	for (UInt8 index = 0; index < tachometerMaxCount; ++index) {
		VirtualSMCAPI::addKey(KeyF0Ac(index), vsmcPlugin.data, VirtualSMCAPI::valueWithFp(0, SmcKeyTypeFpe2, new TachometerKey(getSmcSuperIO(), this, index)));
	}
}

void NuvotonGenericDevice::update()
{
	IOSimpleLockLock(getSmcSuperIO()->counterLock);
	updateTachometers();
	IOSimpleLockUnlock(getSmcSuperIO()->counterLock);
}

void NuvotonGenericDevice::updateTachometers()
{
	for (UInt8 index = 0; index < tachometerMaxCount; ++index) {
		UInt8 high = readByte(tachometerRpmBaseRegister + (index << 1));
		UInt8 low = readByte(tachometerRpmBaseRegister + (index << 1) + 1);
		
		UInt32 value = (high << 8) | low;
		
		tachometers[index] = value > tachometerMinRPM ? value : 0;
	}
}

/**
 * Keys
 */
SMC_RESULT TachometerKey::readAccess() {
	IOSimpleLockLock(sio->counterLock);
	double val = (double)device->getTachometerValue(index);
	*reinterpret_cast<uint32_t *>(data) = VirtualSMCAPI::encodeFp(SmcKeyTypeFpe2, val);
	sio->quickReschedule();
	IOSimpleLockUnlock(sio->counterLock);
	return SmcSuccess;
}
