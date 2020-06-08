//
//  SMCBatteryManager.cpp
//  SMCBatteryManager
//
//  Copyright © 2018 usrsse2. All rights reserved.
//

#include <VirtualSMCSDK/kern_vsmcapi.hpp>

#include "SMCBatteryManager.hpp"
#include "KeyImplementations.hpp"

OSDefineMetaClassAndStructors(SMCBatteryManager, IOService)

bool ADDPR(debugEnabled) = false;
uint32_t ADDPR(debugPrintDelay) = 0;

IOService *SMCBatteryManager::probe(IOService *provider, SInt32 *score) {
	auto ptr = IOService::probe(provider, score);
	if (!ptr) {
		SYSLOG("sbat", "failed to probe the parent");
		return nullptr;
	}
	
	if (!BatteryManager::getShared()->probe())
		return nullptr;

	//WARNING: watch out, key addition is sorted here!

	//FIXME: This needs to be implemented along with AC-W!
	// VirtualSMCAPI::addKey(KeyAC_N, vsmcPlugin.data, VirtualSMCAPI::valueWithUint8(0, new AC_N(batteryManager)));

	if (BatteryManager::getShared()->adapterCount > 0) {
		VirtualSMCAPI::addKey(KeyACEN, vsmcPlugin.data, VirtualSMCAPI::valueWithUint8(0, new ACIN));
		VirtualSMCAPI::addKey(KeyACFP, vsmcPlugin.data, VirtualSMCAPI::valueWithFlag(false, new ACIN));
		VirtualSMCAPI::addKey(KeyACID, vsmcPlugin.data, VirtualSMCAPI::valueWithData(nullptr, 8, SmcKeyTypeCh8s, new ACID));
		VirtualSMCAPI::addKey(KeyACIN, vsmcPlugin.data, VirtualSMCAPI::valueWithFlag(false, new ACIN));
	}

	auto batCount = min(BatteryManager::getShared()->batteriesCount, MaxIndexCount);
	for (size_t i = 0; i < batCount; i++) {
		VirtualSMCAPI::addKey(KeyB0AC(i), vsmcPlugin.data, VirtualSMCAPI::valueWithSint16(400, new B0AC(i), SMC_KEY_ATTRIBUTE_PRIVATE_WRITE|SMC_KEY_ATTRIBUTE_WRITE|SMC_KEY_ATTRIBUTE_READ));
		VirtualSMCAPI::addKey(KeyB0AV(i), vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(13000, new B0AV(i)));
		VirtualSMCAPI::addKey(KeyB0BI(i), vsmcPlugin.data, VirtualSMCAPI::valueWithUint8(1, new B0BI(i)));
		VirtualSMCAPI::addKey(KeyB0CT(i), vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(1, new B0CT(i), SMC_KEY_ATTRIBUTE_PRIVATE_WRITE|SMC_KEY_ATTRIBUTE_WRITE|SMC_KEY_ATTRIBUTE_READ));
		VirtualSMCAPI::addKey(KeyB0FC(i), vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(4000, new B0FC(i), SMC_KEY_ATTRIBUTE_PRIVATE_WRITE|SMC_KEY_ATTRIBUTE_WRITE|SMC_KEY_ATTRIBUTE_READ));
		VirtualSMCAPI::addKey(KeyB0PS(i), vsmcPlugin.data, VirtualSMCAPI::valueWithData(nullptr, 2, SmcKeyTypeHex, new B0PS(i), SMC_KEY_ATTRIBUTE_PRIVATE_WRITE|SMC_KEY_ATTRIBUTE_WRITE|SMC_KEY_ATTRIBUTE_READ));
		VirtualSMCAPI::addKey(KeyB0RM(i), vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(2000, new B0RM(i), SMC_KEY_ATTRIBUTE_PRIVATE_WRITE|SMC_KEY_ATTRIBUTE_WRITE|SMC_KEY_ATTRIBUTE_READ));
		VirtualSMCAPI::addKey(KeyB0St(i), vsmcPlugin.data, VirtualSMCAPI::valueWithData(nullptr, 2, SmcKeyTypeHex, new B0St(i), SMC_KEY_ATTRIBUTE_PRIVATE_WRITE|SMC_KEY_ATTRIBUTE_WRITE|SMC_KEY_ATTRIBUTE_READ));
		VirtualSMCAPI::addKey(KeyB0TF(i), vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(0, new B0TF(i)));
	}
	
	VirtualSMCAPI::addKey(KeyBATP, vsmcPlugin.data, VirtualSMCAPI::valueWithFlag(true, new BATP));
	VirtualSMCAPI::addKey(KeyBBAD, vsmcPlugin.data, VirtualSMCAPI::valueWithFlag(false, new BBAD));
	VirtualSMCAPI::addKey(KeyBBIN, vsmcPlugin.data, VirtualSMCAPI::valueWithFlag(true, new BBIN));
	VirtualSMCAPI::addKey(KeyBFCL, vsmcPlugin.data, VirtualSMCAPI::valueWithUint8(100, new BFCL, SMC_KEY_ATTRIBUTE_READ | SMC_KEY_ATTRIBUTE_WRITE));
	VirtualSMCAPI::addKey(KeyBNum, vsmcPlugin.data, VirtualSMCAPI::valueWithUint8(1, new BNum));
	VirtualSMCAPI::addKey(KeyBRSC, vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(40, new BRSC, SMC_KEY_ATTRIBUTE_READ | SMC_KEY_ATTRIBUTE_WRITE | SMC_KEY_ATTRIBUTE_PRIVATE_WRITE));
	VirtualSMCAPI::addKey(KeyBSIn, vsmcPlugin.data, VirtualSMCAPI::valueWithUint8(0, new BSIn));

	VirtualSMCAPI::addKey(KeyCHLC, vsmcPlugin.data, VirtualSMCAPI::valueWithUint8(1, new CHLC));

#if 0
	for (size_t i = 0; i < batCount; i++) {
		//FIXME: DOIR and B0AC are both battery current, but need to check format, units etc. System doesn't read them, does iStat?
		VirtualSMCAPI::addKey(KeyD0IR(i), vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(0, new DOIR(i)));
		VirtualSMCAPI::addKey(KeyD0VM(i), vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(0, new D0VM(i)));
		VirtualSMCAPI::addKey(KeyD0VR(i), vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(0, new D0VR(i)));
		VirtualSMCAPI::addKey(KeyD0VX(i), vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(0, new D0VX(i)));
	}
#endif

	//TODO: implement the keys below as well
	// IB0R: sp4s or sp5s
	// IBAC: sp7s
	// PB0R = IB0R * VP0R

	return this;
}

