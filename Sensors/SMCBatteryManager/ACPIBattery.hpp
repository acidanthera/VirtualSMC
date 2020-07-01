//
//  ACPIBattery.hpp
//  SMCBatteryManager
//
//  Copyright © 2018 usrsse2. All rights reserved.
//

#ifndef ACPIBattery_hpp
#define ACPIBattery_hpp

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

class ACPIBattery {
public:

	/**
	 * Battery device identifier
	 */
	static constexpr const char *PnpDeviceIdBattery = "PNP0C0A";

	/**
	 *  Peak accuracy for average in percents
	 */
	static constexpr uint8_t AverageBoundPercent = 25;

	/**
	 *  Quick battery info refresh in milliseconds
	 */
	static constexpr uint32_t QuickPollInterval = 1000;

	/**
	 *  Full battery info refresh in milliseconds
	 */
	static constexpr uint32_t NormalPollInterval = 60000;

	/**
	 *  Time to handle ACPI notification in milliseconds
	 */
	static constexpr uint32_t QuickPollPeriod = 60000;

	/**
	 *  Amount of quick polls to do on ACPI notification
	 */
	static constexpr uint32_t QuickPollCount = QuickPollPeriod / QuickPollInterval;

	/**
	 *  Battery status obtained from ACPI _BST
	 */
	enum {
		/**
		 Means that the battery is not charging and not discharging.
		 It may happen when the battery is fully charged,
		 or when AC adapter is connected but charging has not started yet,
		 or when charging is limited to some percent in BIOS settings.
		 */
		BSTNotCharging  = 0,
		BSTDischarging  = 1 << 0,
		BSTCharging     = 1 << 1,
		BSTCritical     = 1 << 2,
		BSTStateMask    = BSTNotCharging | BSTDischarging | BSTCharging,
	};

	/**
	 *  Reference to shared lock for refreshing battery info
	 */
	IOSimpleLock *batteryInfoLock {nullptr};

	/**
	 *  Reference to shared battery info
	 */
	BatteryInfo *batteryInfo {nullptr};

	/**
	 *  Dummy constructor
	 */
	ACPIBattery() {}

	/**
	 *  Actual constructor representing a real device with its own index and shared info struct
	 */
	ACPIBattery(IOACPIPlatformDevice *device, int32_t id, IOSimpleLock *lock, BatteryInfo *info) :
		device(device), id(id), batteryInfoLock(lock), batteryInfo(info) {}

	/**
	 *  Refresh battery static information
	 *  WARNING: updateStaticStatus and updateRealTimeStatus should run sequentially!
	 *
	 *  @param quickPoll  do a quick update
	 *
	 *  @return true when battery is fully charged
	 */
	bool updateRealTimeStatus(bool quickPoll);

	/**
	 *  Refresh battery static information
	 *  WARNING: updateStaticStatus and updateRealTimeStatus should run sequentially!
	 *
	 *  @param calculatedACAdapterConnection  optional ac adapter status if assumed
	 *
	 *  @retun true when battery is connected
	 */
	bool updateStaticStatus(bool *calculatedACAdapterConnection=nullptr);
	
	/**
	 * Calculate value for B0St SMC key and corresponding SMBus command
	 *
	 *  @return value
	 */
	uint16_t calculateBatteryStatus();

private:
	uint32_t getNumberFromArray(OSArray *array, uint32_t index);

	void getStringFromArray(OSArray *array, uint32_t index, char *dst, uint32_t dstSize);

	/**
	 *  Obtain aggregated battery information from ACPI
	 *
	 *  @param bi        battery info to update
	 *  @param extended  read extended info
	 *
	 *  @return true on success
	 */
	bool getBatteryInfo(BatteryInfo &bi, bool extended);

	/**
	 *  Current battery id
	 */
	int32_t id {-1};

	/**
	 *  Current battery ACPI device
	 */
	IOACPIPlatformDevice *device {nullptr};

	/**
	 *  Related ACPI methods
	 */
	static constexpr const char *AcpiStatus               = "_STA";
	static constexpr const char *AcpiBatteryInformation   = "_BIF";
	static constexpr const char *AcpiBatteryInformationEx = "_BIX";
	static constexpr const char *AcpiBatteryStatus        = "_BST";

	/**
	 *  Battery Static Information pack layout
	 */
	enum {
		BIFPowerUnit,                    // Integer (DWORD)
		BIFDesignCapacity,               // Integer (DWORD)
		BIFLastFullChargeCapacity,       // Integer (DWORD)
		BIFBatteryTechnology,            // Integer (DWORD)
		BIFDesignVoltage,                // Integer (DWORD)
		BIFDesignCapacityOfWarning,      // Integer (DWORD)
		BIFDesignCapacityOfLow,          // Integer (DWORD)
		BIFBatteryCapacityGranularity1,  // Integer (DWORD)
		BIFBatteryCapacityGranularity2,  // Integer (DWORD)
		BIFModelNumber,                  // String (ASCIIZ)
		BIFSerialNumber,                 // String (ASCIIZ)
		BIFBatteryType,                  // String (ASCIIZ)
		BIFOEMInformation                // String (ASCIIZ)
	};

	/**
	 *  Extended Battery Static Information pack layout
	 */
	enum {
		BIXRevision,                    //Integer
		BIXPowerUnit,
		BIXDesignCapacity,
		BIXLastFullChargeCapacity,      //Integer (DWORD)
		BIXBatteryTechnology,
		BIXDesignVoltage,
		BIXDesignCapacityOfWarning,
		BIXDesignCapacityOfLow,
		BIXCycleCount,                  //Integer (DWORD)
		BIXMeasurementAccuracy,         //Integer (DWORD)
		BIXMaxSamplingTime,             //Integer (DWORD)
		BIXMinSamplingTime,             //Integer (DWORD)
		BIXMaxAveragingInterval,        //Integer (DWORD)
		BIXMinAveragingInterval,        //Integer (DWORD)
		BIXBatteryCapacityGranularity1,
		BIXBatteryCapacityGranularity2,
		BIXModelNumber,
		BIXSerialNumber,
		BIXBatteryType,
		BIXOEMInformation,
	};

	/**
	 *  Battery Real-time Information pack layout
	 */
	enum {
		BSTState,
		BSTPresentRate,
		BSTRemainingCapacity,
		BSTPresentVoltage
	};
};

#endif /* ACPIBattery_hpp */
