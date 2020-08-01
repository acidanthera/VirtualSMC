//
//  kern_vsmc.hpp
//  VirtualSMC
//
//  Copyright © 2017 vit9696. All rights reserved.
//

#ifndef kern_vsmc_hpp
#define kern_vsmc_hpp

#include "kern_util.hpp"
#include <Library/LegacyIOService.h>
#include <IOKit/IOTimerEventSource.h>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
#include <IOKit/acpi/IOACPIPlatformDevice.h>
#pragma clang diagnostic pop

#include <VirtualSMCSDK/kern_vsmcapi.hpp>
#include <VirtualSMCSDK/kern_smcinfo.hpp>

#include "kern_pmio.hpp"
#include "kern_mmio.hpp"
#include "kern_keystore.hpp"
#include "kern_intrs.hpp"

class EXPORT VirtualSMC : public IOACPIPlatformDevice {
	OSDeclareDefaultStructors(VirtualSMC)

	/**
	 *  The actual interrupt type does not matter, but the original SMC appears to have used edge
	 */
	static constexpr int InterruptType {kIOInterruptTypeEdge};

	/**
	 *  Event interrupt number
	 */
	static constexpr int EventInterruptNo {0};

	/**
	 *  Currently SMC seems to only have EventInterruptNo
	 */
	int ValidInterruptNo[1] {EventInterruptNo};

	/**
	 *  Interrupts may happen during hibernation, so we are not allowed to alloc.
	 *  For this reason we reserve the necessary amount of memory at kext start.
	 *  Affects registeredInterrupts and storedInterrupts.
	 */
	static constexpr size_t MaxActiveInterrupts {16};

	/**
	 *  Maximum interrupt data size used instead of pool allocation.
	 */
	static constexpr size_t MaxInterruptDataSize {SMC_MAX_LOG_SIZE};

	/**
	 *  Registered interrupts by registerInterrupt function
	 */
	evector<RegisteredInterrupt&> registeredInterrupts;

	/**
	 *  Stored interrupts for the delivery (event queue)
	 */
	evector<StoredInterrupt&> storedInterrupts;

	/**
	 *  Interrupt status set by enableInterrupt/disableInterrupt functions
	 */
	bool interruptsEnabled {false};

	/**
	 *  Interrupt access lock
	 */
	IOSimpleLock *interruptsLock {nullptr};

	/**
	 *  Current pending interrupt code
	 */
	SMC_EVENT_CODE currentEventCode {};

	/**
	 *  Global instance set after the initialisation for callback access
	 */
	static VirtualSMC *instance;

	/**
	 *  Keystore instance
	 */
	VirtualSMCKeystore *keystore {nullptr};

	/**
	 *  PMIO protocol implementation
	 */
	SMCProtocolPMIO *pmio {nullptr};

	/**
	 *  MMIO protocol implementation (set in v2 mode only)
	 */
	SMCProtocolMMIO *mmio {nullptr};

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

	/**
	 *  Software implementation for watchdog timer
	 */
	IOTimerEventSource *watchDogTimer {nullptr};

	/**
	 *  Set to false once the job is the last one (e.g. disable watchdog)
	 */
	bool watchDogAcceptJobs {true};

	/**
	 * Flag to mark MMIO support as ready.
	 * This is set when MMIO provider seems AppleSMC kexts and reports ready to handle requests.
	 */
	static _Atomic(bool) mmioReady;

	/**
	 * Flag to mark service support as ready
	 */
	static _Atomic(bool) servicingReady;

	/**
	 *  Software implementation for watchdog work loop
	 */
	IOWorkLoop *watchDogWorkLoop {nullptr};

	/**
	 *  Watchdog current job
	 */
	uint8_t watchDogJob {WatchDogDoNothing};

	/**
	 *  Watchdog timer action handler
	 *
	 *  @param owner   VirtualSMC instance
	 *  @param sender  watchDogTimer pointer
	 */
	static void watchDogAction(OSObject *owner, IOTimerEventSource *sender);

	/**
	 *  Cached value of AppleSMCBufferPMIO mapping
	 */
	IOMemoryMap *pmioMap {nullptr};

