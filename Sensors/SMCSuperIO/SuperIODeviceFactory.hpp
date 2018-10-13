//
//  SuperIODeviceFactory.hpp
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
class SuperIODeviceFactory {
	uint16_t            id;
	uint16_t            model;
	uint8_t             ldn;
	const char*         vendor;
	uint16_t            address;
	i386_ioport_t       port;
	SuperIODevice*		detectedDevice {nullptr};
	bool detectWinbondFamilyChip();
	bool detectITEFamilyChip();
	
public:
	SuperIODevice* detectAndCreate(SMCSuperIO* sio, bool isGigabyte = false);
};

#endif // _SUPERIODEVICEFACTORY_HPP
