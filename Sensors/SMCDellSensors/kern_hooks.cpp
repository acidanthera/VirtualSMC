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
_Atomic(bool) volatile KERNELHOOKS::audioSamplesAvailable = false;
AbsoluteTime KERNELHOOKS::last_audio_event = 0;

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
	if (!audioSamplesAvailable)
		return false;
	
	if (last_audio_event != 0) {
		uint64_t        nsecs;
		AbsoluteTime    cur_time;
		clock_get_uptime(&cur_time);
		SUB_ABSOLUTETIME(&cur_time, &last_audio_event);
		absolutetime_to_nanoseconds(cur_time, &nsecs);
		if (nsecs > 10000000000) {
			audioSamplesAvailable = false;
			last_audio_event = 0;
		}
	}
	
	return audioSamplesAvailable;
}

IOReturn KERNELHOOKS::IOAudioStream_processOutputSamples(void *that, void *clientBuffer, UInt32 firstSampleFrame, UInt32 loopCount, bool samplesAvailable)
{
	callbackKERNELHOOKS->audioSamplesAvailable = true;
	int attempts = 20;
	while (SMIMonitor::smmIsBeingRead && --attempts >= 0)
		IOSleep(1);
	int result = FunctionCast(IOAudioStream_processOutputSamples,
							  callbackKERNELHOOKS->orgIOAudioStream_processOutputSamples)(that, clientBuffer, firstSampleFrame, loopCount, samplesAvailable);
	callbackKERNELHOOKS->audioSamplesAvailable = samplesAvailable && (result == kIOReturnSuccess);
	if (callbackKERNELHOOKS->audioSamplesAvailable)
		clock_get_uptime(&last_audio_event);
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