	/**
	 *  Cached value of AppleSMCBufferPMIO mapping physical address
	 */
	IOPhysicalAddress pmioBase {0};

	/**
	 *  Obtains device model information from the bootloader if any
	 *
	 *  @param device  device information to be filled
	 *
	 *  @return true on success
	 */
	bool obtainBooterModelInfo(SMCInfo &deviceInfo);

	/**
	 *  Obtains device model information
	 *
	 *  @param deviceInfo  device information to be filled
	 *  @param boardID     board-id for the mac model
	 *  @param main        main model information provider
	 *  @param user        user model override information provider
	 *
	 *  Model Information Provider is a dictionary of Model Information Entries.
	 *  The result of parsing all model information must be 4 properties: hwname, rev, revfb, revfu.
	 *  Which represent hardware codename, and 3 revisions: main revision, flasher base revision,
	 *  and flasher update revision. Revision major is represented in deviceGeneration.
	 *
	 *  Provider dictionaries range from highest to lowest priority by their key names.
	 *  If two key names collide the ones with higher priority are used.
	 *  User provider has higher priority than main provider.
	 *
	 *  Property         Definition                 Explanation
	 *  -------------------------------------------------------------------------------------------
	 *  Mac-XXXXXXXXX    XXXXXXXXX is board-id      Information specifc to this mac model
	 *  GenericVY        Y is either 1 or 2         Information specific to SMC version Y
	 *
	 *  So the following order is implied user Mac-XXXXX, main Mac-XXXXX, user GenericVY, main GenericVY
	 *
	 *  Model Information Entry is described as follows:
	 *
	 *  Property  Type      Size                   Value
	 *  -------------------------------------------------------------------------------------------
	 *  hwname    OSData                           Null-terminated SMC codename from compatible
	 *  rev       OSData    6 bytes                SMC firmware revision (REV key value)
	 *  revfb     OSData    6 bytes                SMC flasher base revision (RVBF key value)
	 *  revfu     OSData    6 bytes                SMC flasher update revision (RVUF key value)
	 *
	 *  @return true on success
	 */
	bool obtainModelInfo(SMCInfo &deviceInfo, const char *boardIdentifier, const OSDictionary *main, const OSDictionary *user);

	/**
	 *  Detect whether more than one SMC device is available.
	 *
	 *  @param provider  i/o registry to lookup SMC device at
	 *
	 *  @return true if there is at least 1 SMC device (excluding VirtualSMC)
	 */
	bool devicesPresent(IOService *provider);

public:

	/**
	 *  Decide on whether to load or not.
	 *  When Lilu is off VirtualSMCProvider initialisation is performed here as well.
	 *
	 *  @param provider  parent IOService object
	 *  @param score     probing score
	 *
	 *  @return self if we could load anyhow
	 */
	IOService *probe(IOService *provider, SInt32 *score) override;

	/**
	 *  Actually initialise VirtualSMC device NUB.
	 *  When Lilu is off we will register our service here as well.
	 *
	 *  @param provider  parent IOService object
	 *
	 *  @return true on success
	 */
	bool start(IOService *provider) override;

	/**
	 *  Deinitialise VirtualSMC device NUB.
	 *  Is not meant to be doing anything specific atm.
	 *
	 *  @param provider  parent IOService object
	 */
	void stop(IOService *provider) override;

	/**
	 *  Obtain shared keystore, pmio/mmio protocols need it.
	 *  Also signals vsmc availability.
	 *
	 *  @return true on success
	 */
	static VirtualSMCKeystore *getKeystore() {
		return instance->keystore;
	}

	/**
	 *  Reports whether instance is available.
	 *
	 *  @return true on success
	 */
	static bool isServicingReady() {
		return atomic_load_explicit(&servicingReady, memory_order_acquire);
	}

	/**
	 *  Reports whether we should avoid using MMIO as early as possible to avoid unnecessary patches.
	 *
	 *  @return true for first generation if already known
	 */
	static bool isFirstGeneration() {
		auto &info = getKeystore()->getDeviceInfo();
		return info.getGeneration() == SMCInfo::Generation::V1;
	}

