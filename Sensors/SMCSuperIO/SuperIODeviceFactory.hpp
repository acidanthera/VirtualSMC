//
//  SuperIODeviceFactory.h
//
//  Detects SuperIO Chip installed
//
//  Based on https://github.com/kozlek/HWSensors/blob/master/SuperIOSensors/SuperIODevice.h
//  @author joedm.
//

#ifndef _SUPERIODEVICEFACTORY_HPP
#define _SUPERIODEVICEFACTORY_HPP

#include <IOKit/IOService.h>
#include <architecture/i386/pio.h>
#include "SuperIODevice.hpp"

class SMCSuperIO;
class SuperIODeviceFactory
{
	UInt16              id;
	UInt16              model;
	UInt8               ldn;
	const char*         vendor;
	UInt16              address;
	i386_ioport_t       port;
	SuperIODevice*		detectedDevice {nullptr};
	bool detectWinbondFamilyChip();
	bool detectITEFamilyChip();
	
public:
	SuperIODevice* detectAndCreate(SMCSuperIO* sio, bool isGigabyte = false);
};

#endif // _SUPERIODEVICEFACTORY_HPP
