//
//  ACPIACAdapter.cpp
//  SMCBatteryManager
//
//  Copyright © 2018 usrsse2. All rights reserved.
//

#include "ACPIACAdapter.hpp"

#include <Headers/kern_util.hpp>

bool ACPIACAdapter::updateStatus() {
	uint32_t acpi = 0;
	bool connected = false;
	if (device->evaluateInteger(AcpiPowerSource, &acpi) == kIOReturnSuccess) {
		DBGLOG("acpic", "return power source %x for %d", acpi, id);
		connected = acpi ? true : false;
	}

	IOSimpleLockLock(adapterInfoLock);
	adapterInfo->connected = connected;
	IOSimpleLockUnlock(adapterInfoLock);

	return connected;
}
