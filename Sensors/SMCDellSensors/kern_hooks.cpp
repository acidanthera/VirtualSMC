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
	AbsoluteTime prev_time = atomic_load_explicit(&last_audio_event, memory_order_acquire);
	if (prev_time != 0) {
		uint64_t        nsecs;
		AbsoluteTime    cur_time;
		clock_get_uptime(&cur_time);
		SUB_ABSOLUTETIME(&cur_time, &prev_time);
		absolutetime_to_nanoseconds(cur_time, &nsecs);
		if (nsecs > 60000000000) {
			atomic_store_explicit(&outputCounter, 0, memory_order_release);
			atomic_store_explicit(&last_audio_event, 0, memory_order_seq_cst);
		}
	}
	
	return atomic_load_explicit(&outputCounter, memory_order_acquire) > 0;
}

IOReturn KERNELHOOKS::IOAudioEngineUserClient_performClientOutput(void *that, UInt32 firstSampleFrame, UInt32 loopCount, void *bufferSet, UInt32 sampleIntervalHi, UInt32 sampleIntervalLo)
{
	while (SMIMonitor::IsSmmBeingRead()) {}
	atomic_fetch_add_explicit(&outputCounter, 1, memory_order_release);
	AbsoluteTime cur_time;
	clock_get_uptime(&cur_time);
	atomic_store_explicit(&last_audio_event, cur_time, memory_order_seq_cst);
	IOReturn result = FunctionCast(IOAudioEngineUserClient_performClientOutput,
							  callbackKERNELHOOKS->orgIOAudioEngineUserClient_performClientOutput)(that, firstSampleFrame, loopCount, bufferSet, sampleIntervalHi, sampleIntervalLo);
	atomic_fetch_sub_explicit(&outputCounter, 1, memory_order_release);
	return result;
}

void KERNELHOOKS::IOAudioEngineUserClient_performWatchdogOutput(void *that, void *clientBufferSet, UInt32 generationCount)
{
	while (SMIMonitor::IsSmmBeingRead()) {}
	atomic_fetch_add_explicit(&outputCounter, 1, memory_order_release);
	FunctionCast(IOAudioEngineUserClient_performWatchdogOutput,
							  callbackKERNELHOOKS->orgIOAudioEngineUserClient_performWatchdogOutput)(that, clientBufferSet, generationCount);
	atomic_fetch_sub_explicit(&outputCounter, 1, memory_order_release);
}

void KERNELHOOKS::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size)
{
	if (kextIOAudio.loadIndex == index) {
		DBGLOG("sdell", "%s", kextIOAudio.id);
				
		KernelPatcher::RouteRequest requests[] {
			{"__ZN23IOAudioEngineUserClient19performClientOutputEjjP22IOAudioClientBufferSetjj", IOAudioEngineUserClient_performClientOutput, orgIOAudioEngineUserClient_performClientOutput},
			{"__ZN23IOAudioEngineUserClient21performWatchdogOutputEP22IOAudioClientBufferSetj", IOAudioEngineUserClient_performWatchdogOutput, orgIOAudioEngineUserClient_performWatchdogOutput}
		};
		patcher.routeMultiple(index, requests, address, size);
		if (patcher.getError() == KernelPatcher::Error::NoError) {
			DBGLOG("sdell", "routed is successful", "__ZN13IOAudioStream20processOutputSamplesEP19IOAudioClientBufferjjb");
		} else {
			SYSLOG("sdell", "failed to resolve symbols, error = %d", patcher.getError());
			patcher.clearError();
		}
	}
}
