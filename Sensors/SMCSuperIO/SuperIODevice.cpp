//
//  SuperIODevice.cpp
//
//  SuperIO Chip data
//
//  Based on https://github.com/kozlek/HWSensors/blob/master/SuperIOSensors/SuperIODevice.cpp
//  @author joedm.
//

#include "SMCSuperIO.hpp"
#include "SuperIODevice.hpp"

/**
 *  Keys
 */
SMC_RESULT TachometerKey::readAccess() {
	IOSimpleLockLock(sio->counterLock);
	double val = (double)device->getTachometerValue(index);
	*reinterpret_cast<uint16_t *>(data) = VirtualSMCAPI::encodeFp(SmcKeyTypeFpe2, val);
	sio->quickReschedule();
	IOSimpleLockUnlock(sio->counterLock);
	return SmcSuccess;
}
