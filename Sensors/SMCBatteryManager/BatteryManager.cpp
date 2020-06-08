//
//  SMCBatteryManager.cpp
//  SMCBatteryManager
//
//  Copyright © 2018 usrsse2. All rights reserved.
//

#include "BatteryManager.hpp"

BatteryManager *BatteryManager::instance = nullptr;

OSDefineMetaClassAndStructors(BatteryManager, OSObject)

void BatteryInfo::validateData(int32_t id) {
	if (!state.designVoltage)
		state.designVoltage = DummyVoltage;
	if (state.powerUnitIsWatt) {
    auto mV = state.designVoltage;
		DBGLOG("binfo", "battery %d design voltage %d,%03d", id, mV / 1000, mV % 1000);
		if (designCapacity * 1000 / mV < 900) {
			SYSLOG("binfo", "battery %d reports mWh but uses mAh (%u)", id, designCapacity);
			state.powerUnitIsWatt = false;
		} else {
			designCapacity = designCapacity * 1000 / mV;
			state.lastFullChargeCapacity = state.lastFullChargeCapacity * 1000 / mV;
			state.designCapacityWarning = state.designCapacityWarning * 1000 / mV;
			state.designCapacityLow = state.designCapacityLow * 1000 / mV;
		}
	}

	if (designCapacity < state.lastFullChargeCapacity) {
		SYSLOG("binfo", "battery %d reports lower design capacity than maximum charged (%u/%u)", id,
			   designCapacity, state.lastFullChargeCapacity);
		if (state.lastFullChargeCapacity < ValueMax) {
			auto temp = designCapacity;
			designCapacity = state.lastFullChargeCapacity;
			state.lastFullChargeCapacity = temp;
		}
	}

	DBGLOG("binfo", "battery %d cycle count %d remaining capacity %ld", id, cycle, state.lastFullChargeCapacity);
}

void BatteryManager::checkDevices() {
	if (timerEventSource == nullptr) {
		DBGLOG("bmgr", "WTF timerEventSource is null");
		return;
	}
	timerEventSource->cancelTimeout();

	DBGLOG("bmgr", "checkDevices");

	bool batteriesConnected = false;
	bool batteriesConnection[BatteryManagerState::MaxBatteriesSupported] {};
	bool calculatedACAdapterConnection[BatteryManagerState::MaxBatteriesSupported] {};

	for (uint32_t i = 0; i < batteriesCount; i++)
		batteriesConnected |= batteriesConnection[i] = batteries[i].updateStaticStatus(&calculatedACAdapterConnection[i]);

	bool externalPowerConnected = false;
	for (uint32_t i = 0; i < adapterCount; i++)
		externalPowerConnected |= adapters[i].updateStatus();

	bool batteriesAreFull = false;
	if (batteriesConnected) {
		batteriesAreFull = true;
		for (uint32_t i = 0; i < batteriesCount; i++) {
			if (batteriesConnection[i]) {
				auto poll = atomic_load_explicit(&quickPoll, memory_order_acquire);
				batteriesAreFull = batteriesAreFull && batteries[i].updateRealTimeStatus(poll > 0);
			}
			if (!adapterCount && !externalPowerConnected)
				externalPowerConnected |= calculatedACAdapterConnection[i];
		}
	} else {
		// Safe to assume without batteries you need AC
		externalPowerConnected = true;
	}

	externalPowerNotify(externalPowerConnected);
	DBGLOG("bmgr", "status batteriesConnected %d externalPowerConnected %d batteriesAreFull %d", batteriesConnected, externalPowerConnected, batteriesAreFull);
	if (externalPowerConnected && batteriesAreFull) {
		DBGLOG("bmgr", "no poll");
		return;
	}

	if (decrementQuickPoll()) {
		DBGLOG("bmgr", "quick poll");
		timerEventSource->setTimeoutMS(ACPIBattery::QuickPollInterval);
	} else {
		DBGLOG("bmgr", "normal poll");
		timerEventSource->setTimeoutMS(ACPIBattery::NormalPollInterval);
	}
}

void BatteryManager::externalPowerNotify(bool status) {
	IOPMrootDomain *rd = IOACPIPlatformDevice::getPMRootDomain();
	rd->receivePowerNotification(kIOPMSetACAdaptorConnected | (kIOPMSetValue * status));
}

bool BatteryManager::decrementQuickPoll() {
	do {
		auto poll = atomic_load_explicit(&quickPoll, memory_order_acquire);
		if (poll <= 0)
			return false;

		if (atomic_compare_exchange_strong_explicit(&quickPoll, &poll, poll - 1, memory_order_release, memory_order_relaxed))
			return true;
	} while (1);
}

