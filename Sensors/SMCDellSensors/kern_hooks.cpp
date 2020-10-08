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
atomic_flag KERNELHOOKS::busy = {};

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

IOReturn KERNELHOOKS::IOAudioEngineUserClient_performClientOutput(void *that, UInt32 firstSampleFrame, UInt32 loopCount, void *bufferSet, UInt32 sampleIntervalHi, UInt32 sampleIntervalLo)
{
	while (atomic_flag_test_and_set_explicit(&busy, memory_order_acquire)) { IOSleep(0); }
	IOReturn result = FunctionCast(IOAudioEngineUserClient_performClientOutput,
							  callbackKERNELHOOKS->orgIOAudioEngineUserClient_performClientOutput)(that, firstSampleFrame, loopCount, bufferSet, sampleIntervalHi, sampleIntervalLo);
	atomic_flag_clear_explicit(&busy, memory_order_release);
	return result;
}

void KERNELHOOKS::IOAudioEngineUserClient_performWatchdogOutput(void *that, void *clientBufferSet, UInt32 generationCount)
{
	while (atomic_flag_test_and_set_explicit(&busy, memory_order_acquire)) { IOSleep(0); }
	FunctionCast(IOAudioEngineUserClient_performWatchdogOutput,
							  callbackKERNELHOOKS->orgIOAudioEngineUserClient_performWatchdogOutput)(that, clientBufferSet, generationCount);
	atomic_flag_clear_explicit(&busy, memory_order_release);
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
