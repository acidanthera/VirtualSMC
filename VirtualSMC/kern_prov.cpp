//
//  kern_prov.cpp
//  VirtualSMC
//
//  Copyright © 2017 vit9696. All rights reserved.
//

#include <Headers/kern_util.hpp>
#include <Headers/kern_patcher.hpp>
#include <Headers/kern_api.hpp>
#include <Headers/kern_efi.hpp>
#include <Headers/kern_crypto.hpp>
#include <Headers/kern_nvram.hpp>
#include <Headers/plugin_start.hpp>

#include <IOKit/IOMapper.h>

#include "kern_prov.hpp"
#include "kern_efiend.hpp"
#include "kern_vsmc.hpp"
#include "kern_handler.h"

#ifndef T_PF_PROT
#define T_PF_PROT     0x1   /* protection violation */
#define T_PF_WRITE    0x2   /* write access */
#define T_PF_USER     0x4   /* from user state */
#define T_PF_RSVD     0x8   /* reserved bit set to 1 */
#define T_PF_EXECUTE  0x10  /* instruction fetch when NX */
#endif

#ifndef VM_MIN_KERNEL_ADDRESS
#define VM_MIN_KERNEL_ADDRESS			((vm_offset_t) 0xFFFFFF8000000000UL)
#define VM_MIN_KERNEL_AND_KEXT_ADDRESS	(VM_MIN_KERNEL_ADDRESS - 0x80000000ULL)
#define VM_MAX_KERNEL_ADDRESS			((vm_offset_t) 0xFFFFFFFFFFFFEFFFUL)
#endif

VirtualSMCProvider *VirtualSMCProvider::instance;

#if defined(__x86_64__)

static const char *kextPath[] {
	"/System/Library/Extensions/AppleSMC.kext/Contents/MacOS/AppleSMC"
};

static KernelPatcher::KextInfo kextAppleSmc {"com.apple.driver.AppleSMC", kextPath, arrsize(kextPath), {true}, {}, KernelPatcher::KextInfo::Unloaded };

mach_vm_address_t VirtualSMCProvider::monitorStart, VirtualSMCProvider::monitorEnd;
mach_vm_address_t VirtualSMCProvider::monitorSmcStart, VirtualSMCProvider::monitorSmcEnd;
mach_vm_address_t VirtualSMCProvider::orgKernelTrap;
mach_vm_address_t VirtualSMCProvider::orgCallPlatformFunction;
mach_vm_address_t VirtualSMCProvider::orgStopWatchdogTimer;

#endif

bool VirtualSMCProvider::firstGeneration;

