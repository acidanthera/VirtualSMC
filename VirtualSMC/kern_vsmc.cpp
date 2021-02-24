//
//  kern_vsmc.cpp
//  VirtualSMC
//
//  Copyright © 2017 vit9696. All rights reserved.
//

#include <IOKit/IOService.h>
#include <Headers/kern_efi.hpp>
#include <Headers/kern_devinfo.hpp>
#include <Headers/kern_iokit.hpp>
#include <Headers/kern_crypto.hpp>
#include <Headers/kern_version.hpp>
#include <Headers/plugin_start.hpp>
#include <IOKit/pwr_mgt/IOPM.h>
#include <IOKit/IODeviceTreeSupport.h>
#include <VirtualSMCSDK/AppleSmcBridge.hpp>

#include "kern_vsmc.hpp"
#include "kern_prov.hpp"
#include "kern_efiend.hpp"

OSDefineMetaClassAndStructors(VirtualSMC, IOACPIPlatformDevice)

VirtualSMC *VirtualSMC::instance;
_Atomic(bool) VirtualSMC::mmioReady;
_Atomic(bool) VirtualSMC::servicingReady;

IOService *VirtualSMC::probe(IOService *provider, SInt32 *score) {
	auto service = IOService::probe(provider, score);

	if (service && !ADDPR(startSuccess)) {
		DBGLOG("vsmc", "probing with lilu offline");
		auto prov = VirtualSMCProvider::getInstance();
		if (prov)
			return service;
	}

	return ADDPR(startSuccess) ? service : nullptr;
}