bool BatteryManager::findBatteries() {
	batteriesCount = 0;
	auto iterator = IORegistryIterator::iterateOver(gIOACPIPlane, kIORegistryIterateRecursively);
	auto pnp = OSString::withCString(ACPIBattery::PnpDeviceIdBattery);
	if (iterator) {
		while (auto entry = iterator->getNextObject()) {
			if (entry->compareName(pnp)) {
				auto device = OSDynamicCast(IOACPIPlatformDevice, entry);
				if (device) {
					DBGLOG("bmgr", "found ACPI PNP battery %s", safeString(entry->getName()));
					batteries[batteriesCount] = ACPIBattery(device, batteriesCount, stateLock, &state.btInfo[batteriesCount]);
					batteryNotifiers[batteriesCount] = device->registerInterest(gIOGeneralInterest, acpiNotification, this);
					if (!batteryNotifiers[batteriesCount]) {
						SYSLOG("bmgr", "battery find is unable to register interest for battery notifications");
						batteriesCount = 0;
						break;
					}

					batteriesCount++;
					if (batteriesCount >= BatteryManagerState::MaxBatteriesSupported)
						break;
				}
			}
		}
		iterator->release();
	} else {
		SYSLOG("bmgr", "battery find failed to iterate over acpi");
	}

	pnp->release();

	return batteriesCount > 0;
}

bool BatteryManager::findACAdapters() {
	adapterCount = 0;
	auto iterator = IORegistryIterator::iterateOver(gIOACPIPlane, kIORegistryIterateRecursively);
	auto pnp = OSString::withCString(ACPIACAdapter::PnpDeviceIdAcAdapter);
	if (iterator) {
		while (auto entry = iterator->getNextObject()) {
			if (entry->compareName(pnp)) {
				auto device = OSDynamicCast(IOACPIPlatformDevice, entry);
				if (device) {
					DBGLOG("bmgr", "found ACPI PNP adapter %s", safeString(entry->getName()));
					adapters[adapterCount] = ACPIACAdapter(device, adapterCount, stateLock, &state.acInfo[adapterCount]);
					adapterNotifiers[adapterCount] = device->registerInterest(gIOGeneralInterest, acpiNotification, this);
					if (!adapterNotifiers[adapterCount]) {
						SYSLOG("bmgr", "adapter find is unable to register interest for adapter notifications");
						adapterCount = 0;
						break;
					}

					adapterCount++;
					if (adapterCount >= BatteryManagerState::MaxAcAdaptersSupported)
						break;
				}
			}
		}
		iterator->release();
	} else {
		SYSLOG("bmgr", "adapter find failed to iterate over acpi");
	}

	pnp->release();

	return adapterCount > 0;
}

void BatteryManager::subscribe(PowerSourceInterestHandler h, void *t) {
	atomic_store_explicit(&handlerTarget, t, memory_order_release);
	atomic_store_explicit(&handler, h, memory_order_release);
}

IOReturn BatteryManager::acpiNotification(void *target, void *refCon, UInt32 messageType, IOService *provider, void *messageArgument, vm_size_t argSize) {
	if (messageType == kIOACPIMessageDeviceNotification) {
		DBGLOG("bmgr", "%s kIOACPIMessageDeviceNotification", safeString(provider->getName()));
		if (target == nullptr) {
			DBGLOG("bmgr", "target is null");
			return kIOReturnError;
		}
		
		auto self = OSDynamicCast(BatteryManager, reinterpret_cast<OSMetaClassBase*>(target));
		if (self == nullptr) {
			DBGLOG("bmgr", "target is not a BatteryManager");
			return kIOReturnError;
		}
		DBGLOG("bmgr", "%s received kIOACPIMessageDeviceNotification", safeString(provider->getName()));
		atomic_store_explicit(&self->quickPoll, ACPIBattery::QuickPollCount, memory_order_release);

		IOLockLock(self->mainLock);
		self->checkDevices();
		IOLockUnlock(self->mainLock);

		auto h = atomic_load_explicit(&self->handler, memory_order_acquire);
		if (h) h(atomic_load_explicit(&self->handlerTarget, memory_order_acquire));
	} else {
		DBGLOG("bmgr", "%s received %08X", safeString(provider->getName()), messageType);
	}
	return kIOReturnSuccess;
}

void BatteryManager::wake() {
	IOLockLock(mainLock);
	checkDevices();
	IOLockUnlock(mainLock);

	auto h = atomic_load_explicit(&handler, memory_order_acquire);
	if (h) h(atomic_load_explicit(&handlerTarget, memory_order_acquire));
}