void VirtualSMCProvider::init() {
	constexpr size_t allocSizes[AppleSMCBufferTotal] {
		SMCProtocolPMIO::WindowAllocSize,
		SMCProtocolMMIO::WindowAllocSize
	};

	constexpr size_t windowAligns[AppleSMCBufferTotal] {
		SMCProtocolPMIO::WindowAlign,
		SMCProtocolMMIO::WindowAlign
	};

	for (size_t i = 0; i < AppleSMCBufferTotal; i++) {
		// IOMapper::gSystem is both an AppleVTD instance and an event bitset waiting until it arrives (great idea).
		// XNU decides whether IOMapper will arrive based on kHasMapper (0x2) bit in IOMapper::gSystem.
		// This bit is directly not set when dart=0/-x boot-args are passed when SIP FS protection is off.
		// Otherwise, when IOMMU is unavailable (detected by missing DMAR table), it is IOPCIFamily job
		// to unset the bit in a call to IOPCIPlatformInitialize -> AppleVTD::install.
		// In 10.12 and earlier this is done in IOPCIBridge::configOp.
		// In 10.13 and newer this is done in AppleACPIPlatformExpert::start -> ::initPCIExpressSupport (does it still race :).
		//
		// IOBufferMemoryDescriptor::inTaskWithOptions calls to IOMapper::checkForSystemMapper unless kIOMemoryMapperNone
		// is provided, which forwards the call to waitForSystemMapper if kHasMapper bit is set.
		// One may guess that IOPCIBridge::configOp is called later, and that this whole code results in an infinite
		// loop in IOMapper::waitForSystemMapper() if VTD is unavailable on macOS prior 10.13.
		//
		// A friendly fix would have been removing kHasMapper from IOMapper::gSystem, which is safe, since
		// the class instance is 8-byte aligned, yet that would result in issues on real Macs.
		// reinterpret_cast<uintptr_t &>(IOMapper::gSystem) &= ~static_cast<uintptr_t>(kWaitMask);
		// So we pass kIOMemoryMapperNone, which seems to be natural under these conditions.
		memoryDescriptors[i] = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, kIOMemoryMapperNone, allocSizes[i], windowAligns[i]);
		if (memoryDescriptors[i]) {
			auto r = memoryDescriptors[i]->prepare();
			if (r == kIOReturnSuccess) {
				memoryMaps[i] = memoryDescriptors[i]->map();
				if (memoryMaps[i]) {
					bzero(reinterpret_cast<void *>(memoryMaps[i]->getVirtualAddress()), allocSizes[i]);
					DBGLOG("prov", "descriptor %lu was mapped", i);
				} else {
					PANIC("prov", "descriptor %lu map failure", i);
				}
			} else {
				PANIC("prov", "descriptor %lu prepare failure %X", i, r);
			}
		} else {
			PANIC("prov", "descriptor %lu alloc failure", i);
		}
	}

	firmwareStatus = EfiBackend::detectFirmwareBackend();

#if defined(__x86_64__)
	// When we have no Lilu we should avoid any use of it
	// Same assumed for 1st generation smc
	bool forceLegacy = VirtualSMC::forcedGeneration() == SMCInfo::Generation::V1;

	if (!forceLegacy) {
		auto err = lilu.onPatcherLoad([](void *user, KernelPatcher &patcher){
			static_cast<VirtualSMCProvider *>(user)->onPatcherLoad(patcher);
		}, this);
		if (err != LiluAPI::Error::NoError)
			SYSLOG("prov", "failed to register Lilu patcher load cb");

		if (getKernelVersion() <= KernelVersion::Mavericks)
			lilu_get_boot_args("smcdebug", &debugFlagMask, sizeof(debugFlagMask));

		err = lilu.onKextLoad(&kextAppleSmc, 1, [](void *user, KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
			static_cast<VirtualSMCProvider *>(user)->onKextLoad(patcher, index, address, size);
		}, this);

		if (err != LiluAPI::Error::NoError)
			SYSLOG("prov", "failed to register Lilu kext load cb");
	} else {
#endif
		SYSLOG("prov", "legacy mode, most of the features are disabled!");
		
#if defined(__x86_64__)
	}
#endif
}

#if defined(__x86_64__)

