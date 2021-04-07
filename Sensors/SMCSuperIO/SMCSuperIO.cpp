//
//  SMCSuperIO.cpp
//  SuperIO Sensors
//
//  Based on some code found across Internet. We respect those code authors and their work.
//  Copyright © 2018 vit9696. All rights reserved.
//  Copyright © 2018 joedm. All rights reserved.
//

#include <VirtualSMCSDK/kern_vsmcapi.hpp>
#include <Headers/kern_time.hpp>
#include <Headers/kern_version.hpp>
#include <IOKit/IODeviceTreeSupport.h>
#include <IOKit/IOTimerEventSource.h>

#include "SMCSuperIO.hpp"
#include "SuperIODevice.hpp"
#include "WinbondFamilyDevice.hpp"
#include "ITEDevice.hpp"
#include "ECDevice.hpp"

OSDefineMetaClassAndStructors(SMCSuperIO, IOService)

bool ADDPR(debugEnabled) = false;
uint32_t ADDPR(debugPrintDelay) = 0;

void SMCSuperIO::timerCallback() {
	dataSource->update();
	// timerEventSource->setTimeoutMS calls thread_call_enter_delayed_with_leeway, which spins.
	// If the previous one was too long ago, schedule another one for differential recalculation!
	timerEventSource->setTimeoutMS(TimerTimeoutMs);
	atomic_flag_clear_explicit(&timerEventScheduled, memory_order_release);
}

IOService *SMCSuperIO::probe(IOService *provider, SInt32 *score) {
	return IOService::probe(provider, score);
}

bool SMCSuperIO::start(IOService *provider) {
	SYSLOG("ssio", "starting up SuperIO sensors");

	if (!IOService::start(provider)) {
		SYSLOG("ssio", "failed to start the parent");
		return false;
	}

	setProperty("VersionInfo", kextVersion);

	auto ioreg = OSDynamicCast(IORegistryEntry, this);

	dataSource = detectDevice();
	if (!dataSource) {
		SYSLOG("ssio", "failed to detect supported SuperIO chip");
		return false;
	}

	// Prepare time sources and event loops
	workloop = IOWorkLoop::workLoop();
	timerEventSource = IOTimerEventSource::timerEventSource(this, [](OSObject *object, IOTimerEventSource *sender) {
		auto cp = OSDynamicCast(SMCSuperIO, object);
		if (cp) {
			cp->timerCallback();
		}
	});

	if (!timerEventSource || !workloop) {
		SYSLOG("ssio", "failed to create workloop or timer event source");
		goto startFailed;
	}
	if (workloop->addEventSource(timerEventSource) != kIOReturnSuccess) {
		SYSLOG("ssio", "failed to add timer event source");
		goto startFailed;
	}

	dataSource->setupKeys(vsmcPlugin);
	SYSLOG("ssio", "detected device %s", dataSource->getModelName());

	PMinit();
	provider->joinPMtree(this);
	registerPowerDriver(this, powerStates, arrsize(powerStates));
	
	timerEventSource->setTimeoutMS(TimerTimeoutMs * 2);
	vsmcNotifier = VirtualSMCAPI::registerHandler(vsmcNotificationHandler, this);
	
	// set the chip name to IORegistry
	if (ioreg) {
		auto chipName = OSString::withCString(dataSource->getModelName());
		const OSSymbol *key = OSSymbol::withCString("ChipName");
		ioreg->setProperty(key, chipName);
		chipName->release();
		key->release();
	}
	// done
	DBGLOG("ssio", "starting up SuperIO sensors done %d", vsmcNotifier != nullptr);
	return vsmcNotifier != nullptr;

startFailed:
	OSSafeReleaseNULL(workloop);
	OSSafeReleaseNULL(timerEventSource);
	return false;
}

void SMCSuperIO::quickReschedule() {
	if (!atomic_flag_test_and_set_explicit(&timerEventScheduled, memory_order_acquire)) {
		// Make it 10 times faster
		timerEventSource->setTimeoutMS(TimerTimeoutMs/10);
	}
}

bool SMCSuperIO::vsmcNotificationHandler(void *sensors, void *refCon, IOService *vsmc, IONotifier *notifier) {
	if (sensors && vsmc) {
		DBGLOG("ssio", "got vsmc notification");
		auto &plugin = static_cast<SMCSuperIO *>(sensors)->vsmcPlugin;
		auto ret = vsmc->callPlatformFunction(VirtualSMCAPI::SubmitPlugin, true, sensors, &plugin, nullptr, nullptr);
		if (ret == kIOReturnSuccess) {
			DBGLOG("ssio", "submitted plugin");
			return true;
		} else if (ret != kIOReturnUnsupported) {
			SYSLOG("ssio", "plugin submission failure %X", ret);
		} else {
			DBGLOG("ssio", "plugin submission to non vsmc");
		}
	} else {
		SYSLOG("ssio", "got null vsmc notification");
	}
	return false;
}

void SMCSuperIO::stop(IOService *provider) {
	PMstop();
}

IOReturn SMCSuperIO::setPowerState(unsigned long state, IOService *whatDevice) {
	DBGLOG("ssio", "changing power state to %lu", state);
	
	if (dataSource) {
		dataSource->powerStateChanged(state);
	}
	
	return kIOPMAckImplied;
}

SuperIODevice* SMCSuperIO::detectDevice() {
	SuperIODevice* detectedDevice;

	auto lpc = getParentEntry(gIOServicePlane);
	if (lpc != nullptr) {
		if (PE_parse_boot_argn("ssioec", &deviceNameEC, sizeof(deviceNameEC))) {
			DBGLOG("ssio", "found EC device %s", deviceNameEC);
		} else {
			auto name = lpc->getProperty("ec-device");
			auto nameStr = OSDynamicCast(OSString, name);
			auto nameData = OSDynamicCast(OSData, name);
			if (nameStr) {
				strlcpy(deviceNameEC, nameStr->getCStringNoCopy(), sizeof(deviceNameEC));
				DBGLOG("ssio", "found EC device %s from string", deviceNameEC);
			} else if (nameData) {
				auto s = nameData->getLength();
				if (s > sizeof(deviceNameEC)) s = sizeof(deviceNameEC);
				strlcpy(deviceNameEC, static_cast<const char *>(nameData->getBytesNoCopy()), s);
				DBGLOG("ssio", "found EC device %s from data", deviceNameEC);
			}
		}
		detectedDevice = EC::ECDevice::detect(this, deviceNameEC, lpc);
		if (detectedDevice) {
			return detectedDevice;
		}
	}

	detectedDevice = WindbondFamilyDevice::detect(this);
	if (detectedDevice) {
		return detectedDevice;
	}
	detectedDevice = ITE::ITEDevice::detect(this);
	if (detectedDevice) {
		return detectedDevice;
	}
	return nullptr;
}

EXPORT extern "C" kern_return_t ADDPR(kern_start)(kmod_info_t *, void *) {
	// Report success but actually do not start and let I/O Kit unload us.
	// This works better and increases boot speed in some cases.
	PE_parse_boot_argn("liludelay", &ADDPR(debugPrintDelay), sizeof(ADDPR(debugPrintDelay)));
	ADDPR(debugEnabled) = checkKernelArgument("-vsmcdbg") || checkKernelArgument("-ssiodbg") || checkKernelArgument("-liludbgall");
	return KERN_SUCCESS;
}

EXPORT extern "C" kern_return_t ADDPR(kern_stop)(kmod_info_t *, void *) {
	// It is not safe to unload VirtualSMC plugins!
	return KERN_FAILURE;
}
