//
//  kern_hooks.cpp
//  SMCDellSensors
//
//  Copyright © 2020 lvs1974. All rights reserved.
//

#include <Headers/kern_api.hpp>
#include <Headers/kern_util.hpp>

#include "kern_hooks.hpp"
#include "SMIMonitor.hpp"

static KERNELHOOKS *callbackKERNELHOOKS = nullptr;
_Atomic(bool) KERNELHOOKS::tempLock = false;
_Atomic(uint32_t) KERNELHOOKS::outputCounter = 0;
_Atomic(AbsoluteTime) KERNELHOOKS::last_audio_event = 0;

static const char *kextIOAudioFamily[] { "/System/Library/Extensions/IOAudioFamily.kext/Contents/MacOS/IOAudioFamily" };

static KernelPatcher::KextInfo kextIOAudio { "com.apple.iokit.IOAudioFamily", kextIOAudioFamily, 1, {true, true}, {}, KernelPatcher::KextInfo::Unloaded };

bool KERNELHOOKS::init()
{
	DBGLOG("sdell", "KERNELHOOKS::init is called");
	
	callbackKERNELHOOKS = this;

	lilu.onKextLoadForce(&kextIOAudio, 1,
	[](void *user, KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
		callbackKERNELHOOKS->processKext(patcher, index, address, size);
	}, this);

	return true;
}

void KERNELHOOKS::deinit()
{
}

bool KERNELHOOKS::areAudioSamplesAvailable()
{
	if (atomic_load_explicit(&tempLock, memory_order_acquire))
		return true;
		
	if (last_audio_event != 0) {
		uint64_t        nsecs;
		AbsoluteTime    cur_time;
		clock_get_uptime(&cur_time);
		SUB_ABSOLUTETIME(&cur_time, &last_audio_event);
		absolutetime_to_nanoseconds(cur_time, &nsecs);
		if (nsecs > 10000000000) {
			atomic_store_explicit(&outputCounter, 0, memory_order_release);
			atomic_store_explicit(&last_audio_event, 0, memory_order_seq_cst);
		}
	}
	
	return atomic_load_explicit(&outputCounter, memory_order_acquire) > 0;
}

IOReturn KERNELHOOKS::IOAudioStream_processOutputSamples(void *that, void *clientBuffer, UInt32 firstSampleFrame, UInt32 loopCount, bool samplesAvailable)
{
	while (SMIMonitor::IsSmmBeingRead()) {}
	atomic_store_explicit(&tempLock, true, memory_order_release);
	int result = FunctionCast(IOAudioStream_processOutputSamples,
							  callbackKERNELHOOKS->orgIOAudioStream_processOutputSamples)(that, clientBuffer, firstSampleFrame, loopCount, samplesAvailable);
	if (samplesAvailable && (result == kIOReturnSuccess)) {
		atomic_fetch_add_explicit(&outputCounter, 1, memory_order_release);
		uint64_t temptime;
		clock_get_uptime(&temptime);
		atomic_store_explicit(&last_audio_event, temptime, memory_order_seq_cst);
	} else if (atomic_load_explicit(&outputCounter, memory_order_acquire) > 0) {
		atomic_fetch_sub_explicit(&outputCounter, 1, memory_order_release);
	}

	atomic_store_explicit(&tempLock, false, memory_order_release);
	return result;
}

void KERNELHOOKS::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size)
{
	if (kextIOAudio.loadIndex == index) {
		DBGLOG("sdell", "%s", kextIOAudio.id);
				
		KernelPatcher::RouteRequest request {
				"__ZN13IOAudioStream20processOutputSamplesEP19IOAudioClientBufferjjb",
				IOAudioStream_processOutputSamples,
				orgIOAudioStream_processOutputSamples
		};
		patcher.routeMultiple(index, &request, 1, address, size);
		if (patcher.getError() == KernelPatcher::Error::NoError) {
			DBGLOG("sdell", "routed %s", "__ZN13IOAudioStream20processOutputSamplesEP19IOAudioClientBufferjjb");
		} else {
			SYSLOG("sdell", "failed to resolve %s, error = %d", "__ZN13IOAudioStream20processOutputSamplesEP19IOAudioClientBufferjjb", patcher.getError());
			patcher.clearError();
		}
	}
}