bool VirtualSMC::start(IOService *provider) {
	// Implementing multiple SMC devices results in infinite recursion at least on 10.12
	// This is because AppleSMC.kext unconditionally updates PE_halt_restart pointer with its own function
	// and stores the original function in its start method and has only one global `this` pointer.
	if (devicesPresent(provider)) {
		SYSLOG("vsmc", "disabling itself due to multiple devices present");
		return false;
	}

	//FIXME: This is essential because we need to initialise _platform, however it is in a wrong place
	if (!IOACPIPlatformDevice::init(provider, nullptr, getPropertyTable())) {
		SYSLOG("vsmc", "failed to initalise the parent");
		return false;
	}
	
	if (!IOACPIPlatformDevice::start(provider)) {
		SYSLOG("vsmc", "failed to start the parent");
		return false;
	}

	// We do not check for disabling boot arguments here, because we are meant to work unless
	// some other SMC implementation is available. Lilu handles limitations for us, and -vsmcoff
	// is supposed to only switch MMIO mode off.

	watchDogWorkLoop = IOWorkLoop::workLoop();
	if (watchDogWorkLoop) {
		watchDogTimer = IOTimerEventSource::timerEventSource(this, watchDogAction);
		if (watchDogTimer)
			watchDogWorkLoop->addEventSource(watchDogTimer);
		else
			SYSLOG("vsmc", "watchdog timer allocation failure");
	} else {
		SYSLOG("vsmc", "watchdog loop allocation failure");
	}

	int computerModel = BaseDeviceInfo::get().modelType;
	if (computerModel == WIOKit::ComputerModel::ComputerAny) {
		DBGLOG("vsmc", "failed to determine laptop or desktop model");
		computerModel = WIOKit::ComputerModel::ComputerInvalid;
	}

	auto boardIdentifier = BaseDeviceInfo::get().boardIdentifier;

	SMCInfo deviceInfo {};

	if (obtainBooterModelInfo(deviceInfo))
		DBGLOG("vsmc", "obtained device model info from the bootloader");

	auto modelInfo = OSDynamicCast(OSDictionary, getProperty("ModelInfo"));
	auto overrModelInfo = OSDynamicCast(OSDictionary, getProperty("OverrideModelInfo"));
	if (!modelInfo || !obtainModelInfo(deviceInfo, boardIdentifier, modelInfo, overrModelInfo)) {
		SYSLOG("vsmc", "failed to get model info");
		return false;
	}

	auto hardwareModel = reinterpret_cast<char *>(deviceInfo.getBuffer(SMCInfo::Buffer::HardwareModel));
	setProperty("compatible", hardwareModel, static_cast<uint32_t>(strlen(hardwareModel)+1));

	keystore = new VirtualSMCKeystore;
	if (keystore == nullptr) {
		SYSLOG("vsmc", "keystore allocation failure");
		return false;
	}

	auto store = OSDynamicCast(OSDictionary, getProperty("Keystore"));
	auto userStore = OSDynamicCast(OSDictionary, getProperty("UserKeystore"));
	if (!keystore->init(store, userStore, deviceInfo, boardIdentifier, computerModel, VirtualSMCProvider::getFirmwareBackendStatus())) {
		SYSLOG("vsmc", "keystore initialisation failure");
		delete keystore;
		return false;
	}

	PMinit();
	provider->joinPMtree(this);
	registerPowerDriver(this, powerStates, arrsize(powerStates));

	// We do not need them anymore
	removeProperty("CFBundleIdentifier");
	// Do not remove IOClass and IOMatchCategory as it may cause double startup to happen on 10.6.
	removeProperty("IOName");
	removeProperty("IOProbeScore");
	removeProperty("IOProviderClass");
	removeProperty("ModelInfo");
	removeProperty("OverrideModelInfo");
	removeProperty("Keystore");
	removeProperty("UserKeystore");

	setProperty("VersionInfo", kextVersion);

	pmio = new SMCProtocolPMIO;
	if (deviceInfo.getGeneration() >= SMCInfo::Generation::V2)
		mmio = new SMCProtocolMMIO;
	
	if (!pmio || (!mmio && deviceInfo.getGeneration() >= SMCInfo::Generation::V2)) {
		SYSLOG("vsmc", "protocol allocation failure");
		delete keystore;
		delete pmio;
		delete mmio;
		return false;
	}

	interruptsLock = IOSimpleLockAlloc();
	if (!interruptsLock)
		PANIC("vsmc", "failed to allocate interrupt lock");

	// Reserve interrupt slots
	if (!storedInterrupts.reserve(MaxActiveInterrupts) ||
		!registeredInterrupts.reserve(MaxActiveInterrupts)) {
		PANIC("vsmc", "failed to reserve interrupt slots");
	}

	// Service initialisation may be delayed to allow trap hook to be initialised
	instance = this;

	// Report servicing as now ready.
	atomic_store_explicit(&servicingReady, true, memory_order_release);

	// For first generation the initialisation is handled entirely in a private manner.
	// For second generation perform initialisation when MMIO landed earlier and requested
	// servicing to perform the initialisation.
	bool shouldServicingInit = deviceInfo.getGeneration() == SMCInfo::Generation::V1 ||
		atomic_load_explicit(&mmioReady, memory_order_acquire);

	if (shouldServicingInit) {
		registerService();
		publishResource(VirtualSMCAPI::SubmitPlugin);
		// Retain ourselves to avoid crashes if lilu is missing (only valid for first gen).
		ADDPR(startSuccess) = true;
	}

	DBGLOG("vsmc", "starting up vsmc service (%d) " PRIKADDR, shouldServicingInit, CASTKADDR(this));

	return true;
}

void VirtualSMC::watchDogAction(OSObject *owner, IOTimerEventSource *sender) {
	auto vsmc = OSDynamicCast(VirtualSMC, owner);
	if (vsmc) {
		auto job = vsmc->watchDogJob;

		vsmc->watchDogJob = WatchDogDoNothing;
		DBGLOG("vsmc", "watchdog action %u arrived", job);

		switch (job) {
			case WatchDogForceRestart: {
				// Do not risk locking here, it is pointless anyway.
				auto *rt = EfiRuntimeServices::get();
				if (rt) rt->resetSystem(EfiResetWarm);
				PANIC("vsmc", "watchdog force restart!");
				break;
			}
			case WatchDogShutdownToS5: {
				// Do not risk locking here, it is pointless anyway.
				auto *rt = EfiRuntimeServices::get();
				if (rt) rt->resetSystem(EfiResetShutdown);
				PANIC("vsmc", "watchdog force shutdown (S5)!");
				break;
			}
			case WatchDogForceStartup:
				SYSLOG("vsmc", "watchdog startup request!");
				break;
			case WatchDogReserved:
				SYSLOG("vsmc", "watchdog reserved request!");
				break;
			case WatchDogShutdownToUnk: {
				// We do not know what exactly this is, let's cold reboot for now.
				// Do not risk locking here, it is pointless anyway.
				auto *rt = EfiRuntimeServices::get();
				if (rt) rt->resetSystem(EfiResetCold);
				PANIC("vsmc", "watchdog force shutdown (Unk)!");
				break;
			}
		}
	} else {
		SYSLOG("vsmc", "watchdog action conversion failure");
	}
}