void VirtualSMCProvider::onPatcherLoad(KernelPatcher &kp) {
	firstGeneration = VirtualSMC::isServicingReady() && VirtualSMC::isFirstGeneration();
	if (firstGeneration) {
		DBGLOG("prov", "falling back to legacy generation at patcher");
		return;
	}

	mach_vm_address_t kernelTrapWrapper;
	auto kernelVersion = getKernelVersion();
	if (kernelVersion >= KernelVersion::Yosemite)
		kernelTrapWrapper = reinterpret_cast<mach_vm_address_t>(kernelTrap<x86_saved_state_1010_t>);
	else if (kernelVersion == KernelVersion::Mavericks)
		kernelTrapWrapper = reinterpret_cast<mach_vm_address_t>(kernelTrap<x86_saved_state_109_t>);
	else
		kernelTrapWrapper = reinterpret_cast<mach_vm_address_t>(kernelTrap<x86_saved_state_108_t>);
	KernelPatcher::RouteRequest req("_kernel_trap", kernelTrapWrapper, orgKernelTrap);
	if (!kp.routeMultipleLong(KernelPatcher::KernelID, &req, 1))
		return;

	const SMCInfo::Memory *memInfo[AppleSMCBufferTotal] {
		SMCProtocolPMIO::MemoryInfo,
		SMCProtocolMMIO::MemoryInfo
	};

	const size_t memInfoSize[AppleSMCBufferTotal] {
		SMCProtocolPMIO::MemoryInfoSize,
		SMCProtocolMMIO::MemoryInfoSize
	};

	for (size_t i = 0; i < AppleSMCBufferTotal; i++) {
		for (size_t j = 0; j < memInfoSize[i]; j++) {
			auto ret = vm_protect(kernel_map, memoryMaps[i]->getVirtualAddress() + memInfo[i][j].start, memInfo[i][j].size, FALSE, memInfo[i][j].prot);
			if (ret != KERN_SUCCESS)
				PANIC("prov", "failed to set prot of %08X size %08X to %02X error %d", static_cast<uint32_t>(memInfo[i][j].start),
					  static_cast<uint32_t>(memInfo[i][j].size), memInfo[i][j].prot, ret);
		}
	}
}

void VirtualSMCProvider::onKextLoad(KernelPatcher &kp, size_t index, mach_vm_address_t address, size_t size) {

	if (!firstGeneration && !monitorStart && !monitorEnd && kextAppleSmc.loadIndex == index) {
		DBGLOG("prov", "caught AppleSMC kext", kextAppleSmc.id);

		firstGeneration = VirtualSMC::isServicingReady() && VirtualSMC::isFirstGeneration();
		if (firstGeneration) {
			DBGLOG("prov", "falling back to legacy generation at kext load");
			return;
		}

		if (debugFlagMask > 0) {
			auto flags = kp.solveSymbol(index, "_gAppleSMCDebugFlags");
			if (flags) {
				DBGLOG("prov", "updated _gAppleSMCDebugFlags at %08X", static_cast<uint32_t>(flags));
				*reinterpret_cast<uint32_t *>(flags) = debugFlagMask & 0xF;
			} else {
				SYSLOG("prov", "failed to solve _gAppleSMCDebugFlags");
				kp.clearError();
			}
		}

		// SMC-based panic handling appeared in 10.12.x
		if (getKernelVersion() >= KernelVersion::Sierra) {
			KernelPatcher::RouteRequest req("__ZN8AppleSMC20callPlatformFunctionEPK8OSSymbolbPvS3_S3_S3_", filterCallPlatformFunction, orgCallPlatformFunction);
			if (!kp.routeMultiple(index, &req, 1))
				SYSLOG("prov", "failed route AppleSMC::callPlatformFunction on supported os");
			
			if (getKernelVersion() >= KernelVersion::BigSur) {
				KernelPatcher::RouteRequest reqPe("__ZN8AppleSMC20smcStopWatchdogTimerEv", filterStopWatchdogTimer, orgStopWatchdogTimer);
				if (!kp.routeMultiple(index, &reqPe, 1))
					SYSLOG("prov", "failed route AppleSMC::smcStopWatchdogTimer on supported os");
			}
		}

		monitorSmcStart = address;
		monitorSmcEnd = monitorSmcStart + size;
		monitorStart = memoryMaps[AppleSMCBufferMMIO]->getVirtualAddress();
		monitorEnd = monitorStart + SMCProtocolMMIO::WindowSize;

		VirtualSMC::postMmioReady();
	}
}

