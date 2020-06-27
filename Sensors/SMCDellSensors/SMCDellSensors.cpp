//
//  SMCDellSensors.cpp
//  SMCDellSensors
//
//  Copyright © 2020 lvs1974. All rights reserved.
//

#include <VirtualSMCSDK/kern_vsmcapi.hpp>

#include "SMCDellSensors.hpp"
#include "KeyImplementations.hpp"

OSDefineMetaClassAndStructors(SMCDellSensors, IOService)

bool ADDPR(debugEnabled) = false;
uint32_t ADDPR(debugPrintDelay) = 0;

IOService *SMCDellSensors::probe(IOService *provider, SInt32 *score) {
	auto ptr = IOService::probe(provider, score);
	if (!ptr) {
		SYSLOG("sdell", "failed to probe the parent");
		return nullptr;
	}
	
	if (!SMIMonitor::getShared()->probe())
		return nullptr;
	
	auto fanCount = min(SMIMonitor::getShared()->fanCount, MaxIndexCount);
	VirtualSMCAPI::addKey(KeyFNum, vsmcPlugin.data,
		VirtualSMCAPI::valueWithUint8(fanCount, nullptr, SMC_KEY_ATTRIBUTE_CONST | SMC_KEY_ATTRIBUTE_READ));
	for (size_t i = 0; i < fanCount; i++) {
		VirtualSMCAPI::addKey(KeyF0Ac(i), vsmcPlugin.data, VirtualSMCAPI::valueWithFp(0, SmcKeyTypeFpe2, new F0Ac(i), SMC_KEY_ATTRIBUTE_WRITE | SMC_KEY_ATTRIBUTE_READ));
		VirtualSMCAPI::addKey(KeyF0As(i), vsmcPlugin.data, VirtualSMCAPI::valueWithUint8(0, new F0As(i), SMC_KEY_ATTRIBUTE_READ));
		VirtualSMCAPI::addKey(KeyF0Mn(i), vsmcPlugin.data, VirtualSMCAPI::valueWithFp(0, SmcKeyTypeFpe2, new F0Mn(i), SMC_KEY_ATTRIBUTE_CONST | SMC_KEY_ATTRIBUTE_READ));
		VirtualSMCAPI::addKey(KeyF0Mx(i), vsmcPlugin.data, VirtualSMCAPI::valueWithFp(0, SmcKeyTypeFpe2, new F0Mx(i), SMC_KEY_ATTRIBUTE_CONST | SMC_KEY_ATTRIBUTE_READ));
	}
	
	auto tempCount = min(SMIMonitor::getShared()->tempCount, MaxIndexCount);
	for (size_t i = 0; i < tempCount; i++) {
		IOSimpleLockLock(SMIMonitor::getShared()->stateLock);
		TempInfo::SMMTempSensorType type = SMIMonitor::getShared()->state.tempInfo[i].type;
		IOSimpleLockUnlock(SMIMonitor::getShared()->stateLock);
		if (type <= TempInfo::Unsupported || type >= TempInfo::Last) {
			DBGLOG("sdell", "Temp sensor type %d is unknown, auto assign value %d", type, SMIMonitor::getShared()->state.tempInfo[i].index);
			type = static_cast<TempInfo::SMMTempSensorType>(SMIMonitor::getShared()->state.tempInfo[i].index);
		}
		
		switch (type)
		{
		case TempInfo::CPU:
			VirtualSMCAPI::addKey(KeyTC0P(0), vsmcPlugin.data, VirtualSMCAPI::valueWithSp(0, SmcKeyTypeSp78, new TC0P(i),
				SMC_KEY_ATTRIBUTE_WRITE|SMC_KEY_ATTRIBUTE_READ));
			break;
		case TempInfo::GPU:
			VirtualSMCAPI::addKey(KeyTG0P(0), vsmcPlugin.data, VirtualSMCAPI::valueWithSp(0, SmcKeyTypeSp78, new TG0P(i),
				SMC_KEY_ATTRIBUTE_WRITE|SMC_KEY_ATTRIBUTE_READ));
			break;
		case TempInfo::Memory:
			VirtualSMCAPI::addKey(KeyTm0P(0), vsmcPlugin.data, VirtualSMCAPI::valueWithSp(0, SmcKeyTypeSp78, new Tm0P(i),
				SMC_KEY_ATTRIBUTE_WRITE|SMC_KEY_ATTRIBUTE_READ));
			break;
		case TempInfo::Misc:
			VirtualSMCAPI::addKey(KeyTN0P(0), vsmcPlugin.data, VirtualSMCAPI::valueWithSp(0, SmcKeyTypeSp78, new TN0P(i),
				SMC_KEY_ATTRIBUTE_WRITE|SMC_KEY_ATTRIBUTE_READ));
			break;
		case TempInfo::Ambient:
			VirtualSMCAPI::addKey(KeyTA0P(0), vsmcPlugin.data, VirtualSMCAPI::valueWithSp(0, SmcKeyTypeSp78, new TA0P(i),
				SMC_KEY_ATTRIBUTE_WRITE|SMC_KEY_ATTRIBUTE_READ));
			break;
		case TempInfo::Other:
			VirtualSMCAPI::addKey(KeyTW0P(0), vsmcPlugin.data, VirtualSMCAPI::valueWithSp(0, SmcKeyTypeSp78, new TW0P(i),
				SMC_KEY_ATTRIBUTE_WRITE|SMC_KEY_ATTRIBUTE_READ));
			break;
		default:
			DBGLOG("sdell", "Temp sensor type %d is unsupported", type);
			break;
		}
	}
		
	for (size_t i = 0; i < fanCount; i++) {
		FanTypeDescStruct	desc;
		char fan_name[DiagFunctionStrLen];
		snprintf(fan_name, DiagFunctionStrLen, "Fan %lu", i);
		lilu_os_strncpy(desc.strFunction, fan_name, DiagFunctionStrLen);
		IOSimpleLockLock(SMIMonitor::getShared()->stateLock);
		VirtualSMCAPI::addKey(KeyF0ID(i), vsmcPlugin.data, VirtualSMCAPI::valueWithData(
			reinterpret_cast<const SMC_DATA *>(&desc), sizeof(desc), SmcKeyTypeFds, nullptr, SMC_KEY_ATTRIBUTE_CONST | SMC_KEY_ATTRIBUTE_READ));
		IOSimpleLockUnlock(SMIMonitor::getShared()->stateLock);
	}
	
	return this;
}

