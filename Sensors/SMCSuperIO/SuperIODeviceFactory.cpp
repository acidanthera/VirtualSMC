//
//  SuperIODeviceFactory.cpp
//
//  Detects SuperIO Chip installed
//
//  Based on https://github.com/kozlek/HWSensors/blob/master/SuperIOSensors/SuperIODevice.cpp
//  @author joedm.
//

#include <VirtualSMCSDK/kern_vsmcapi.hpp>
#include <IOKit/IOLib.h>

#include "SuperIODeviceFactory.hpp"
#include "NuvotonDevice.hpp"
#include "ITEDevice.hpp"

bool SuperIODeviceFactory::detectWinbondFamilyChip()
{
	for (int i = 0; i < 2; i++) {
		port = kSuperIOPorts[i];
		winbond_family_enter(port);
		id = superio_listen_port_word(port, kSuperIOChipIDRegister);
		DBGLOG("ssio", "probing device on 0x%x, id=0x%x", port, id);

		switch (id) {
				// Fintek
			case F71858:
				model = id;
				ldn = kF71858HardwareMonitorLDN;
				vendor = "Fintek";
				break;
				
			case F71862:
			case F71868A:
			case F71869:
			case F71869A:
			case F71882:
			case F71889AD:
			case F71889ED:
			case F71889F:
			case F71808E:
				model = id;
				ldn = kFintekITEHardwareMonitorLDN;
				vendor = "Fintek";
				break;
				
			default:
				switch (id >> 8) {
						// Winbond
					case 0x52:
						switch (id & 0xff) {
							case 0x17:
							case 0x3A:
							case 0x41:
								model = W83627HF;
								ldn = kWinbondHardwareMonitorLDN;
								vendor = "Winbond";
								break;
						}
						break;
						
					case 0x82:
						switch (id & 0xf0) {
							case 0x80:
								model = W83627THF;
								ldn = kWinbondHardwareMonitorLDN;
								vendor = "Winbond";
								break;
						}
						break;
						
					case 0x85:
						switch (id & 0xff) {
							case 0x41:
								model = W83687THF;
								ldn = kWinbondHardwareMonitorLDN;
								vendor = "Winbond";
								break;
						}
						break;
						
					case 0x88:
						switch (id & 0xf0) {
							case 0x50:
							case 0x60:
								model = W83627EHF;
								ldn = kWinbondHardwareMonitorLDN;
								vendor = "Winbond";
								break;
						}
						break;
						
					case 0xA0:
						switch (id & 0xf0) {
							case 0x20:
								model = W83627DHG;
								ldn = kWinbondHardwareMonitorLDN;
								vendor = "Winbond";
								break;
						}
						break;
						
					case 0xA5:
						switch (id & 0xf0) {
							case 0x10:
								model = W83667HG;
								ldn = kWinbondHardwareMonitorLDN;
								vendor = "Winbond";
								break;
						}
						break;
						
					case 0xB0:
						switch (id & 0xf0) {
							case 0x70:
								model = W83627DHGP;
								ldn = kWinbondHardwareMonitorLDN;
								vendor = "Winbond";
								break;
						}
						break;
						
					case 0xB3:
						switch (id & 0xf0) {
							case 0x50:
								model = W83667HGB;
								ldn = kWinbondHardwareMonitorLDN;
								vendor = "Winbond";
								break;
						}
						break;
						
						// Nuvoton
					case 0xB4:
						switch (id & 0xf0) {
							case 0x70:
								model = NCT6771F;
								ldn = kWinbondHardwareMonitorLDN;
								vendor = "Nuvoton";
								detectedDevice = new NuvotonNCT6771FDevice();
								break;
						}
						break;
						
					case 0xC3:
						switch (id & 0xf0) {
							case 0x30:
								model = NCT6776F;
								ldn = kWinbondHardwareMonitorLDN;
								vendor = "Nuvoton";
								detectedDevice = new NuvotonNCT6776FDevice();
								break;
						}
						break;
						
					case 0xC5:
						switch (id & 0xf0) {
							case 0x60:
								model = NCT6779D;
								ldn = kWinbondHardwareMonitorLDN;
								vendor = "Nuvoton";
								detectedDevice = new NuvotonNCT6779DDevice();
								break;
						}
						break;
						
					case 0xC8:
						switch (id & 0xff) {
							case 0x03:
								model = NCT6791D;
								ldn = kWinbondHardwareMonitorLDN;
								vendor = "Nuvoton";
								detectedDevice = new NuvotonNCT679xxDevice(model);
								break;
						}
						break;
						
					case 0xC9:
						switch (id & 0xff) {
							case 0x11:
								model = NCT6792D;
								ldn = kWinbondHardwareMonitorLDN;
								vendor = "Nuvoton";
								detectedDevice = new NuvotonNCT679xxDevice(model);
								break;
						}
						break;
						
					case 0xD1:
						switch (id & 0xff) {
							case 0x21:
								model = NCT6793D;
								ldn = kWinbondHardwareMonitorLDN;
								vendor = "Nuvoton";
								detectedDevice = new NuvotonNCT679xxDevice(model);
								break;
						}
						break;
						
					case 0xD3:
						switch (id & 0xff) {
							case 0x52:
								model = NCT6795D;
								ldn = kWinbondHardwareMonitorLDN;
								vendor = "Nuvoton";
								detectedDevice = new NuvotonNCT679xxDevice(model);
								break;
						}
						break;
						
					case 0xD4:
						switch (id & 0xff) {
							case 0x23:
								model = NCT6796D;
								ldn = kWinbondHardwareMonitorLDN;
								vendor = "Nuvoton";
								detectedDevice = new NuvotonNCT679xxDevice(model);
								break;
						}
						break;
						
				}
				break;
		}
		
		if (model != 0 && ldn != 0 && detectedDevice) {
			UInt16 verify = 0;
			DBGLOG("ssio", "detected %s %s, starting address sanity checks", vendor, superio_get_model_name(model));
			superio_select_logical_device(port, ldn);
			address = superio_listen_port_word(port, kSuperIOBaseAddressRegister);
			IOSleep(50);
			verify = superio_listen_port_word(port, kSuperIOBaseAddressRegister);
			winbond_family_exit(port);
			if (address != verify) {
				DBGLOG("ssio", "address verify check error!");
				delete detectedDevice;
				detectedDevice = nullptr;
				continue;
			}
			// some Fintek chips have address register offset 0x05 added already
			if ((address & 0x07) == 0x05) {
				address &= 0xFFF8;
			}
			if (address < 0x100 || (address & 0xF007) != 0) {
				DBGLOG("ssio", "address is out of bounds!");
				delete detectedDevice;
				detectedDevice = nullptr;
				continue;
			}
			detectedDevice->deviceAddress = address;
			detectedDevice->devicePort = port;
			return true;
		} else  {
			winbond_family_exit(port);
		}
	}
	
	return false;
}