template <typename T>
void VirtualSMCProvider::kernelTrap(T *state, uintptr_t *lo_spp) {
	if (state->flavor == x86_SAVED_STATE64 && state->ss_64.isf.trapno == EXC_I386_PGFLT) {
		mach_vm_address_t faultAddr = state->ss_64.cr2;
		// Note that this is false until monitorStart/monitorEnd are loaded, because they are zero-initialised.
		if (faultAddr >= monitorStart && faultAddr < monitorEnd) {
			mach_vm_address_t retAddr = state->ss_64.isf.rip;
			if (retAddr >= monitorSmcStart && retAddr < monitorSmcEnd) {
				// Simple case, fault instruction is from AppleSMC
				retAddr += Disassembler::quickInstructionSize(retAddr, 1);
			} else {
				// More complex case, fault instruction is from memcpy or something similar, which we should never patch.
				// We extract the AppleSMC address from the stack, which is the call address of a common function.
				// A lot of those functions omit frame pointer for speed reasons, so we perform raw stack bruteforce.
				retAddr = 0;
				if (state->ss_64.isf.rsp > VM_MIN_KERNEL_AND_KEXT_ADDRESS && state->ss_64.isf.rsp < VM_MAX_KERNEL_ADDRESS) {
					auto sp = reinterpret_cast<mach_vm_address_t *>(state->ss_64.isf.rsp);
					for (size_t i = 0; i < 256 && !retAddr; i++) {
						//DBGLOG("%lu %llx", i, retAddr);
						if (sp[i] >= monitorSmcStart && sp[i] < monitorSmcEnd)
							retAddr = sp[i];
					}
				}

				//SYSTRACE("prov @ retaddr=0x%llx monitorSmcStart=0x%llx monitorSmcEnd=0x%llx", retAddr, monitorSmcStart, monitorSmcEnd);

				if (!retAddr)
					PANIC("prov", "unable to find ret addr");
			}

			auto errorCode = state->ss_64.isf.err;
			const uint8_t pageIndex = ((faultAddr - monitorStart) / PAGE_SIZE) & FaultIndexMask;
			const uint8_t faultType = (errorCode & T_PF_WRITE) ? FaultTypeWrite : FaultTypeRead;
			const uint8_t faultUpgrade = (errorCode & T_PF_PROT) ? FaultUpgradeWP : FaultUpgradeVM;
			auto &info = instance->pageInfo[pageIndex];

			lilu_os_memcpy(info.org, reinterpret_cast<void *>(retAddr), sizeof(Trampoline));
			info.retAddr = retAddr;
			info.mmioAddr = faultAddr;

#if 0
			DBGLOG("prov", "fault addr is %08X %08X ret addr is %08X %08X code %d",
				   static_cast<uint32_t>(faultAddr >> 32), static_cast<uint32_t>(faultAddr & 0xffffffff),
				   static_cast<uint32_t>(retAddr >> 32), static_cast<uint32_t>(retAddr & 0xffffffff),
				   static_cast<uint32_t>(errorCode));

			DBGLOG("prov", "monitor start is %08X %08X monitor end is %08X %08X",
				   static_cast<uint32_t>(monitorStart >> 32), static_cast<uint32_t>(monitorStart & 0xffffffff),
				   static_cast<uint32_t>(monitorEnd >> 32), static_cast<uint32_t>(monitorEnd & 0xffffffff));
#endif

			//DBGLOG("prov", "trap at %08X inst size %lu", static_cast<uint32_t>(faultAddr - monitorStart), sz);

			// We receive T_PF_PROT when we need to upgrade from read-only pages, and in this case we need vm_fault.
			if (faultUpgrade == FaultUpgradeVM) {
				//DBGLOG("prov", "prot upgrade to ro page %u", pageIndex);

				MachInfo::setInterrupts(true);
				auto ret = vm_protect(kernel_map, monitorStart + pageIndex*PAGE_SIZE, PAGE_SIZE, FALSE, VM_PROT_READ|VM_PROT_WRITE);
				if (ret != KERN_SUCCESS)
					PANIC("prov", "cannot upgrade to ro page %u error %d", pageIndex, ret);
				MachInfo::setInterrupts(false);
				//DBGLOG("prov", "prot upgrade to ro page %u done", pageIndex);

				// Ensure that our fault enables write protection, since we may write stuff now.
				state->ss_64.isf.err |= T_PF_WRITE;
				FunctionCast(kernelTrap<T>, orgKernelTrap)(state, lo_spp);

				if (faultType == FaultTypeRead)
					VirtualSMC::handleRead(monitorStart, info.mmioAddr);
			} else {
				MachInfo::setInterrupts(state->ss_64.isf.rflags & EFL_IF);
			}

			Trampoline t;
			t.trinfo = pageIndex | faultType | faultUpgrade;
			t.proc = ioTrapHandler;

			// Disable preemption to keep WP bit
			if (MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) == KERN_SUCCESS)
				*reinterpret_cast<Trampoline *>(retAddr) = t;
			else
				PANIC("prov", "trap cannot disable wp");

			// Write protection is left disabled since we sync anyway

			return;
		}
	}
	
	FunctionCast(kernelTrap<T>, orgKernelTrap)(state, lo_spp);
}

