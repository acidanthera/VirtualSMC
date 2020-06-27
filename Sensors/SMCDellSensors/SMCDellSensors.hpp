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
#include <Library/LegacyIOService.h>
#include <IOKit/IOReportTypes.h>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
#include <IOKit/acpi/IOACPIPlatformDevice.h>
#pragma clang diagnostic pop
#include <Sensors/Private/pwr_mgt/IOPMPowerSource.h>
#include <Sensors/Private/pwr_mgt/RootDomain.h>
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
	static constexpr SMC_KEY KeyF0As(size_t i) { return SMC_MAKE_IDENTIFIER('F',KeyIndexes[i],'A','s'); }
	static constexpr SMC_KEY KeyF0Mn(size_t i) { return SMC_MAKE_IDENTIFIER('F',KeyIndexes[i],'M','n'); }
	static constexpr SMC_KEY KeyF0Mx(size_t i) { return SMC_MAKE_IDENTIFIER('F',KeyIndexes[i],'M','x'); }
	
	static constexpr SMC_KEY KeyTC0P(size_t i) { return SMC_MAKE_IDENTIFIER('T','C',KeyIndexes[i],'P'); }
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

public:
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
	 *  Submit the keys to received VirtualSMC service.
	 *
	 *  @param sensors   SMCBatteryManager service
	 *  @param refCon    reference
	 *  @param vsmc      VirtualSMC service
	 *  @param notifier  created notifier
	 */
	static bool vsmcNotificationHandler(void *sensors, void *refCon, IOService *vsmc, IONotifier *notifier);
};

#endif /* SMCDellSensors_hpp */
