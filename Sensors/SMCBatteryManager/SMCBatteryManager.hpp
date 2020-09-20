//
//  SMCBatteryManager.hpp
//  SMCBatteryManager
//
//  Copyright © 2018 usrsse2. All rights reserved.
//
//  Portions copyright © 2009 Superhai.
//

#ifndef SMCBatteryManager_hpp
#define SMCBatteryManager_hpp

#include "BatteryManager.hpp"
#include <IOKit/IOService.h>
#include <IOKit/IOReportTypes.h>
#include <IOKit/acpi/IOACPIPlatformDevice.h>
#include <IOKit/pwr_mgt/IOPMPowerSource.h>
#include <IOKit/pwr_mgt/RootDomain.h>
#include <IOKit/IOTimerEventSource.h>

#include <Headers/kern_util.hpp>

class EXPORT SMCBatteryManager : public IOService {
	OSDeclareDefaultStructors(SMCBatteryManager)

	/**
	 *  Key name index mapping
	 */
	static constexpr size_t MaxIndexCount = sizeof("0123456789ABCDEF") - 1;
	static constexpr const char *KeyIndexes = "0123456789ABCDEF";

	/**
	 *  Key name declarations
	 */
	static constexpr SMC_KEY KeyAC_N = SMC_MAKE_IDENTIFIER('A','C','-','N');
	static constexpr SMC_KEY KeyACEN = SMC_MAKE_IDENTIFIER('A','C','E','N');
	static constexpr SMC_KEY KeyACFP = SMC_MAKE_IDENTIFIER('A','C','F','P');
	static constexpr SMC_KEY KeyACID = SMC_MAKE_IDENTIFIER('A','C','I','D');
	static constexpr SMC_KEY KeyACIN = SMC_MAKE_IDENTIFIER('A','C','I','N');
	static constexpr SMC_KEY KeyBATP = SMC_MAKE_IDENTIFIER('B','A','T','P');
	static constexpr SMC_KEY KeyBBAD = SMC_MAKE_IDENTIFIER('B','B','A','D');
	static constexpr SMC_KEY KeyBBIN = SMC_MAKE_IDENTIFIER('B','B','I','N');
	static constexpr SMC_KEY KeyBFCL = SMC_MAKE_IDENTIFIER('B','F','C','L');
	static constexpr SMC_KEY KeyBNum = SMC_MAKE_IDENTIFIER('B','N','u','m');
	static constexpr SMC_KEY KeyBRSC = SMC_MAKE_IDENTIFIER('B','R','S','C');
	static constexpr SMC_KEY KeyBSIn = SMC_MAKE_IDENTIFIER('B','S','I','n');
	static constexpr SMC_KEY KeyCHBI = SMC_MAKE_IDENTIFIER('C','H','B','I');
	static constexpr SMC_KEY KeyCHBV = SMC_MAKE_IDENTIFIER('C','H','B','V');
	static constexpr SMC_KEY KeyCHLC = SMC_MAKE_IDENTIFIER('C','H','L','C');