	/**
	 * Perform registration when servicing is already active and it is not in first
	 * generation mode, meaning, we requested for MMIO handler to register the service.
	 */
	static void postMmioReady() {
		bool shouldMmioInit = atomic_load_explicit(&servicingReady, memory_order_acquire);
		atomic_store_explicit(&mmioReady, true, memory_order_release);
		if (shouldMmioInit && !isFirstGeneration()) {
			instance->registerService();
			instance->publishResource(VirtualSMCAPI::SubmitPlugin);
		}
	}

	/**
	 *  Transfer mmio pre-read event to the implementation.
	 *
	 *  @param base  mmio virtual region base address
	 *  @param addr  actual read address in mmio virtual region
	 */
	static void handleRead(mach_vm_address_t base, mach_vm_address_t addr) {
		if (instance && instance->mmio)
			instance->mmio->handleRead(base, addr);
		else
			PANIC("vsmc", "missing mmio instance for read, instance %d", instance != nullptr);
	}

	/**
	 *  Transfer mmio post-write event to the implementation.
	 *
	 *  @param base  mmio virtual region base address
	 *  @param addr  actual write address in mmio virtual region
	 */
	static void handleWrite(mach_vm_address_t base, mach_vm_address_t addr) {
		if (instance && instance->mmio)
			instance->mmio->handleWrite(base, addr);
		else
			PANIC("vsmc", "missing mmio instance for write, instance %d", instance != nullptr);
	}

	/**
	 *  Detect user-specified or os-specific overrides enforcing SMC generation to use.
	 *  Currently the override list includes:
	 *  - os older than 10.8 -> v1
	 *  - Lilu start failure -> v1
	 *  - vsmcgen boot-arg   -> v1 if 1, v2 otherwise
	 *
	 *  @return enforced generation or SMCInfo::Generation::Unspecified
	 */
	static SMCInfo::Generation forcedGeneration();

	/**
	 *  Change interrupt response availability for SMC devices.
	 *
	 *  @param enable  interrupt status
	 */
	static void setInterrupts(bool enable);

	/**
	 *  Post interrupt for a delivery. Those should be sent by the device.
	 *
	 *  @param code      interrupt event code
	 *  @param data      interrupt data if available (copied)
	 *  @param dataSize  interrupt data size
	 *
	 *  @return true if accepted
	 */
	static bool postInterrupt(SMC_EVENT_CODE code, const void *data=nullptr, uint32_t dataSize=0);

	/**
	 *  Consume stored interrupt.
	 *
	 *  @retrun interrupt code or 0 if nothing is found
	 */
	static SMC_EVENT_CODE getInterrupt();

	/**
	 *  Possible watchdog jobs
	 */
	enum WatchDogJob : uint8_t {
		WatchDogDoNothing = 0,
		WatchDogShutdownToS5 = 1,
		WatchDogForceRestart = 2,
		WatchDogForceStartup = 3,
		WatchDogReserved = 4,
		WatchDogShutdownToUnk = 32
	};

	/**
	 *  Post watchdog job
	 *
	 *  @param code     watchdog event code (see WatchDogJob)
	 *  @param timeout  time till firing the job in ms
	 *  @param last     this is the last job, do not accept any more
	 */
	static void postWatchDogJob(uint8_t code, uint64_t timeout, bool last=false);

	/**
	 *  Return emulated device memory as a memory map
	 *
	 *  @param index    device memory index (must be below AppleSMCBufferTotal)
	 *  @param options  mapping options
	 *
	 *  @param mapped memory region
	 */
	IOMemoryMap *mapDeviceMemoryWithIndex(unsigned int index, IOOptionBits options) override;

	/**
	 *  Verify whether this port-mapped i/o operation is supported
	 *
	 *  @param offset  port offset
	 *  @param map     memory map
	 *
	 *  @return true if the operation is legit
	 */
	bool ioVerify(UInt16 &offset, IOMemoryMap *map);

	/**
	 *  Perform 32-bit write (currently forbidden)
	 *
	 *  @param offset  port offset
	 *  @param value   value to write
	 *  @param map     memory map to use
	 */
	void ioWrite32(UInt16 offset, UInt32 value, IOMemoryMap *map) override;

