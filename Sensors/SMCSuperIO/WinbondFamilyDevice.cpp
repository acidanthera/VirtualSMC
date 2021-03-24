//
//  WindbondFamilyDevice.cpp
//
//  Windbond-family devices data
//
//  Based on https://github.com/kozlek/HWSensors/blob/master/SuperIOSensors/SuperIODevice.h
//  @author joedm
//

#include "WinbondFamilyDevice.hpp"

uint16_t WindbondFamilyDevice::detectAndVerifyAddress(i386_ioport_t port, uint8_t ldn) {
	selectLogicalDevice(port, ldn);
	uint16_t address = listenPortWord(port, SuperIOBaseAddressRegister);
	IOSleep(50);
	uint16_t verifyAddress = listenPortWord(port, SuperIOBaseAddressRegister);
	if (address == verifyAddress) {
		// some Fintek chips have address register offset 0x05 added already
		if ((address & 0x07) == 0x05) {
			address &= 0xFFF8;
		}
		if (address < 0x100 || (address & 0xF007) != 0) {
			DBGLOG("ssio", "WindbondFamilyDevice address 0x%04X is out of bounds!", address);
		} else {
			return address;
		}
	} else {
		DBGLOG("ssio", "WindbondFamilyDevice address verify check error: address = 0x%04X, verifyAddress = 0x%04X", address, verifyAddress);
	}
	return 0;
}