bool VirtualSMC::obtainBooterModelInfo(SMCInfo &deviceInfo) {
	if (forcedGeneration() != SMCInfo::Generation::Unspecified) {
		SYSLOG("vsmc", "ignoring booter rev info due to forced gen %d", forcedGeneration());
		return false;
	}

	auto platform = IORegistryEntry::fromPath("/efi/platform", gIODTPlane);
	if (platform) {
		auto rev = OSDynamicCast(OSData, platform->getProperty("REV"));
		if (rev && rev->getLength() == deviceInfo.getBufferSize(SMCInfo::Buffer::RevMain)) {
			lilu_os_memcpy(deviceInfo.getBuffer(SMCInfo::Buffer::RevMain), rev->getBytesNoCopy(), rev->getLength());
			lilu_os_memcpy(deviceInfo.getBuffer(SMCInfo::Buffer::RevFlasherBase), rev->getBytesNoCopy(), rev->getLength());
			lilu_os_memcpy(deviceInfo.getBuffer(SMCInfo::Buffer::RevFlasherUpdate), rev->getBytesNoCopy(), rev->getLength());
			platform->removeProperty("REV");
		}

		auto rbr = OSDynamicCast(OSData, platform->getProperty("RBr"));
		if (rbr && rbr->getLength() == deviceInfo.getBufferSize(SMCInfo::Buffer::Branch)) {
			lilu_os_memcpy(deviceInfo.getBuffer(SMCInfo::Buffer::Branch), rbr->getBytesNoCopy(), rbr->getLength());
			platform->removeProperty("RBr");
		}

		auto rplt = OSDynamicCast(OSData, platform->getProperty("RPlt"));
		if (rplt && rplt->getLength() == deviceInfo.getBufferSize(SMCInfo::Buffer::Platform)) {
			lilu_os_memcpy(deviceInfo.getBuffer(SMCInfo::Buffer::Platform), rplt->getBytesNoCopy(), rplt->getLength());
			platform->removeProperty("RPlt");
		}

		platform->release();
		return rev || rbr || rplt;
	}

	SYSLOG("vsmc", "missing efi device");
	return false;
}