	static constexpr SMC_KEY KeyB0AC(size_t i) { return SMC_MAKE_IDENTIFIER('B',KeyIndexes[i],'A','C'); }
	static constexpr SMC_KEY KeyB0AV(size_t i) { return SMC_MAKE_IDENTIFIER('B',KeyIndexes[i],'A','V'); }
	static constexpr SMC_KEY KeyB0BI(size_t i) { return SMC_MAKE_IDENTIFIER('B',KeyIndexes[i],'B','I'); }
	static constexpr SMC_KEY KeyB0CT(size_t i) { return SMC_MAKE_IDENTIFIER('B',KeyIndexes[i],'C','T'); }
	static constexpr SMC_KEY KeyB0FC(size_t i) { return SMC_MAKE_IDENTIFIER('B',KeyIndexes[i],'F','C'); }
	static constexpr SMC_KEY KeyB0PS(size_t i) { return SMC_MAKE_IDENTIFIER('B',KeyIndexes[i],'P','S'); }
	static constexpr SMC_KEY KeyB0RM(size_t i) { return SMC_MAKE_IDENTIFIER('B',KeyIndexes[i],'R','M'); }
	static constexpr SMC_KEY KeyB0St(size_t i) { return SMC_MAKE_IDENTIFIER('B',KeyIndexes[i],'S','t'); }
	static constexpr SMC_KEY KeyB0TF(size_t i) { return SMC_MAKE_IDENTIFIER('B',KeyIndexes[i],'T','F'); }
	static constexpr SMC_KEY KeyD0IR(size_t i) { return SMC_MAKE_IDENTIFIER('D',KeyIndexes[i],'I','R'); }
	static constexpr SMC_KEY KeyD0VM(size_t i) { return SMC_MAKE_IDENTIFIER('D',KeyIndexes[i],'V','M'); }
	static constexpr SMC_KEY KeyD0VR(size_t i) { return SMC_MAKE_IDENTIFIER('D',KeyIndexes[i],'V','R'); }
	static constexpr SMC_KEY KeyD0VX(size_t i) { return SMC_MAKE_IDENTIFIER('D',KeyIndexes[i],'V','X'); }
	static constexpr SMC_KEY KeyTB0T(size_t i) { return SMC_MAKE_IDENTIFIER('T','B',KeyIndexes[i],'T'); }

	// Keys used in BigSur 10.16 and later
#if 0
	static constexpr SMC_KEY KeyBNCB = SMC_MAKE_IDENTIFIER('B','N','C','B');
	static constexpr SMC_KEY KeyCHII = SMC_MAKE_IDENTIFIER('C','H','I','I');
	static constexpr SMC_KEY KeyAC_W = SMC_MAKE_IDENTIFIER('A','C','-','W');
	static constexpr SMC_KEY KeyD0PT(size_t i) { return SMC_MAKE_IDENTIFIER('D',KeyIndexes[i],'P','T'); }
	static constexpr SMC_KEY KeyD0BD(size_t i) { return SMC_MAKE_IDENTIFIER('D',KeyIndexes[i],'B','D'); }
	static constexpr SMC_KEY KeyD0ER(size_t i) { return SMC_MAKE_IDENTIFIER('D',KeyIndexes[i],'E','R'); }
	static constexpr SMC_KEY KeyD0DE(size_t i) { return SMC_MAKE_IDENTIFIER('D',KeyIndexes[i],'D','E'); }
	static constexpr SMC_KEY KeyD0is(size_t i) { return SMC_MAKE_IDENTIFIER('D',KeyIndexes[i],'i','s'); }
	static constexpr SMC_KEY KeyD0if(size_t i) { return SMC_MAKE_IDENTIFIER('D',KeyIndexes[i],'i','f'); }
	static constexpr SMC_KEY KeyD0ih(size_t i) { return SMC_MAKE_IDENTIFIER('D',KeyIndexes[i],'i','h'); }
	static constexpr SMC_KEY KeyD0ii(size_t i) { return SMC_MAKE_IDENTIFIER('D',KeyIndexes[i],'i','i'); }
	static constexpr SMC_KEY KeyD0in(size_t i) { return SMC_MAKE_IDENTIFIER('D',KeyIndexes[i],'i','n'); }
	static constexpr SMC_KEY KeyD0im(size_t i) { return SMC_MAKE_IDENTIFIER('D',KeyIndexes[i],'i','m'); }
	static constexpr SMC_KEY KeyD0PI(size_t i) { return SMC_MAKE_IDENTIFIER('D',KeyIndexes[i],'P','I'); }
	static constexpr SMC_KEY KeyD0FC(size_t i) { return SMC_MAKE_IDENTIFIER('D',KeyIndexes[i],'F','C'); }
#endif
	static constexpr SMC_KEY KeyBC1V(size_t i) { return SMC_MAKE_IDENTIFIER('B','C',KeyIndexes[i],'V'); }
	
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

#endif /* SMCBatteryManager_hpp */
