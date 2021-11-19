//
//  SMCDellSensors.cpp
//  SMCDellSensors
//
//  Copyright © 2020 lvs1974. All rights reserved.
//

#include <VirtualSMCSDK/kern_vsmcapi.hpp>

#include "SMCDellSensors.hpp"
#include "KeyImplementations.hpp"
#include <Headers/plugin_start.hpp>
#include <Headers/kern_version.hpp>
#include <IOKit/pwr_mgt/RootDomain.h>

#include "kern_hooks.hpp"

OSDefineMetaClassAndStructors(SMCDellSensors, IOService)

bool SMCDellSensors::init(OSDictionary *properties) {
	if (!IOService::init(properties)) {
		return false;
	}

	if (!SMIMonitor::getShared())
		SMIMonitor::createShared();

	SMIMonitor::getShared()->fanMult = 1; //linux proposed to get nominal speed and if it high then change multiplier
	OSNumber * Multiplier = OSDynamicCast(OSNumber, properties->getObject("FanMultiplier"));
	if (Multiplier)
		SMIMonitor::getShared()->fanMult = Multiplier->unsigned32BitValue();
	return true;
}

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
		VirtualSMCAPI::addKey(KeyF0Mn(i), vsmcPlugin.data, VirtualSMCAPI::valueWithFp(0, SmcKeyTypeFpe2, new F0Mn(i), SMC_KEY_ATTRIBUTE_WRITE | SMC_KEY_ATTRIBUTE_READ));
		VirtualSMCAPI::addKey(KeyF0Mx(i), vsmcPlugin.data, VirtualSMCAPI::valueWithFp(0, SmcKeyTypeFpe2, new F0Mx(i), SMC_KEY_ATTRIBUTE_WRITE | SMC_KEY_ATTRIBUTE_READ));
		VirtualSMCAPI::addKey(KeyF0Md(i), vsmcPlugin.data, VirtualSMCAPI::valueWithUint8(0, new F0Md(i), SMC_KEY_ATTRIBUTE_WRITE | SMC_KEY_ATTRIBUTE_READ));
		VirtualSMCAPI::addKey(KeyF0Tg(i), vsmcPlugin.data, VirtualSMCAPI::valueWithFp(0, SmcKeyTypeFpe2, new F0Tg(i), SMC_KEY_ATTRIBUTE_WRITE | SMC_KEY_ATTRIBUTE_READ));
	}
	VirtualSMCAPI::addKey(KeyFS__, vsmcPlugin.data,
		VirtualSMCAPI::valueWithUint16(0, new FS__(), SMC_KEY_ATTRIBUTE_WRITE | SMC_KEY_ATTRIBUTE_READ));

	OSArray* fanNames = OSDynamicCast(OSArray, getProperty("FanNames"));
	OSArray* stopFanOffsets = OSDynamicCast(OSArray, getProperty("StopFanOffsets"));
	char fan_name[DiagFunctionStrLen];

	for (unsigned int i = 0, cpu = 0; i < fanCount; i++) {
		FanTypeDescStruct	desc;
		FanInfo::SMMFanType type = SMIMonitor::getShared()->state.fanInfo[i].type;
		if (type == FanInfo::Unsupported) {
			auto auto_type = (cpu++ == 0) ? FanInfo::CPU : FanInfo::GPU;
			DBGLOG("sdell", "Fan type %d is unknown, auto assign value %d", type, auto_type);
			type = auto_type;
		}
		snprintf(fan_name, DiagFunctionStrLen, "Fan %u", i);
		if (fanNames && type < fanNames->getCount()) {
			OSString* name = OSDynamicCast(OSString, fanNames->getObject(type));
			if (name)
				lilu_os_strncpy(fan_name, name->getCStringNoCopy(), DiagFunctionStrLen);
		}
		if (stopFanOffsets && i < stopFanOffsets->getCount()) {
			OSNumber* offset = OSDynamicCast(OSNumber, stopFanOffsets->getObject(i));
			if (offset)
				SMIMonitor::getShared()->state.fanInfo[i].stopOffset = offset->unsigned32BitValue();
		}
		lilu_os_strncpy(desc.strFunction, fan_name, DiagFunctionStrLen);
		VirtualSMCAPI::addKey(KeyF0ID(i), vsmcPlugin.data, VirtualSMCAPI::valueWithData(
			reinterpret_cast<const SMC_DATA *>(&desc), sizeof(desc), SmcKeyTypeFds, nullptr, SMC_KEY_ATTRIBUTE_CONST | SMC_KEY_ATTRIBUTE_READ));
	}

	auto tempCount = min(SMIMonitor::getShared()->tempCount, MaxIndexCount);
	for (size_t i = 0; i < tempCount; i++) {
		TempInfo::SMMTempSensorType type = SMIMonitor::getShared()->state.tempInfo[i].type;
		if (type <= TempInfo::Unsupported || type >= TempInfo::Last) {
			DBGLOG("sdell", "Temp sensor type %d is unknown, auto assign value %d", type, SMIMonitor::getShared()->state.tempInfo[i].index);
			type = static_cast<TempInfo::SMMTempSensorType>(SMIMonitor::getShared()->state.tempInfo[i].index);
		}

		switch (type)
		{
		case TempInfo::CPU:
			DBGLOG("sdell", "CPU Proximity sensor is handled by SMCProcessor plugin");
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

	qsort(const_cast<VirtualSMCKeyValue *>(vsmcPlugin.data.data()), vsmcPlugin.data.size(), sizeof(VirtualSMCKeyValue), VirtualSMCKeyValue::compare);

	return this;
}

bool SMCDellSensors::start(IOService *provider) {
	if (!IOService::start(provider)) {
		SYSLOG("sdell", "failed to start the parent");
		return false;
	}

	setProperty("VersionInfo", kextVersion);

	notifier = registerSleepWakeInterest(IOSleepHandler, this);
	if (notifier == NULL) {
		SYSLOG("sdell", "failed to register sleep/wake interest");
		return false;
	}
	
	if (!eventTimer) {
		if (!workLoop)
			workLoop = IOWorkLoop::workLoop();

		if (workLoop) {
			eventTimer = IOTimerEventSource::timerEventSource(workLoop,
			[](OSObject *owner, IOTimerEventSource *) {
				SMIMonitor::getShared()->handlePowerOn();
			});

			if (eventTimer) {
				IOReturn result = workLoop->addEventSource(eventTimer);
				if (result != kIOReturnSuccess) {
					SYSLOG("sdell", "SMCDellSensors addEventSource failed");
					OSSafeReleaseNULL(eventTimer);
				}
			}
			else
				SYSLOG("sdell", "SMCDellSensors timerEventSource failed");
		}
		else
			SYSLOG("sdell", "SMCDellSensors IOService instance does not have workLoop");
	}

	SMIMonitor::getShared()->start();

	PMinit();
	provider->joinPMtree(this);
	registerPowerDriver(this, powerStates, arrsize(powerStates));

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

IOReturn SMCDellSensors::setPowerState(unsigned long state, IOService *whatDevice){
	DBGLOG("sdell", "changing power state to %lu", state);
	return kIOPMAckImplied;
}

IOReturn SMCDellSensors::IOSleepHandler(void *target, void *, UInt32 messageType, IOService *provider, void *messageArgument, vm_size_t)
{
	sleepWakeNote *swNote = (sleepWakeNote *)messageArgument;

	if (messageType != kIOMessageSystemWillSleep &&
		messageType != kIOMessageSystemHasPoweredOn &&
		messageType != kIOMessageSystemWillNotSleep &&
		messageType != kIOMessageSystemWillPowerOff &&
		messageType != kIOMessageSystemWillRestart) {
		return kIOReturnUnsupported;
	}

	DBGLOG("sdell", "IOSleepHandler message type = 0x%x", messageType);

	swNote->returnValue = 0;
	acknowledgeSleepWakeNotification(swNote->powerRef);

	auto that = OSDynamicCast(SMCDellSensors, reinterpret_cast<SMCDellSensors*>(target));

	if (messageType == kIOMessageSystemWillSleep || messageType == kIOMessageSystemWillPowerOff || messageType == kIOMessageSystemWillRestart) {
		if (that && that->eventTimer)
			that->eventTimer->cancelTimeout();
		SMIMonitor::getShared()->handlePowerOff();
	} else if (messageType == kIOMessageSystemHasPoweredOn || messageType == kIOMessageSystemWillNotSleep) {
		auto that = OSDynamicCast(SMCDellSensors, reinterpret_cast<SMCDellSensors*>(target));
		if (that && that->eventTimer)
			that->eventTimer->setTimeoutMS(10000);
		else
			SMIMonitor::getShared()->handlePowerOn();
	}

	return kIOReturnSuccess;
}