bool VirtualSMC::obtainModelInfo(SMCInfo &deviceInfo, const char *boardIdentifier, const OSDictionary *main, const OSDictionary *user) {
	auto doObtain = [this, main, user, &deviceInfo](const char *name, bool overr) {
		auto src = overr ? user : main;
		if (src) {
			auto dict = OSDynamicCast(OSDictionary, src->getObject(name));
			if (dict) {
				auto updateBuffer = [&](SMCInfo::Buffer id) {
					auto buf = deviceInfo.getBuffer(id);
					if (buf[0])
						return;
					
					const char *prop = nullptr;
					if (id == SMCInfo::Buffer::RevMain)
						prop = "rev";
					else if (id == SMCInfo::Buffer::RevFlasherBase)
						prop = "revfb";
					else if (id == SMCInfo::Buffer::RevFlasherUpdate)
						prop = "revfu";
					else if (id == SMCInfo::Buffer::Branch)
						prop = "branch";
					else if (id == SMCInfo::Buffer::Platform)
						prop = "platform";
					else if (id == SMCInfo::Buffer::HardwareModel)
						prop = "hwname";
					else
						return;
					
					auto size = deviceInfo.getBufferSize(id);
					auto contents = OSDynamicCast(OSData, dict->getObject(prop));
					if (contents) {
						auto len = contents->getLength();
						if (id == SMCInfo::Buffer::HardwareModel) {
							auto hardwareModel = reinterpret_cast<char *>(buf);
							strlcpy(hardwareModel, static_cast<const char *>(contents->getBytesNoCopy()), len < size ? len+1 : size);
							DBGLOG("vsmc", "detected model %s from %s (override %d)", hardwareModel, name, overr);
						} else if (len == size) {
							lilu_os_memcpy(buf, contents->getBytesNoCopy(), size);
							DBGLOG("vsmc", "getting buffer %s from %s (override %d)", prop, name, overr);
						}
					}
				};
				
				updateBuffer(SMCInfo::Buffer::RevMain);
				updateBuffer(SMCInfo::Buffer::RevFlasherBase);
				updateBuffer(SMCInfo::Buffer::RevFlasherUpdate);
				updateBuffer(SMCInfo::Buffer::Branch);
				updateBuffer(SMCInfo::Buffer::Platform);
				updateBuffer(SMCInfo::Buffer::HardwareModel);
			}
		}
	};
	
	auto gen = forcedGeneration();
	if (gen == SMCInfo::Generation::Unspecified && boardIdentifier[0] != '\0') {
		doObtain(boardIdentifier, true);
		doObtain(boardIdentifier, false);
	}

	// Check forced and V1 prior to checking V3, as it may be default
	const char *generic = "GenericV2";
	if (gen != SMCInfo::Generation::Unspecified) {
		if (gen == SMCInfo::Generation::V1)
			generic = "GenericV1";
		else if (gen == SMCInfo::Generation::V3)
			generic = "GenericV3";
	} else if (deviceInfo.getGeneration() == SMCInfo::Generation::V1) {
		generic = "GenericV1";
	} else if (deviceInfo.getGeneration() == SMCInfo::Generation::V3) {
		generic = "GenericV3";
	}

	DBGLOG("vsmc", "gen %d (%d) board %s - %s", gen, deviceInfo.getGeneration(),
		   boardIdentifier, generic);

	doObtain(generic, true);
	doObtain(generic, false);
	
	if (!deviceInfo.isValid()) {
		SYSLOG("vsmc", "unable to get necessary smc model information");
		return false;
	}

	// Obtain extra model info:
	auto platform = IORegistryEntry::fromPath("/", gIODTPlane);
	if (platform) {
		auto serial = OSDynamicCast(OSString, platform->getProperty("IOPlatformSerialNumber"));
		auto cstr = serial ? serial->getCStringNoCopy() : nullptr;
		if (cstr) {
			auto dserial = deviceInfo.getBuffer(SMCInfo::Buffer::Serial);
			auto sserialSize = strlen(cstr);
			auto dserialSize = deviceInfo.getBufferSize(SMCInfo::Buffer::Serial);
			lilu_os_memcpy(dserial, cstr, sserialSize > dserialSize ? dserialSize : sserialSize);
		}
		platform->release();
	}

	EfiBackend::readSerials(deviceInfo.getBuffer(SMCInfo::Buffer::MotherboardSerial), deviceInfo.getBufferSize(SMCInfo::Buffer::MotherboardSerial),
							deviceInfo.getBuffer(SMCInfo::Buffer::MacAddress), deviceInfo.getBufferSize(SMCInfo::Buffer::MacAddress));

	return true;
}

void VirtualSMC::stop(IOService * provider) {
	DBGLOG("vsmc", "stopping vsmc service " PRIKADDR, CASTKADDR(this));
	PMstop();
	IOACPIPlatformDevice::stop(this);
}

bool VirtualSMC::devicesPresent(IOService *provider) {
	// The use of getMatchingServices appears to be no longer possible due to IOService changes in 10.13.

	bool preferHardwareSMC = checkKernelArgument("-vsmccomp");
	bool activeDevicesFound = false;

	size_t smcDeviceNum = 0;
	auto iterator = provider->getChildIterator(gIOServicePlane);
	if (iterator) {
		IORegistryEntry *obj = nullptr;
		while ((obj = OSDynamicCast(IORegistryEntry, iterator->getNextObject())) != nullptr) {
			auto name = OSDynamicCast(OSData, obj->getProperty("name"));
			if (name && name->isEqualTo(VirtualSMCAPI::ServiceName, static_cast<uint32_t>(strlen(VirtualSMCAPI::ServiceName)+1))) {
				DBGLOG("vsmc", "found %s compatible service %s", VirtualSMCAPI::ServiceName, obj->getName());
				auto cls = OSDynamicCast(OSString, obj->getProperty("IOClass"));
				if (cls && cls->isEqualTo("VirtualSMC")) {
					DBGLOG("vsmc", "ignoring self by class name");
					continue;
				} else if (!preferHardwareSMC) {
					auto smc = obj->childFromPath("AppleSMC", gIOServicePlane);
					if (smc) {
						DBGLOG("vsmc", "found active smc device");
						smc->release();
						activeDevicesFound = true;
					} else {
						// Disable inactive existing devices
						DBGLOG("vsmc", "disabling another smc device");
						obj->setProperty("name", "OFF0001");
					}
				}
				smcDeviceNum++;
			}
		}
		iterator->release();
	}

	DBGLOG("vsmc", "found %lu smc devices", smcDeviceNum);
	return (preferHardwareSMC || activeDevicesFound) && smcDeviceNum > 0;
}

