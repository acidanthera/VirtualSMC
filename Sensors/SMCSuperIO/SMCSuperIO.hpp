//
//  SMCSuperIO.hpp
//  SuperIO Sensors
//
//  Based on some code found across Internet. We respect those code authors and their work.
//  Copyright © 2018 vit9696. All rights reserved.
//  Copyright © 2018 joedm. All rights reserved.
//

#ifndef _SMCSUPERIO_HPP
#define _SMCSUPERIO_HPP

#include <IOKit/IOService.h>

#include <Headers/kern_util.hpp>
#include <Headers/kern_cpu.hpp>
#include <Headers/kern_time.hpp>
#include <VirtualSMCSDK/kern_vsmcapi.hpp>

class SuperIODevice;

class EXPORT SMCSuperIO : public IOService {
	OSDeclareDefaultStructors(SMCSuperIO)

public:
	/**
	 *  Power state name indexes
	 */
	enum PowerState {
		PowerStateOff,
		PowerStateOn,
		PowerStateMax
	};
	
private:
	/**
	 *  Power states we monitor
	 */
	IOPMPowerState powerStates[PowerStateMax]  {
		{kIOPMPowerStateVersion1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{kIOPMPowerStateVersion1, kIOPMPowerOn | kIOPMDeviceUsable, kIOPMPowerOn, kIOPMPowerOn, 0, 0, 0, 0, 0, 0, 0, 0}
	};
	
	/**
	 *  Update power state with the new one, here we catch sleep/wake/boot/shutdown calls
	 *  New power state could be the reason for keystore to be saved to NVRAM, for example
	 *
	 *  @param state      power state index (must be below PowerStateMax)
	 *  @param whatDevice power state device
	 */
	IOReturn setPowerState(unsigned long state, IOService *whatDevice) override;
	
	/**
	 *  VirtualSMC service registration notifier
	 */
	IONotifier *vsmcNotifier {nullptr};

	/**
	 *  Detected SuperIO device instance
	 */
	SuperIODevice *dataSource {nullptr};
	
	/**
	 *  Registered plugin instance
	 */
	VirtualSMCAPI::Plugin vsmcPlugin {
		xStringify(PRODUCT_NAME),
		parseModuleVersion(xStringify(MODULE_VERSION)),
		VirtualSMCAPI::Version,
	};

	/**
	 *  Workloop used to poll counters updates on timer basis
	 */
	IOWorkLoop *workloop {nullptr};

	/**
	 *  Workloop timer event sources for refresh scheduling
	 */
	IOTimerEventSource *timerEventSource {nullptr};

	/**
	 *  Default event timer timeout
	 */
	static constexpr uint32_t TimerTimeoutMs {500};

	/**
	 *  Maximum delta between timer triggers for a reschedule
	 */
	static constexpr uint64_t MaxDeltaForRescheduleNs {convertScToNs(10)};

	/**
	 *  Minimum delta between energy calculation count
	 */
	static constexpr uint64_t MinDeltaForRescheduleNs {convertMsToNs(100)};

	/**
	 *  Last timer trigger timestamp in nanoseconds
	 */
	uint64_t timerEventLastTime {0};

	/**
	 *  Timer scheduling status
	 */
	bool timerEventScheduled {false};

	/**
	 *  Refresh sensor state on timer basis
	 */
	void timerCallback();

	/**
	 *  Detect a SuperIO device installed.
	 *  Returns nullptr if no supported device found. Otherwise returns a SuperIODevice instance.
	 */
	SuperIODevice* detectDevice();
public:
	/**
	 *  Sensor access lock
	 */
	IOSimpleLock *counterLock {nullptr};
	
	/**
	 *  Decide on whether to load or not by checking the processor compatibility.
	 *
	 *  @param provider  parent IOService object
	 *  @param score     probing score
	 *
	 *  @return self if we could load anyhow
	 */
	IOService *probe(IOService *provider, SInt32 *score) override;

	/**
	 *  Add VirtualSMC listening notification.
	 *
	 *  @param provider  parent IOService object
	 *
	 *  @return true on success
	 */
	bool start(IOService *provider) override;

	/**
	 *  Detect unloads, though they are prohibited.
	 *
	 *  @param provider  parent IOService object
	 */
	void stop(IOService *provider) override;

	/**
	 *  Do a quick refresh reschedule if necessary
	 */
	void quickReschedule();

	/**
	 *  Submit the keys to received VirtualSMC service.
	 *
	 *  @param sensors   SensorsCPU service
	 *  @param refCon    reference
	 *  @param vsmc      VirtualSMC service
	 *  @param notifier  created notifier
	 */
	static bool vsmcNotificationHandler(void *sensors, void *refCon, IOService *vsmc, IONotifier *notifier);
};

#endif // _SMCSUPERIO_HPP