bool SMCDellSensors::start(IOService *provider) {
	if (!IOService::start(provider)) {
		SYSLOG("sdell", "failed to start the parent");
		return false;
	}
	
	SMIMonitor::getShared()->start();
	
	vsmcNotifier = VirtualSMCAPI::registerHandler(vsmcNotificationHandler, this);
	return vsmcNotifier != nullptr;
}

bool SMCDellSensors::vsmcNotificationHandler(void *sensors, void *refCon, IOService *vsmc, IONotifier *notifier) {
	if (sensors && vsmc) {
		DBGLOG("sdell", "got vsmc notification");
		auto &plugin = static_cast<SMCDellSensors *>(sensors)->vsmcPlugin;
		auto ret = vsmc->callPlatformFunction(VirtualSMCAPI::SubmitPlugin, true, sensors, &plugin, nullptr, nullptr);
		if (ret == kIOReturnSuccess) {
			DBGLOG("sdell", "submitted plugin");
			return true;
		} else if (ret != kIOReturnUnsupported) {
			SYSLOG("sdell", "plugin submission failure %X", ret);
		} else {
			DBGLOG("sdell", "plugin submission to non vsmc");
		}
	} else {
		SYSLOG("sdell", "got null vsmc notification");
	}
	return false;
}

void SMCDellSensors::stop(IOService *provider) {
	PANIC("sdell", "called stop!!!");
}

EXPORT extern "C" kern_return_t ADDPR(kern_start)(kmod_info_t *, void *) {
	// Report success but actually do not start and let I/O Kit unload us.
	// This works better and increases boot speed in some cases.
	PE_parse_boot_argn("liludelay", &ADDPR(debugPrintDelay), sizeof(ADDPR(debugPrintDelay)));
	ADDPR(debugEnabled) = checkKernelArgument("-vsmcdbg") || checkKernelArgument("-sdelldbg");
	SMIMonitor::createShared();
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