SMCInfo::Generation VirtualSMC::forcedGeneration() {
	// I do not think we support anything below 10.8 but just in case...
	if (getKernelVersion() < KernelVersion::MountainLion) {
		DBGLOG("vsmc", "mmio protocol is supported on 10.8 and newer");
		return SMCInfo::Generation::V1;
	}

	if (!ADDPR(startSuccess))
		return SMCInfo::Generation::V1;

	int32_t gen;
	if (PE_parse_boot_argn("vsmcgen", &gen, sizeof(int32_t))) {
		if (gen == 3)
			return SMCInfo::Generation::V3;
		else if (gen == 1)
			return SMCInfo::Generation::V1;
		else
			return SMCInfo::Generation::V2;
	}

	return SMCInfo::Generation::Unspecified;
}

void VirtualSMC::setInterrupts(bool enable) {
	if (instance) {
		IOSimpleLockLock(instance->interruptsLock);
		instance->interruptsEnabled = enable;
		IOSimpleLockUnlock(instance->interruptsLock);
	}
}

bool VirtualSMC::postInterrupt(SMC_EVENT_CODE code, const void *data, uint32_t dataSize) {
	if (instance) {
		IOSimpleLockLock(instance->interruptsLock);
		if (instance->interruptsEnabled && code > 0) {
			if (dataSize > sizeof(StoredInterrupt::data)) {
				IOSimpleLockUnlock(instance->interruptsLock);
				SYSLOG("vsmc", "postInterrupt dataSize overflow %u", dataSize);
				return false;
			}

			for (size_t i = 0; i < instance->storedInterrupts.size(); i++) {
				auto &si = instance->storedInterrupts[i];
				if (si.code == code) {
					si.size = dataSize;
					if (dataSize > 0)
						lilu_os_memcpy(si.data, data, dataSize);

					// It should be already caused when it was added
					IOSimpleLockUnlock(instance->interruptsLock);
					return true;
				}
			}

			if (instance->storedInterrupts.size() == MaxActiveInterrupts) {
				IOSimpleLockUnlock(instance->interruptsLock);
				SYSLOG("vsmc", "postInterrupt reserve overflow");
				return false;
			}

			StoredInterrupt si {code, dataSize};
			if (dataSize > 0)
				lilu_os_memcpy(si.data, data, dataSize);
			if (instance->storedInterrupts.push_back(si)) {
				// Cause interrupt immediately if it is the first one
				bool oneIntr = instance->storedInterrupts.size() == 1;
				IOSimpleLockUnlock(instance->interruptsLock);
				if (oneIntr) {
					DBGLOG("vsmc", "causing interrupt %02X with size %u", code, dataSize);
					instance->causeInterrupt(EventInterruptNo);
				}
				return true;
			}

			IOSimpleLockUnlock(instance->interruptsLock);
			SYSLOG("vsmc", "unable to store interrupt");
		} else {
			IOSimpleLockUnlock(instance->interruptsLock);
		}
	}
	
	return false;
}

SMC_EVENT_CODE VirtualSMC::getInterrupt() {
	if (instance) {
		IOSimpleLockLock(instance->interruptsLock);

		if (instance->storedInterrupts.size() > 0) {
			auto &si = instance->storedInterrupts[0];

			//DBGLOG("vsmc", "setting interrupt %02X with size %u", si.code, si.size);

			if (instance->mmio)
				instance->mmio->setInterrupt(si.code, si.data, si.size);
			else
				instance->pmio->setInterrupt(si.code, si.data, si.size);

			instance->currentEventCode = si.code;
			instance->storedInterrupts.erase(0, false);

			auto code = instance->currentEventCode;
			auto moreIntrs = instance->storedInterrupts.size() > 0;
			IOSimpleLockUnlock(instance->interruptsLock);

			// Continue processing interrupts that are left
			if (moreIntrs)
				instance->causeInterrupt(EventInterruptNo);

			return code;
		} else {
			IOSimpleLockUnlock(instance->interruptsLock);
		}
	}
	
	DBGLOG("vsmc", "no interrupt to report");
	return 0;
}

