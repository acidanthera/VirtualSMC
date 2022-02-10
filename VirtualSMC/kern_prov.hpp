//
//  kern_prov.hpp
//  VirtualSMC
//
//  Copyright © 2017 vit9696. All rights reserved.
//

#ifndef kern_prov_hpp
#define kern_prov_hpp

#include <Headers/kern_patcher.hpp>
#include <IOKit/IOService.h>
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <Private/thread_status.h>

#include "kern_pmio.hpp"
#include "kern_mmio.hpp"

class VirtualSMCProvider {
public:

	/**
	 *  Device memory buffer indexes
	 */
	enum AppleSMCBufferIndex {
		AppleSMCBufferPMIO,
		AppleSMCBufferMMIO,
		AppleSMCBufferTotal
	};

	/**
	 *  Obtain a single initialised provider instance
	 */
	static VirtualSMCProvider *getInstance() {
		if (!instance) {
			instance = new VirtualSMCProvider;
			if (instance)
				instance->init();
			else
				SYSLOG("prov", "provider allocation failure");
		}

		return instance;
	}

	/**
	 *  Return emulated device memory as a memory map
	 *
	 *  @param index    device memory index (must be below AppleSMCBufferTotal)
	 *  @param options  mapping options
	 *
	 *  @param mapped memory region
	 */
	static IOMemoryMap *getMapping(unsigned int index, IOOptionBits options=0) {
		if (instance && index < AppleSMCBufferTotal)
			return instance->memoryMaps[index];
		PANIC("prov", "invalid smc buffer %u", index);
		return nullptr;
	}

	/**
	 *  Return firmware support availability (see detectFirmwareBackend)
	 *
	 *  @return true on presence and validity
	 */
	static bool getFirmwareBackendStatus() {
		return instance && instance->firmwareStatus;
	}
	
private:

	/**
	 *  Initialise memory maps and register Lilu callbacks if necessary
	 */
	void init();
	
	/**
	 *  Global instance set after the initialisation for callback access (i.e. kernel_trap wrapping)
	 */
	static VirtualSMCProvider *instance;
	
	/**
	 *  Allocated memory descriptors for i/o emulation
	 */
	IOBufferMemoryDescriptor *memoryDescriptors[AppleSMCBufferTotal] {};

	/**
	 *  Memory mappings of the i/o descriptors
	 */
	IOMemoryMap *memoryMaps[AppleSMCBufferTotal] {};
	
	/**
	 *  Kext patcher done status
	 */
	static bool firstGeneration;

	/**
	 *  Firmware support availability and validity status
	 */
	bool firmwareStatus {false};
	
#if defined(__x86_64__)

	/**
	 *  Lilu patcher load callback, sets memory map protection and routes kernel traps
	 *
	 *  @param kp     kernel patcher instance
	 */
	void onPatcherLoad(KernelPatcher &kp);

	/**
	 *  Lilu kext load callback, responsible for determining AppleSMC layout and enabling our service (when MMIO is on)
	 *
	 *  @param kp     kernel patcher instance
	 *  @param id     loaded kinfo id
	 *  @param slide  loaded slide
	 *  @param size   loaded memory size
	 */
	void onKextLoad(KernelPatcher &kp, size_t index, mach_vm_address_t address, size_t size);

	/**
	 *  There is no support for smcdebug boot argument on 10.9
	 *  We implement it manually by updating gAppleSMCDebugFlags value
	 */
	uint32_t debugFlagMask {0};

	/**
	 *  For better packing we only use 8 bits to describe our i/o access
	 */
	using FaultInfo = uint8_t;

	/**
	 *  Bits 0:5 are used for page index, allowing us to distinguish 64 pages
	 */
	static constexpr FaultInfo FaultIndexMask   = 0x3F;

	/**
	 *  Bit 6 is used for protection upgrade types
	 */
	static constexpr FaultInfo FaultUpgradeMask = 0x40;

	/**
	 *  Bit 7 is used for i/o direction type
	 */
	static constexpr FaultInfo FaultTypeMask    = 0x80;

