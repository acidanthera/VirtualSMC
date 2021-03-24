//
//  WindbondFamilyDevice.hpp
//
//  Windbond-family devices data
//
//  Based on https://github.com/kozlek/HWSensors/blob/master/SuperIOSensors/SuperIODevice.h
//  @author joedm
//

#ifndef _WINBONDFAMILYDEVICE_HPP
#define _WINBONDFAMILYDEVICE_HPP

#include "SuperIODevice.hpp"
#include "Devices.hpp"

class WindbondFamilyDevice : public SuperIODevice {
protected:
	/**
	 *  Hardware access methods
	 */
	static inline void enter(i386_ioport_t port) {
		::outb(port, 0x87);
		::outb(port, 0x87);
	}
	
	static inline void leave(i386_ioport_t port) {
		::outb(port, 0xAA);
	}
	
	/**
	 *  Windbond-family common procedure to detect device address.
	 *  NOTE: the device must be in entered state before the call to this method.
	 */
	static uint16_t detectAndVerifyAddress(i386_ioport_t port, uint8_t ldn);
	
	/**
	 *  Device factory helper
	 */
	static SuperIODevice* probePort(i386_ioport_t port, SMCSuperIO* sio) {
		enter(port);
		uint16_t id = listenPortWord(port, SuperIOChipIDRegister);
		DBGLOG("ssio", "WindbondFamilyDevice probing device on 0x%04X, id=0x%04X", port, id);
		
		SuperIODevice *detectedDevice = createDevice(id);
		// create the device instance
		if (detectedDevice) {
			DBGLOG("ssio", "WindbondFamilyDevice detected %s, starting address sanity checks", detectedDevice->getModelName());
			uint16_t address = detectAndVerifyAddress(port, detectedDevice->getLdn());
			if (address) {
				detectedDevice->initialize(address, port, sio);
			} else {
				delete detectedDevice;
				return nullptr;
			}
		}
		// done
		leave(port);
		return detectedDevice;
	}
	
	/**
	 *  Ctors
	 */
	WindbondFamilyDevice() = default;
	
public:
	/**
	 *  Device factory
	 */
	static SuperIODevice* detect(SMCSuperIO* sio) {
		SuperIODevice *detectedDevice = probePort(SuperIOPort2E, sio);
		if (!detectedDevice) {
			detectedDevice = probePort(SuperIOPort4E, sio);
		}
		return detectedDevice;
	}

};


#endif // _WINBONDFAMILYDEVICE_HPP
