//
//  SMCDellSensors.hpp
//  SMCDellSensors
//
//  Copyright (C) 2001  Massimo Dal Zotto <dz@debian.org>
//  http://www.debian.org/~dz/i8k/
//  more work https://www.diefer.de/i8kfan/index.html , 2007
//
//  SMIMonitor ported from FakeSMC plugin (created by Slice 2014).
//

#ifndef SMCDellSensors_hpp
#define SMCDellSensors_hpp

#include "SMIMonitor.hpp"
#include <IOKit/IOService.h>
#include <IOKit/IOReportTypes.h>
#include <IOKit/acpi/IOACPIPlatformDevice.h>
#include <IOKit/pwr_mgt/IOPMPowerSource.h>
#include <IOKit/pwr_mgt/RootDomain.h>
#include <IOKit/IOTimerEventSource.h>

#include <Headers/kern_util.hpp>

class EXPORT SMCDellSensors : public IOService {
	OSDeclareDefaultStructors(SMCDellSensors)

	/**
	 *  Key name index mapping
	 */
	static constexpr size_t MaxIndexCount = sizeof("0123456789ABCDEF") - 1;
	static constexpr const char *KeyIndexes = "0123456789ABCDEF";

	/**
	 *  Key name declarations
	 */
	
	static constexpr SMC_KEY KeyFNum = SMC_MAKE_IDENTIFIER('F','N','u','m');
	static constexpr SMC_KEY KeyF0ID(size_t i) { return SMC_MAKE_IDENTIFIER('F',KeyIndexes[i],'I','D'); }
	static constexpr SMC_KEY KeyF0Ac(size_t i) { return SMC_MAKE_IDENTIFIER('F',KeyIndexes[i],'A','c'); }
	static constexpr SMC_KEY KeyF0Mn(size_t i) { return SMC_MAKE_IDENTIFIER('F',KeyIndexes[i],'M','n'); }
	static constexpr SMC_KEY KeyF0Mx(size_t i) { return SMC_MAKE_IDENTIFIER('F',KeyIndexes[i],'M','x'); }
	static constexpr SMC_KEY KeyF0Md(size_t i) { return SMC_MAKE_IDENTIFIER('F',KeyIndexes[i],'M','d'); }
	static constexpr SMC_KEY KeyF0Tg(size_t i) { return SMC_MAKE_IDENTIFIER('F',KeyIndexes[i],'T','g'); }
	// the following constant defines smc key 'FS! '
	static constexpr SMC_KEY KeyFS__ = SMC_MAKE_IDENTIFIER('F','S','!',' ');
	
	static constexpr SMC_KEY KeyTG0P(size_t i) { return SMC_MAKE_IDENTIFIER('T','G',KeyIndexes[i],'P'); }
	static constexpr SMC_KEY KeyTm0P(size_t i) { return SMC_MAKE_IDENTIFIER('T','m',KeyIndexes[i],'P'); }
	static constexpr SMC_KEY KeyTN0P(size_t i) { return SMC_MAKE_IDENTIFIER('T','N',KeyIndexes[i],'P'); }
	static constexpr SMC_KEY KeyTA0P(size_t i) { return SMC_MAKE_IDENTIFIER('T','A',KeyIndexes[i],'P'); }
	static constexpr SMC_KEY KeyTW0P(size_t i) { return SMC_MAKE_IDENTIFIER('T','W',KeyIndexes[i],'P'); }
	
	/**
	 *  VirtualSMC service registration notifier
	 */
	IONotifier *vsmcNotifier {nullptr};

	/**
	 *  Registered plugin instance
	 */
	VirtualSMCAPI::Plugin vsmcPlugin {
		xStringify(PRODUCT_NAME),
		parseModuleVersion(xStringify(MODULE_VERSION)),
		VirtualSMCAPI::Version,
	};
	
	/**
	 *  Power state name indexes
	 */
	enum PowerState {
		PowerStateOff,
		PowerStateOn,
		PowerStateMax
	};

	/**
	 *  Power states we monitor
	 */
	IOPMPowerState powerStates[PowerStateMax]  {
		{kIOPMPowerStateVersion1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{kIOPMPowerStateVersion1, kIOPMPowerOn | kIOPMDeviceUsable, kIOPMPowerOn, kIOPMPowerOn, 0, 0, 0, 0, 0, 0, 0, 0}
	};

public:
	/* Registry Entry allocation & init */

	/*! @function init
	 *   @abstract Standard init method for all IORegistryEntry subclasses.
	 *   @discussion A registry entry must be initialized with this method before it can be used. A property dictionary may passed and will be retained by this method for use as the registry entry's property table, or an empty one will be created.
	 *   @param dictionary A dictionary that will become the registry entry's property table (retaining it), or zero which will cause an empty property table to be created.
	 *   @result true on success, or false on a resource failure. */

	bool init(OSDictionary *properties = NULL) override;
	
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
	 *  Update power state with the new one, here we catch sleep/wake/boot/shutdown calls
	 *  New power state could be the reason for keystore to be saved to NVRAM, for example
	 *
	 *  @param state      power state index (must be below PowerStateMax)
	 *  @param whatDevice power state device
	 */
	IOReturn setPowerState(unsigned long state, IOService *whatDevice) override;
	
	/**
	 *  Submit the keys to received VirtualSMC service.
	 *
	 *  @param sensors   SMCBatteryManager service
	 *  @param refCon    reference
	 *  @param vsmc      VirtualSMC service
	 *  @param notifier  created notifier
	 */
	static bool vsmcNotificationHandler(void *sensors, void *refCon, IOService *vsmc, IONotifier *notifier);

	/**
	 *  Sleep handler method (required to handle sleep event as soon as possible
	 *
	 *  @param target Reference supplied when the notification was registered.
	 *  @param refCon Reference constant supplied when the notification was registered.
	 *  @param messageType Type of the message - IOKit defined in IOKit/IOMessage.h or family specific.
	 *  @param provider The IOService object who is delivering the notification. It is retained for the duration of the handler's invocation and doesn't need to be released by the handler.
	 *  @param messageArgument An argument for message, dependent on its type.
	 *  @param argSize Non zero if the argument represents a struct of that size, used when delivering messages outside the kernel.
	 */
	static IOReturn IOSleepHandler(void *target, void */*refCon*/, UInt32 messageType, IOService */*provider*/,	void *messageArgument, vm_size_t /*argSize*/);
	
	IONotifier *notifier {};
	
	IOWorkLoop *workLoop {};
	IOTimerEventSource *eventTimer {};
};

#endif /* SMCDellSensors_hpp */