	/**
	 *  When the page is already present (T_PF_PROT is set) we can just unset the WP bit
	 *  Otherwise we perform a full trap sequence via vm_protect
	 */
	static constexpr FaultInfo FaultUpgradeVM = 0x00;
	static constexpr FaultInfo FaultUpgradeWP = 0x40;

	/**
	 *  We need to handle read and writes differently, thus the flags
	 */
	static constexpr FaultInfo FaultTypeRead  = 0x00;
	static constexpr FaultInfo FaultTypeWrite = 0x80;

	/**
	 *  Assembly trampoline responsible for transferring the control flow to ioTrapHandler
	 */
	struct PACKED Trampoline {
		uint8_t push {0x6A};
		FaultInfo trinfo {};
		uint16_t jmp {0x25FF};
		uint32_t off {0x0};
		void (*proc)() {};
	};

	/**
	 *  We support granular page-sized mmio trapping
	 *  This structure contains the relevant information about original unpatched bytes and addresses
	 */
	struct PageInfo {
		uint8_t org[sizeof(Trampoline)];  /* back up of the original code */
		mach_vm_address_t retAddr;        /* return address */
		mach_vm_address_t mmioAddr;       /* mmio r/w address */
	};

	/**
	 *  Array of mmio-enabled page trap states
	 */
	PageInfo pageInfo[SMCProtocolMMIO::WindowNPages] {};

	/**
	 *  Monitored device mmio area region start and end
	 */
	static mach_vm_address_t monitorStart, monitorEnd;

	/**
	 *  AppleSMC kext memory region start and end
	 *  These are used to determine whether the fault address is an AppleSMC address
	 *  When it is not (e.g. memcpy), we perform stack unwinding until we find an AppleSMC address
	 */
	static mach_vm_address_t monitorSmcStart, monitorSmcEnd;

	/**
	 *  Original kernel_trap routine
	 */
	static mach_vm_address_t orgKernelTrap;

	/**
	 *  Wrapper of kernel_trap responsible for catching MMIO access
	 *  Declared as a template because x86_saved_state_t differ across different os
	 *
	 *  @param state  x86_saved_state_t (see x86_saved_state_1010_t)
	 *  @param lo_spp stack pointer
	 */
	template <typename T>
	static void kernelTrap(T *state, uintptr_t *lo_spp);

	/**
	 *  Original AppleSMC::callPlatformFunction routine
	 */
	static mach_vm_address_t orgCallPlatformFunction;

	/**
	 *  Wrapper for AppleSMC platform function processor avoiding SMC-based panic handling.
	 *  While this is great that SMC may store panic logs now, almost no macs support this,
	 *  and doing it in software leads to unnecessary risks when anybody tries perform a watchdog
	 *  mmio write during the another page fault.
	 *
	 *  @param that             AppleSMC instance
	 *  @param functionName     platform function
	 *  @param waitForFunction  wait for resource or not to
	 *  @param param1           first param
	 *  @param param2           relevant pe index (e.g. kPEPanicBegin)
	 *  @param param3           third param
	 *  @param param4           fourth param
	 */
	static IOReturn filterCallPlatformFunction(void *that, const OSSymbol *functionName, bool waitForFunction,
											   void *param1, void *param2, void *param3, void *param4);

	/**
	 *  Original AppleSMC::smcStopWatchdogTimer() routine.
	 */
	static mach_vm_address_t orgStopWatchdogTimer;
	
	/**
	 *  Wrapper for AppleSMC::smcStopWatchdogTimer() avoiding MMIO during interrupts.
	 *  Similar as filterCallPlatformFunction, required on macOS 11 within AppleSMC::smcHandlePEHaltRestart().
	 *
	 *  @param that  AppleSMC instance
	 */
	static IOReturn filterStopWatchdogTimer(void *that);

	/**
	 *  Restores the original code execution path and
	 *  handles a SMC reply if necessary
	 *
	 *  @param trinfo  encoded page index and trap type
	 *
	 *  @return code return address
	 */
	static mach_vm_address_t ioProcessResult(FaultInfo trinfo);
	
#endif
	
};

#endif /* kern_prov_hpp */
