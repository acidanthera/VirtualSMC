//
//  SMCBatteryManager.hpp
//  SMCBatteryManager
//
//  Copyright © 2018 usrsse2. All rights reserved.
//
//  Portions copyright © 2009 Superhai.
//  VoodooBattery. Contact http://www.superhai.com/
//

#ifndef BatteryManagerBase_hpp
#define BatteryManagerBase_hpp

#include "ACPIBattery.hpp"
#include "ACPIACAdapter.hpp"
#include "BatteryManagerState.hpp"
#include <IOKit/IOService.h>
#include <IOKit/IOReportTypes.h>
#include <IOKit/acpi/IOACPIPlatformDevice.h>
#include <IOKit/pwr_mgt/IOPMPowerSource.h>
#include <IOKit/pwr_mgt/RootDomain.h>
#include <IOKit/IOTimerEventSource.h>
#include <Headers/kern_util.hpp>
#include <stdatomic.h>

class EXPORT BatteryManager : public OSObject {
	OSDeclareDefaultStructors(BatteryManager)
	
public:
	/**
	 *  Actual battery count
	 */
	uint32_t batteriesCount {0};

	/**
	 *  Actual AC adapter count
	 */
	uint32_t adapterCount {0};

	/**
	 *  Run battery information refresh in quick mode
	 */
	_Atomic(uint32_t) quickPoll;

	/**
	 *  QuickPoll will be disable when all battery has average rate available from EC
	 */
	bool quickPollDisabled {true};

	/**
	 *  State lock, every state access must be guarded by this lock
	 */
	IOSimpleLock *stateLock {nullptr};

	/**
	 *  Main refreshed battery state containing battery information
	 */
	BatteryManagerState state {};

	/**
	 *  Probe battery manager
	 *
	 *  @return true on sucess
	 */
	bool probe();

	/**
	 *  Start battery manager up
	 */
	void start();

	/**
	 *  Power source interest handler type
	 */
	using PowerSourceInterestHandler = IOReturn (*)(void *target);

	/**
	 *  Subscribe to power source interest changes
	 *  Only one handler could be subscribed to atm.
	 *
	 *  @param h  handler to be called
	 *  @param t  handler target (context)
	 */
	void subscribe(PowerSourceInterestHandler h, void *t);

	/**
	 *  Wake battery manager from sleep or some other power state change
	 */
	void wake();

	/**
	 *  Sleep battery manager
	 */
	void sleep();
	
	/**
	 *  Checks whether any battery is connected, must be guarded by stateLock
	 *
	 *  @return true on success
	 */
	bool batteriesConnected();
	
	/**
	 *  Checks whether any AC adapter is connected, must be guarded by stateLock
	 *
	 *  @return true on success
	 */
	bool adaptersConnected();
	
	/**
	 *  Checks whether the batteries are full, must be guarded by stateLock
	 *
	 *  @return true on success
	 */
	bool batteriesAreFull();

	/**
	 *  Checks whether any adapter is connected, must be guarded by stateLock
	 *
	 *  @return true on success
	 */
	bool externalPowerConnected();

	/**
	 *  Allocate battery manager instance, can only be called once
	 */
	static void createShared();

	/**
	 *  Obtain shared instance for later usage
	 *
	 *  @return battery manager shared instance
	 */
	static BatteryManager *getShared() {
		return instance;
	}
	
	/**
	 *  Calculate battery status in AlarmWarning format
	 *
	 *  @param  index battery index
	 *
	 *  @return calculated value
	 */
	uint16_t calculateBatteryStatus(size_t index);

	/**
	 *  Last access time by SMCSMBusController
	 */
	uint64_t lastAccess {0};

private:
	/**
	 *  The only allowed battery manager instance
	 */
	static BatteryManager *instance;

	/**
	 *  Not publicly exported notifications
	 */
	enum {
		kIOPMSetValue				= 1 << 16,
		kIOPMSetDesktopMode			= 1 << 17,
		kIOPMSetACAdaptorConnected	= 1 << 18
	};

	/**
	 *  Battery information providers
	 */
	ACPIBattery batteries[BatteryManagerState::MaxBatteriesSupported] {};

	/**
	 *  Battery event notifiers
	 */
	IONotifier *batteryNotifiers[BatteryManagerState::MaxBatteriesSupported] {};

	/**
	 *  AC adapter information providers
	 */
	ACPIACAdapter adapters[BatteryManagerState::MaxAcAdaptersSupported] {};

	/**
	 *  Adapter event notifiers
	 */
	IONotifier *adapterNotifiers[BatteryManagerState::MaxAcAdaptersSupported] {};

	/**
	 *  A lock to permit concurrent access
	 */
	IOLock *mainLock {nullptr};

	/**
	 *  Workloop used to poll battery updates on timer basis
	 */
	IOWorkLoop *workloop {nullptr};

	/**
	 *  Workloop timer event sources for refresh scheduling
	 */
	IOTimerEventSource *timerEventSource {nullptr};

	/**
	 *  Initial device check on startup
	 */
	bool initialCheckDevices {false};

	/**
	 *  Handle notification about battery or AC adapter (dis)connection
	 *
	 *  @param messageType      kIOACPIMessageDeviceNotification
	 *  @param provider         The ACPI device being connected or disconnected
	 *  @param messageArgument  nullptr
	 *
	 *  @return kIOReturnSuccess
	 */
	static IOReturn acpiNotification(void *target, void *refCon, UInt32 messageType, IOService *provider, void *messageArgument, vm_size_t argSize);

	/**
	 *  Find available batteries on this system, must be guarded by mainLock
	 *
	 *  @return true on sucess
	 */
	bool findBatteries();

	/**
	 *  Find available ac adapters on this system, must be guarded by mainLock
	 *
	 *  @return true on sucess
	 */
	bool findACAdapters();

	/**
	 *  Check devices for changes, must be guarded by mainLock
	 */
	void checkDevices();

	/**
	 *  Post external power plug-in/plug-out update
	 *
	 *  @param status  true on AC adapter connect
	 */
	void externalPowerNotify(bool status);

	/**
	 *  Decrement quickPoll if positive
	 *
	 *  @return true if decremented
	 */
	bool decrementQuickPoll();

	/**
	 *  Stored subscribed power source change handler target
	 */
	_Atomic(void *) handlerTarget;

	/**
	 *  Stored subscribed power source change handler
	 */
	_Atomic(PowerSourceInterestHandler) handler;
};

#endif /* BatteryManagerBase_hpp */
