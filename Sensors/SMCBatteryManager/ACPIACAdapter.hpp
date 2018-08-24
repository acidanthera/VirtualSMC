//
//  ACPIACAdapter.hpp
//  SMCBatteryManager
//
//  Copyright © 2018 usrsse2. All rights reserved.
//

#ifndef ACPIACAdapter_hpp
#define ACPIACAdapter_hpp

#include <Library/LegacyIOService.h>
#include <IOKit/IOReportTypes.h>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
#include <IOKit/acpi/IOACPIPlatformDevice.h>
#pragma clang diagnostic pop
#include <Sensors/Private/pwr_mgt/IOPMPowerSource.h>
#include <Sensors/Private/pwr_mgt/RootDomain.h>
#include <IOKit/IOTimerEventSource.h>
#include "BatteryManagerState.hpp"

class ACPIACAdapter {
public:
	/**
	 * AC adapter device identifier
	 */
	static constexpr const char *PnpDeviceIdAcAdapter = "ACPI0003";

	/**
	 *  Dummy constructor
	 */
	ACPIACAdapter() {}

	/**
	 *  Actual constructor representing a real device with its own index and shared info struct
	 */
	ACPIACAdapter(IOACPIPlatformDevice *device, int32_t id, IOSimpleLock *lock, ACAdapterInfo *info) :
		device(device), id(id), adapterInfoLock(lock), adapterInfo(info) {}

	/**
	 *  Refresh adapter status
	 *
	 *  @return true if connected
	 */
	bool updateStatus();

private:
	/**
	 *  Relevant ACPI methods
	 */
	static constexpr const char *AcpiPowerSource = "_PSR";

	/**
	 *  Current adapter ACPI device
	 */
	IOACPIPlatformDevice *device {nullptr};

	/**
	 *  Current adapter id
	 */
	int32_t id {-1};

	/**
	 *  Reference to shared lock for refreshing adapter info
	 */
	IOSimpleLock *adapterInfoLock {nullptr};

	/**
	 *  Reference to shared adapter info
	 */
	ACAdapterInfo *adapterInfo {nullptr};
};

#endif /* ACPIACAdapter_hpp */
