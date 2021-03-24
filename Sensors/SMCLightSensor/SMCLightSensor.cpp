//
//  SMCLightSensor.cpp
//  SMCLightSensor
//
//  Copyright © 2018 usrsse2. All rights reserved.
//

#include "SMCLightSensor.hpp"
#include <Headers/kern_version.hpp>

OSDefineMetaClassAndStructors(SMCLightSensor, IOService)

bool ADDPR(debugEnabled) = true;
uint32_t ADDPR(debugPrintDelay) = 0;

bool SMCLightSensor::init(OSDictionary *dict) {
	if (!IOService::init(dict)) {
		SYSLOG("alsd", "failed to init the parent");
		return false;
	}

	atomic_init(&currentLux, 0);
	return true;
}

IOService *SMCLightSensor::probe(IOService *provider, SInt32 *score) {
	DBGLOG("alsd", "probe");

	if (!IOService::probe(provider, score))	{
		SYSLOG("alsd", "failed to probe the parent");
		return nullptr;
	}

	auto dict = IOService::nameMatching("AppleACPIPlatformExpert");
	if (!dict) {
		SYSLOG("alsd", "WTF? Failed to create matching dictionary");
		return nullptr;
	}
	
	auto acpi = IOService::waitForMatchingService(dict);
	dict->release();
	
	if (!acpi) {
		SYSLOG("alsd", "WTF? No ACPI");
		return nullptr;
	}
	
	acpi->release();
	
	dict = IOService::nameMatching("ACPI0008");
	if (!dict) {
		SYSLOG("alsd", "WTF? Failed to create matching dictionary");
		return nullptr;
	}
	
	auto deviceIterator = IOService::getMatchingServices(dict);
	dict->release();
	
	if (!deviceIterator) {
		SYSLOG("alsd", "No iterator");
		return nullptr;
	}

	alsdDevice = OSDynamicCast(IOACPIPlatformDevice, deviceIterator->getNextObject());
	deviceIterator->release();
	
	if (!alsdDevice) {
		SYSLOG("alsd", "ACPI0008 device not found");
		return nullptr;
	}

	if (alsdDevice->validateObject("_ALI") != kIOReturnSuccess || !refreshSensor(false)) {
		SYSLOG("alsd", "No functional _ALI method on ALSD device");
		return nullptr;
	}

	ALSSensor sensor {ALSSensor::Type::Unknown7, true, 6, false};
	ALSSensor noSensor {ALSSensor::Type::NoSensor, false, 0, false};
	SMCAmbientLightValue::Value emptyValue;
	lkb lkb;
	lks lks;

	VirtualSMCAPI::addKey(KeyAL, vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(0, &forceBits, SMC_KEY_ATTRIBUTE_READ | SMC_KEY_ATTRIBUTE_WRITE));
	VirtualSMCAPI::addKey(KeyALI0, vsmcPlugin.data, VirtualSMCAPI::valueWithData(
		reinterpret_cast<const SMC_DATA *>(&sensor), sizeof(sensor), SmcKeyTypeAli, nullptr, SMC_KEY_ATTRIBUTE_CONST | SMC_KEY_ATTRIBUTE_READ));
	VirtualSMCAPI::addKey(KeyALI1, vsmcPlugin.data, VirtualSMCAPI::valueWithData(
		reinterpret_cast<const SMC_DATA *>(&noSensor), sizeof(noSensor), SmcKeyTypeAli, nullptr, SMC_KEY_ATTRIBUTE_CONST | SMC_KEY_ATTRIBUTE_READ));
	VirtualSMCAPI::addKey(KeyALRV, vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(1, nullptr, SMC_KEY_ATTRIBUTE_CONST | SMC_KEY_ATTRIBUTE_READ));
	VirtualSMCAPI::addKey(KeyALV0, vsmcPlugin.data, VirtualSMCAPI::valueWithData(
		reinterpret_cast<const SMC_DATA *>(&emptyValue), sizeof(emptyValue), SmcKeyTypeAlv, new SMCAmbientLightValue(&currentLux, &forceBits),
		SMC_KEY_ATTRIBUTE_READ | SMC_KEY_ATTRIBUTE_WRITE));
	VirtualSMCAPI::addKey(KeyALV1, vsmcPlugin.data, VirtualSMCAPI::valueWithData(
		reinterpret_cast<const SMC_DATA *>(&emptyValue), sizeof(emptyValue), SmcKeyTypeAlv, nullptr,
		SMC_KEY_ATTRIBUTE_READ | SMC_KEY_ATTRIBUTE_WRITE));
	VirtualSMCAPI::addKey(KeyLKSB, vsmcPlugin.data, VirtualSMCAPI::valueWithData(reinterpret_cast<const SMC_DATA *>(&lkb), sizeof(lkb),
		SmcKeyTypeLkb, nullptr, SMC_KEY_ATTRIBUTE_READ | SMC_KEY_ATTRIBUTE_WRITE));
	VirtualSMCAPI::addKey(KeyLKSS, vsmcPlugin.data, VirtualSMCAPI::valueWithData(reinterpret_cast<const SMC_DATA *>(&lks), sizeof(lks),
		SmcKeyTypeLks, nullptr, SMC_KEY_ATTRIBUTE_READ | SMC_KEY_ATTRIBUTE_WRITE));
	VirtualSMCAPI::addKey(KeyMSLD, vsmcPlugin.data, VirtualSMCAPI::valueWithUint8(0));

	return this;
}