void VirtualSMC::postWatchDogJob(uint8_t code, uint64_t timeout, bool last) {
	if (instance && instance->watchDogTimer && instance->watchDogAcceptJobs) {
		instance->watchDogTimer->cancelTimeout();
		if (code == WatchDogDoNothing) {
			instance->watchDogJob = WatchDogDoNothing;
		} else {
			DBGLOG("vsmc", "accepted watchdog job %02X with timer %lld", code, timeout);
			instance->watchDogJob = code;
			instance->watchDogTimer->setTimeoutMS(static_cast<uint32_t>(timeout));
			instance->watchDogTimer->enable();
		}

		if (last)
			instance->watchDogAcceptJobs = false;
	}
}

IOMemoryMap *VirtualSMC::mapDeviceMemoryWithIndex(unsigned int index, IOOptionBits options) {
	DBGLOG("vsmc", "mapDeviceMemoryWithIndex (%u, %u)", index, options);
	return VirtualSMCProvider::getMapping(index, options);
}

bool VirtualSMC::ioVerify(UInt16 &offset, IOMemoryMap *map) {
	if (!pmioBase) {
		pmioMap = VirtualSMCProvider::getMapping(VirtualSMCProvider::AppleSMCBufferPMIO);
		pmioBase = pmioMap->getPhysicalAddress();
	}

	// Offsets must be between 0 and AppleSMCPMIOWindowSize
	if ((!map || map == pmioMap) && offset >= static_cast<UInt16>(pmioBase)) {
		offset -= static_cast<UInt16>(pmioBase);
		if (offset < SMCProtocolPMIO::WindowSize)
			return true;
	}

	SYSLOG("vsmc", "invalid io at %04X base %04X to null map %d / pmio map %d", offset,
		   static_cast<UInt16>(pmioBase), map == nullptr, map == pmioMap);
	return false;
}

void VirtualSMC::ioWrite32(UInt16 offset, UInt32 value, IOMemoryMap *map) {
	PANIC("vsmc", "unsupported write32 at %04X with %08X", offset, value);
}

UInt32 VirtualSMC::ioRead32(UInt16 offset, IOMemoryMap *map) {
	PANIC("vsmc", "unsupported read32 at %04X", offset);
	return 0;
}

void VirtualSMC::ioWrite16(UInt16 offset, UInt16 value, IOMemoryMap *map) {
	PANIC("vsmc", "unsupported write16 at %04X with %04X", offset, value);
}


UInt16 VirtualSMC::ioRead16(UInt16 offset, IOMemoryMap *map) {
	PANIC("vsmc", "unsupported read16 at %04X", offset);
	return 0;
}

void VirtualSMC::ioWrite8(UInt16 offset, UInt8 value, IOMemoryMap *map) {
	if (ioVerify(offset, map)) {
		switch (offset) {
			case SMC_PORT_OFFSET_DATA:
				pmio->writeData(value);
				return;
			case SMC_PORT_OFFSET_COMMAND:
				pmio->writeCommand(value);
				return;
			default:
				PANIC("vsmc", "write to unsupported port %02X", offset);
		}
	}
}

UInt8 VirtualSMC::ioRead8(UInt16 offset, IOMemoryMap *map) {
	if (ioVerify(offset, map)) {
		switch (offset) {
			case SMC_PORT_OFFSET_DATA:
				return pmio->readData();
			case SMC_PORT_OFFSET_STATUS:
				return pmio->readStatus();
			case SMC_PORT_OFFSET_RESULT:
				return pmio->readResult();
			case SMC_PORT_OFFSET_EVENT:
				return currentEventCode;
			default:
				PANIC("vsmc", "read to unsupported port %02X", offset);
		}
	}
	
	return 0;
}