bool SuperIODeviceFactory::detectITEFamilyChip()
{
	// IT87XX can enter only on port 0x2E
	port = 0x2E;
	ite_family_enter(port);
	id = superio_listen_port_word(port, kSuperIOChipIDRegister);
	DBGLOG("ssio", "probing device on 0x%x, id=0x%x", port, id);
	model = id;
	ldn = kFintekITEHardwareMonitorLDN;
	vendor = "ITE";

	switch (id) {
		case IT8512F:
			detectedDevice = new ITE::Device(ITE::Device::_IT8512F);
			break;
		case IT8705F:
			detectedDevice = new ITE::Device(ITE::Device::_IT8705F);
			break;
		case IT8712F:
			detectedDevice = new ITE::Device(ITE::Device::_IT8712F);
			break;
		case IT8716F:
			detectedDevice = new ITE::Device(ITE::Device::_IT8716F);
			break;
		case IT8718F:
			detectedDevice = new ITE::Device(ITE::Device::_IT8718F);
			break;
		case IT8720F:
			detectedDevice = new ITE::Device(ITE::Device::_IT8720F);
			break;
		case IT8721F:
			detectedDevice = new ITE::Device(ITE::Device::_IT8721F);
			break;
		case IT8726F:
			detectedDevice = new ITE::Device(ITE::Device::_IT8726F);
			break;
		case IT8620E:
			detectedDevice = new ITE::Device(ITE::Device::_IT8620E);
			break;
		case IT8628E:
			detectedDevice = new ITE::Device(ITE::Device::_IT8628E);
			break;
		case IT8686E:
			detectedDevice = new ITE::Device(ITE::Device::_IT8686E);
			break;
		case IT8728F:
			detectedDevice = new ITE::Device(ITE::Device::_IT8728F);
			break;
		case IT8752F:
			detectedDevice = new ITE::Device(ITE::Device::_IT8752F);
			break;
		case IT8771E:
			detectedDevice = new ITE::Device(ITE::Device::_IT8771E);
			break;
		case IT8772E:
			detectedDevice = new ITE::Device(ITE::Device::_IT8772E);
			break;
		case IT8792E:
			detectedDevice = new ITE::Device(ITE::Device::_IT8792E);
			break;
		default:
			ite_family_exit(port);
			return false;
	}
	
	DBGLOG("ssio", "detected %s %s, starting address sanity checks", vendor, superio_get_model_name(model));
	superio_select_logical_device(port, ldn);
	IOSleep(10);
	address = superio_listen_port_word(port, kSuperIOBaseAddressRegister);
	IOSleep(10);
	UInt16 verify = superio_listen_port_word(port, kSuperIOBaseAddressRegister);
	IOSleep(10);
	ite_family_exit(port);
	
	if (address != verify || address < 0x100 || (address & 0xF007) != 0) {
		return false;
	}
	detectedDevice->deviceAddress = address;
	detectedDevice->devicePort = port;
	return true;
}

SuperIODevice* SuperIODeviceFactory::detectAndCreate(SMCSuperIO* sio, bool isGigabyte)
{
	// Gigabyte mobos usualy use ITE
	if (isGigabyte) {
		if (!detectITEFamilyChip()) {
			UInt16 ite_id = id;
			if (!detectWinbondFamilyChip()) {
				SYSLOG("ssio", "found unsupported chip! ITE sequence ID=0x%x, Winbond sequence ID=0x%x", ite_id, id);
				return nullptr;
			}
		}
	} else {
		// Other vendors usualy use Winbond family chipsets
		if (!detectWinbondFamilyChip()) {
			UInt16 wnbnd_id = id;
			if (!detectITEFamilyChip()) {
				SYSLOG("ssio", "found unsupported chip! ITE sequence ID=0x%x, Winbond sequence ID=0x%x", id, wnbnd_id);
				return nullptr;
			}
		}
	}

	SYSLOG("ssio", "found %s %s on port=0x%x address=0x%x", vendor, superio_get_model_name(model), port, address);

	detectedDevice->smcSuperIO = sio;
	return detectedDevice;
}