	/**
	 *  Perform 32-bit read (currently forbidden)
	 *
	 *  @param offset  port offset
	 *  @param map     memory map to use
	 *
	 *  @return read value
	 */
	UInt32 ioRead32(UInt16 offset, IOMemoryMap *map) override;

	/**
	 *  Perform 16-bit write (currently forbidden)
	 *
	 *  @param offset  port offset
	 *  @param value   value to write
	 *  @param map     memory map to use
	 */
	void ioWrite16(UInt16 offset, UInt16 value, IOMemoryMap *map) override;

	/**
	 *  Perform 16-bit read (currently forbidden)
	 *
	 *  @param offset  port offset
	 *  @param map     memory map to use
	 *
	 *  @return read value
	 */
	UInt16 ioRead16(UInt16 offset, IOMemoryMap *map) override;

	/**
	 *  Perform 8-bit write
	 *
	 *  @param offset  port offset
	 *  @param value   value to write
	 *  @param map     memory map to use
	 */
	void ioWrite8(UInt16 offset, UInt8 value, IOMemoryMap *map) override;

	/**
	 *  Perform 8-bit read
	 *
	 *  @param offset  port offset
	 *  @param map     memory map to use
	 *
	 *  @return read value
	 */
	UInt8 ioRead8(UInt16 offset, IOMemoryMap *map) override;

	/**
	 *  Interrupt registration implementation, here we catch AppleSMC interrupt handlers to call later
	 *
	 *  @param source  interrupt source
	 *  @param target  delivery target
	 *  @param handler handler to call at interrupt
	 *  @param refCon  reference constant
	 *
	 *  @return kIOReturnSuccess on successful registration
	 */
	IOReturn registerInterrupt(int source, OSObject *target, IOInterruptAction handler, void *refCon) override;

	/**
	 *  Interrupt deregistration implementation (implemented for completeness)
	 *
	 *  @param source  interrupt source
	 *
	 *  @return kIOReturnSuccess on successful deregistration
	 */
	IOReturn unregisterInterrupt(int source) override;

	/**
	 *  Obtain interrupt type (returns InterruptType for valid sources)
	 *
	 *  @param source        interrupt source
	 *  @param interruptType interrupt type if any
	 *
	 *  @return kIOReturnSuccess if source is valid
	 */
	IOReturn getInterruptType(int source, int *interruptType) override;

	/**
	 *  Enable receiving interrupts, until AppleSMC calls this we must not send interrupts to it
	 *
	 *  @param source        interrupt source
	 *
	 *  @return kIOReturnSuccess if source is valid
	 */
	IOReturn enableInterrupt(int source) override;

	/**
	 *  Disable receiving interrupts
	 *
	 *  @param source        interrupt source
	 *
	 *  @return kIOReturnSuccess if source is valid
	 */
	IOReturn disableInterrupt(int source) override;

	/**
	 *  Send interrupt to AppleSMC, this is what we call from MMIO protocol implementation
	 *
	 *  @param source        interrupt source
	 *
	 *  @return kIOReturnSuccess if source is valid
	 */
	IOReturn causeInterrupt(int source) override;

	/**
	 *  Update power state with the new one, here we catch sleep/wake/boot/shutdown calls
	 *  New power state could be the reason for keystore to be saved to NVRAM, for example
	 *
	 *  @param state      power state index (must be below PowerStateMax)
	 *  @param whatDevice power state device
	 */
	IOReturn setPowerState(unsigned long state, IOService *whatDevice) override;

	/**
	 *  An interface for plugin communications.
	 *  See kern_vsmcapi.hpp for more details.
	 *
	 *  @param functionName     normally VirtualSMCSubmitPlugin or ignored
	 *  @param waitForFunction  unused
	 *  @param param1           for VirtualSMCSubmitPlugin it is a pointer to plugin's IOService
	 *  @param param2           for VirtualSMCSubmitPlugin it is a pointer to VirtualSMCAPI::Plugin structure
	 *  @param param3           unused
	 *  @param param4           unused
	 *
	 *  @return kIOReturnSuccess for a successful submission through VirtualSMCSubmitPlugin
	 */
	IOReturn callPlatformFunction(const OSSymbol *functionName, bool waitForFunction, void *param1, void *param2, void *param3, void *param4) override;
};

#endif /* kern_vsmc_hpp */