IOReturn VirtualSMC::registerInterrupt(int source, OSObject *target, IOInterruptAction handler, void *refCon) {
	DBGLOG("vsmc", "registerInterrupt %d", source);
	
	bool valid = false;
	for (size_t i = 0; i < arrsize(ValidInterruptNo); i++) {
		if (ValidInterruptNo[i] == source) {
			valid = true;
			break;
		}
	}

	if (valid) {
		for (size_t i = 0; i < registeredInterrupts.size(); i++) {
			if (registeredInterrupts[i].source == source) {
				return kIOReturnNoResources;
			}
		}

		if (registeredInterrupts.size() == MaxActiveInterrupts) {
			SYSLOG("vsmc", "registerInterrupt reserve overflow");
			return kIOReturnNoMemory;
		}

		if (registeredInterrupts.push_back({source, target, handler, refCon})) {
			DBGLOG("vsmc", "registered interrupt %d", source);
			return kIOReturnSuccess;
		} else {
			SYSLOG("vsmc", "failed to register interrupt %d", source);
		}
	}

	return kIOReturnNoInterrupt;
}

IOReturn VirtualSMC::unregisterInterrupt(int source) {
	DBGLOG("vsmc", "unregisterInterrupt %d", source);
	
	for (size_t i = 0; i < registeredInterrupts.size(); i++) {
		if (registeredInterrupts[i].source == source) {
			registeredInterrupts.erase(i, false);
			DBGLOG("vsmc", "unregistered interrupt %d", source);
			return kIOReturnSuccess;
		}
	}
	
	return kIOReturnNoInterrupt;
}

IOReturn VirtualSMC::getInterruptType(int source, int *interruptType) {
	DBGLOG("vsmc", "getInterruptType %d", source);
	
	for (size_t i = 0; i < arrsize(ValidInterruptNo); i++) {
		if (ValidInterruptNo[i] == source) {
			*interruptType = InterruptType;
			return kIOReturnSuccess;
		}
	}
	
	return kIOReturnNoInterrupt;
}

IOReturn VirtualSMC::enableInterrupt(int source) {
	DBGLOG("vsmc", "enableInterrupt %d", source);
	
	for (size_t i = 0; i < registeredInterrupts.size(); i++) {
		if (registeredInterrupts[i].source == source) {
			registeredInterrupts[i].enabled = true;
			return kIOReturnSuccess;
		}
	}
	
	return kIOReturnNoInterrupt;
}

IOReturn VirtualSMC::disableInterrupt(int source) {
	DBGLOG("vsmc", "disableInterrupt %d", source);
	
	for (size_t i = 0; i < registeredInterrupts.size(); i++) {
		if (registeredInterrupts[i].source == source) {
			registeredInterrupts[i].enabled = false;
			return kIOReturnSuccess;
		}
	}
	
	return kIOReturnNoInterrupt;
}

IOReturn VirtualSMC::causeInterrupt(int source) {
	DBGLOG("vsmc", "causeInterrupt %d", source);
	
	for (size_t i = 0; i < registeredInterrupts.size(); i++) {
		if (source == registeredInterrupts[i].source) {
			if (registeredInterrupts[i].enabled)
				registeredInterrupts[i].handler(registeredInterrupts[i].target, registeredInterrupts[i].refCon, this, registeredInterrupts[i].source);
			return kIOReturnSuccess;
		}
	}

	
	return kIOReturnNoInterrupt;
}

IOReturn VirtualSMC::setPowerState(unsigned long state, IOService *whatDevice){
	DBGLOG("vsmc", "changing power state to %lu", state);

	if (state == PowerStateOff)
		keystore->handlePowerOff();
	else if (state == PowerStateOn)
		keystore->handlePowerOn();

	return kIOPMAckImplied;
}

IOReturn VirtualSMC::callPlatformFunction(const OSSymbol *functionName, bool, void *param1, void *param2, void *, void *) {
	if (functionName && param1 && param2 && functionName->isEqualTo(VirtualSMCAPI::SubmitPlugin)) {
		DBGLOG("vsmc", "received plugin submission");
		// Retain a plugin just in case the implementation allows unloading...
		static_cast<IOService *>(param1)->retain();
		return keystore->loadPlugin(static_cast<VirtualSMCAPI::Plugin *>(param2));
	}

	return kIOReturnUnsupported;
}