bool SMCBatteryManager::start(IOService *provider) {
	if (!IOService::start(provider)) {
		SYSLOG("sbat", "failed to start the parent");
		return false;
	}
	
	BatteryManager::getShared()->start();

	vsmcNotifier = VirtualSMCAPI::registerHandler(vsmcNotificationHandler, this);
	return vsmcNotifier != nullptr;
}

bool SMCBatteryManager::vsmcNotificationHandler(void *sensors, void *refCon, IOService *vsmc, IONotifier *notifier) {
	if (sensors && vsmc) {
		DBGLOG("sbat", "got vsmc notification");
		auto &plugin = static_cast<SMCBatteryManager *>(sensors)->vsmcPlugin;
		auto ret = vsmc->callPlatformFunction(VirtualSMCAPI::SubmitPlugin, true, sensors, &plugin, nullptr, nullptr);
		if (ret == kIOReturnSuccess) {
			DBGLOG("sbat", "submitted plugin");
			return true;
		} else if (ret != kIOReturnUnsupported) {
			SYSLOG("sbat", "plugin submission failure %X", ret);
		} else {
			DBGLOG("sbat", "plugin submission to non vsmc");
		}
	} else {
		SYSLOG("sbat", "got null vsmc notification");
	}
	return false;
}

void SMCBatteryManager::stop(IOService *provider) {
	PANIC("sbat", "called stop!!!");
}

EXPORT extern "C" kern_return_t ADDPR(kern_start)(kmod_info_t *, void *) {
	// Report success but actually do not start and let I/O Kit unload us.
	// This works better and increases boot speed in some cases.
	PE_parse_boot_argn("liludelay", &ADDPR(debugPrintDelay), sizeof(ADDPR(debugPrintDelay)));
	ADDPR(debugEnabled) = checkKernelArgument("-vsmcdbg") || checkKernelArgument("-sbatdbg");
	BatteryManager::createShared();
	return KERN_SUCCESS;
}

EXPORT extern "C" kern_return_t ADDPR(kern_stop)(kmod_info_t *, void *) {
	// It is not safe to unload VirtualSMC plugins!
	return KERN_FAILURE;
}

#ifdef __MAC_10_15

// macOS 10.15 adds Dispatch function to all OSObject instances and basically
// every header is now incompatible with 10.14 and earlier.
// Here we add a stub to permit older macOS versions to link.
// Note, this is done in both kern_util and plugin_start as plugins will not link
// to Lilu weak exports from vtable.

kern_return_t WEAKFUNC PRIVATE OSObject::Dispatch(const IORPC rpc) {
	PANIC("util", "OSObject::Dispatch smcbat stub called");
}

kern_return_t WEAKFUNC PRIVATE OSMetaClassBase::Dispatch(const IORPC rpc) {
	PANIC("util", "OSMetaClassBase::Dispatch smcbat stub called");
}

#endif