bool SMCLightSensor::start(IOService *provider) {
	DBGLOG("alsd", "start");
	
	if (!IOService::start(provider)) {
		SYSLOG("asld", "failed to start the parent");
		return false;
	}

	setProperty("VersionInfo", kextVersion);

	vsmcNotifier = VirtualSMCAPI::registerHandler(vsmcNotificationHandler, this);
	return vsmcNotifier != nullptr;
}

bool SMCLightSensor::vsmcNotificationHandler(void *sensors, void *refCon, IOService *vsmc, IONotifier *notifier) {
	if (sensors && vsmc) {
		DBGLOG("asld", "got vsmc notification");
		auto self = static_cast<SMCLightSensor *>(sensors);
		auto ret = vsmc->callPlatformFunction(VirtualSMCAPI::SubmitPlugin, true, sensors, &self->vsmcPlugin, nullptr, nullptr);
		if (ret == kIOReturnSuccess) {
			DBGLOG("asld", "submitted plugin");

			self->workloop = self->getWorkLoop();
			self->poller = IOTimerEventSource::timerEventSource(self, [](OSObject *object, IOTimerEventSource *sender) {
				auto ls = OSDynamicCast(SMCLightSensor, object);
				if (ls) ls->refreshSensor(true);
			});

			if (!self->poller || !self->workloop) {
				SYSLOG("asld", "failed to create poller or workloop");
				return false;
			}

			if (self->workloop->addEventSource(self->poller) != kIOReturnSuccess) {
				SYSLOG("asld", "failed to add timer event source to workloop");
				return false;
			}

			if (self->poller->setTimeoutMS(SensorUpdateTimeoutMS) != kIOReturnSuccess) {
				SYSLOG("asld", "failed to set timeout");
				return false;
			}

			return true;
		} else if (ret != kIOReturnUnsupported) {
			SYSLOG("asld", "plugin submission failure %X", ret);
		} else {
			DBGLOG("asld", "plugin submission to non vsmc");
		}
	} else {
		SYSLOG("asld", "got null vsmc notification");
	}

	return false;
}

bool SMCLightSensor::refreshSensor(bool post) {
	uint32_t lux = 0;
	auto ret = alsdDevice->evaluateInteger("_ALI", &lux);
	if (ret != kIOReturnSuccess)
		lux = 0xFFFFFFFF; // ACPI invalid

	atomic_store_explicit(&currentLux, lux, memory_order_release);

	if (post) {
		VirtualSMCAPI::postInterrupt(SmcEventALSChange);
		poller->setTimeoutMS(SensorUpdateTimeoutMS);
	}

	return ret == kIOReturnSuccess;
}

void SMCLightSensor::stop(IOService *provider) {
	PANIC("alsd", "called stop!!!");

	// In case this is supported one day
#if 0
	if (poller)
		poller->cancelTimeout();
	if (workloop && poller)
		workloop->removeEventSource(poller);
	OSSafeReleaseNULL(workloop);
	OSSafeReleaseNULL(poller);
#endif
}

EXPORT extern "C" kern_return_t ADDPR(kern_start)(kmod_info_t *, void *) {
	// Report success but actually do not start and let I/O Kit unload us.
	// This works better and increases boot speed in some cases.
	PE_parse_boot_argn("liludelay", &ADDPR(debugPrintDelay), sizeof(ADDPR(debugPrintDelay)));
	ADDPR(debugEnabled) = checkKernelArgument("-vsmcdbg") || checkKernelArgument("-alsddbg") || checkKernelArgument("-liludbgall");
	return KERN_SUCCESS;
}

EXPORT extern "C" kern_return_t ADDPR(kern_stop)(kmod_info_t *, void *) {
	// It is not safe to unload VirtualSMC plugins!
	return KERN_FAILURE;
}
