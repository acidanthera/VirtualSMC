//
//  Devices.hpp
//  VirtualSMC
//
//  Copyright © 2019 joedm. All rights reserved.
//

#ifndef Devices_hpp
#define Devices_hpp

#include "SuperIODevice.hpp"

SuperIODevice *createDevice(uint16_t deviceId);
SuperIODevice *createDeviceITE(uint16_t deviceId);
SuperIODevice *createDeviceEC(const char *name);

#endif /* Devices_hpp */
