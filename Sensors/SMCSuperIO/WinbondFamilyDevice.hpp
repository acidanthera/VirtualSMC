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

class WindbondFamilyDevice : public SuperIODevice {
protected:
	/**
	 *  Hardware access methods
	 */
	static inline void enter(i386_ioport_t port) {
		::outb(port, 0x87);
		::outb(port, 0x87);
	}
	
	static inline void exit(i386_ioport_t port) {
		::outb(port, 0xAA);
	}
	
	/**
	 *  Windbond-family common precedure to detect device address.
	 *  NOTE: the device must be in entered state before the call to this method.
	 */
	static uint16_t detectAndVerifyAddress(i386_ioport_t port, uint8_t ldn);
	
	/**
	 *  Device factory
	 */
	template<typename D, typename DD>
	static SuperIODevice* detect(SMCSuperIO* sio) {
		SuperIODevice *detectedDevice;
		if (nullptr == (detectedDevice = probePort<D, DD>(SuperIOPort2E, sio))) {
			detectedDevice = probePort<D, DD>(SuperIOPort4E, sio);
		}
		if (detectedDevice) {
			return detectedDevice;
		}
		return nullptr;
	}
	
	/**
	 *  Device factory helper
	 */
	template<typename D, typename DD>
	static SuperIODevice* probePort(i386_ioport_t port, SMCSuperIO* sio) {
		enter(port);
		uint16_t id = listenPortWord(port, SuperIOChipIDRegister);
		DBGLOG("ssio", "probing device on 0x%4X, id=0x%4X", port, id);
		
		SuperIODevice *detectedDevice = nullptr;
		uint8_t ldn = WinbondHardwareMonitorLDN;
		const DD *desc = D::detectModel(id, ldn);
		
		// create the device instance
		if (desc) {
			DBGLOG("ssio", "detected %s, starting address sanity checks", getModelName(desc->ID));
			uint16_t address = detectAndVerifyAddress(port, ldn);
			if (address) {
				detectedDevice = new D(*desc, address, port, sio);
			}
		}
		// done
		exit(port);
		return detectedDevice;
	}
	
	/**
	 *  Ctors
	 */
	WindbondFamilyDevice(SuperIOModel model, uint16_t address, i386_ioport_t port, SMCSuperIO* sio)
	: SuperIODevice(model, address, port, sio) {}
	WindbondFamilyDevice() = delete;
};


#endif // _WINBONDFAMILYDEVICE_HPP
