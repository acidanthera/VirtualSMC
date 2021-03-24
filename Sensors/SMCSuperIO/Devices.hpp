//
//  Devices.hpp
//  VirtualSMC
//
//  Copyright © 2019 joedm. All rights reserved.
//

#ifndef Devices_hpp
#define Devices_hpp

#include "SuperIODevice.hpp"

#define EC_ENDPOINT 0xFF

SuperIODevice *createDevice(uint16_t deviceId);
SuperIODevice *createDeviceITE(uint16_t deviceId);

#endif /* Devices_hpp */