IOReturn VirtualSMCProvider::filterCallPlatformFunction(void *that, const OSSymbol *functionName, bool waitForFunction,
														void *param1, void *param2, void *param3, void *param4) {
	// Always check for invalid args
	if (!that || !functionName)
		return kIOReturnBadArgument;

	auto code = static_cast<int>(reinterpret_cast<uintptr_t>(param2));

	if (code == kPEPanicBegin || code == kPEPanicSync || code == kPEPanicEnd) {
		// Ensure that watchdog is disabled
		if (code == kPEPanicBegin)
			VirtualSMC::postWatchDogJob(VirtualSMC::WatchDogDoNothing, 0, true);
		return kIOReturnSuccess;
	}

	return FunctionCast(filterCallPlatformFunction, orgCallPlatformFunction)
		(that, functionName, waitForFunction, param1, param2, param3, param4);
}

IOReturn VirtualSMCProvider::filterStopWatchdogTimer(void *that) {
	if (ml_get_interrupts_enabled())
		return FunctionCast(filterStopWatchdogTimer, orgStopWatchdogTimer)(that);
	
	VirtualSMC::postWatchDogJob(VirtualSMC::WatchDogDoNothing, 0, true);
	return kIOReturnSuccess;
}

mach_vm_address_t VirtualSMCProvider::ioProcessResult(FaultInfo trinfo) {
	const uint8_t pageIndex = trinfo & FaultIndexMask;
	if (pageIndex < SMCProtocolMMIO::WindowNPages) {
		auto &info = instance->pageInfo[pageIndex];
		const uint8_t faultType = trinfo & FaultTypeMask;
		const uint8_t faultUpgrade = trinfo & FaultUpgradeMask;

		lilu_os_memcpy(reinterpret_cast<void *>(info.retAddr), info.org, sizeof(Trampoline));

		if (MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock) == KERN_SUCCESS) {
			DBGLOG("prov", "io result page %u mmio %08X: %08X", pageIndex, static_cast<uint32_t>(info.mmioAddr - monitorStart),
				   *reinterpret_cast<uint32_t *>(info.mmioAddr));
			
			if (faultType == FaultTypeWrite)
				VirtualSMC::handleWrite(monitorStart, info.mmioAddr);
			
			if (faultUpgrade == FaultUpgradeVM) {
				//DBGLOG("prov", "prot downgrade ro page %u", pageIndex);
				auto ret = vm_protect(kernel_map, monitorStart + pageIndex*PAGE_SIZE, PAGE_SIZE, FALSE, VM_PROT_NONE);
				if (ret != KERN_SUCCESS)
					PANIC("prov", "cannot downgrade ro page %u error %d", pageIndex, ret);
				//DBGLOG("prov", "prot downgrade ro page %u done", pageIndex);
			}
			
			//DBGLOG("prov", "returning to 0x%08X", static_cast<uint32_t>(info.ret_addr));
			return info.retAddr;
		} else {
			PANIC("prov", "handle cannot enable wp");
		}
	}
	
	PANIC("prov", "handle page out of bounds %u!", pageIndex);
	return 0;
}

#endif