bool BatteryManager::probe() {
	bool success = true;

	IOLockLock(mainLock);
	if (!workloop) {
		DBGLOG("bmgr", "probing battery manager");

		// ACPI presence is a requirement, wait for it.
		auto dict = IOService::nameMatching("AppleACPIPlatformExpert");
		if (!dict) {
			SYSLOG("bmgr", "failed to create acpi matching dictionary");
			return false;
		}

		auto acpi = IOService::waitForMatchingService(dict);
		dict->release();

		if (!acpi) {
			SYSLOG("bmgr", "failed to wait for acpi");
			return false;
		}

		acpi->release();

		workloop = IOWorkLoop::workLoop();
		timerEventSource = IOTimerEventSource::timerEventSource(this, [](OSObject *object, IOTimerEventSource *sender) {
			auto bm = OSDynamicCast(BatteryManager, object);
			if (bm) {
				IOLockLock(bm->mainLock);
				bm->checkDevices();
				IOLockUnlock(bm->mainLock);
			}
		});
		if (!timerEventSource || !workloop) {
			SYSLOG("bmgr", "failed to create workloop or timer event source");
			success = false;
		}

		if (success && workloop->addEventSource(timerEventSource) != kIOReturnSuccess) {
			SYSLOG("bmgr", "failed to add timer event source");
			success = false;
		}
		
		// timerEventSource must exist before adding ACPI notification handler
		if (success && (!findBatteries() || !findACAdapters())) {
			//FIXME: we should work with battery, but without adapter,
			// but we should guarantee that we find adapter if it exists
			SYSLOG("bmgr", "failed to find batteries or adapters!");
			success = false;
		}
		
		DBGLOG("bmgr", "found %u AC adapters and %u batteries", adapterCount, batteriesCount);

		if (!success) {
			OSSafeReleaseNULL(workloop);
			OSSafeReleaseNULL(timerEventSource);
		}
	}
	IOLockUnlock(mainLock);

	return success;
}

void BatteryManager::start() {
	IOLockLock(mainLock);
	if (!initialCheckDevices) {
		checkDevices();
		initialCheckDevices = true;
	}
	IOLockUnlock(mainLock);
}

bool BatteryManager::batteriesConnected() {
	for (uint32_t i = 0; i < batteriesCount; i++)
		if (state.btInfo[i].connected)
			return true;
	return false;
}

bool BatteryManager::adaptersConnected() {
	if (adapterCount) {
		for (uint32_t i = 0; i < adapterCount; i++)
			if (state.acInfo[i].connected)
				return true;
	}
	else {
		for (uint32_t i = 0; i < batteriesCount; i++)
			if (state.btInfo[i].state.calculatedACAdapterConnected)
				return true;
	}
	return false;
}

bool BatteryManager::batteriesAreFull() {
	// I am not fully convinced we should assume that batteries are full when there are none, but so be it.
	for (uint32_t i = 0; i < batteriesCount; i++)
		if (state.btInfo[i].connected && (state.btInfo[i].state.state & ACPIBattery::BSTStateMask) != ACPIBattery::BSTFullyCharged)
			return false;
	return true;
}

bool BatteryManager::externalPowerConnected() {
	// Firstly try real adapters
	for (uint32_t i = 0; i < adapterCount; i++)
		if (state.acInfo[i].connected)
			return true;

	// Then try calculated adapters
	bool hasBateries = false;
	for (uint32_t i = 0; i < batteriesCount; i++) {
		if (state.btInfo[i].connected) {
			// Ignore calculatedACAdapterConnected when real adapters exist!
			if (adapterCount == 0 && state.btInfo[i].state.calculatedACAdapterConnected)
				return true;
			hasBateries = true;
		}
	}

	// Safe to assume without batteries you need AC
	return hasBateries == false;
}

uint16_t BatteryManager::calculateBatteryStatus(size_t index) {
	return batteries[index].calculateBatteryStatus();
}

void BatteryManager::createShared() {
	if (instance)
		PANIC("bmgr", "attempted to allocate battery manager again");
	instance = new BatteryManager;
	if (!instance)
		PANIC("bmgr", "failed to allocate battery manager");
	instance->mainLock = IOLockAlloc();
	if (!instance->mainLock)
		PANIC("bmgr", "failed to allocate main battery manager lock");
	instance->stateLock = IOSimpleLockAlloc();
	if (!instance->stateLock)
		PANIC("bmgr", "failed to allocate state battery manager lock");

	atomic_init(&instance->quickPoll, 0);
	atomic_init(&instance->handlerTarget, nullptr);
	atomic_init(&instance->handler, nullptr);
}

