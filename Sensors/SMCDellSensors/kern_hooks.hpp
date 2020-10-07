//
//  kern_hooks.hpp
//  SMCDellSensord
//
//  Copyright © 2020 lvs1974. All rights reserved.
//

#ifndef kern_hooks_hpp
#define kern_hooks_hpp

#include <Headers/kern_patcher.hpp>
#include <stdatomic.h>

class KERNELHOOKS {
public:
	bool init();
	void deinit();

	/**
	 *  Returns true if there are any audio samples to play
	 */
	static bool areAudioSamplesAvailable();

private:

	/**
	 *  Patch kext if needed and prepare other patches
	 *
	 *  @param patcher KernelPatcher instance
	 *  @param index   kinfo handle
	 *  @param address kinfo load address
	 *  @param size    kinfo memory size
	 */
	void processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);

	/**
	 *  Hooked methods / callbacks
	 */
	static IOReturn IOAudioEngineUserClient_performClientOutput(void *that, UInt32 firstSampleFrame, UInt32 loopCount, void *bufferSet, UInt32 sampleIntervalHi, UInt32 sampleIntervalLo);
	
	static void IOAudioEngineUserClient_performWatchdogOutput(void *that, void *clientBufferSet, UInt32 generationCount);
	
	/**
	 *  Original method
	 */
	mach_vm_address_t orgIOAudioStream_processOutputSamples {};
	mach_vm_address_t orgIOAudioEngineUserClient_performClientOutput {};
	mach_vm_address_t orgIOAudioEngineUserClient_performWatchdogOutput {};

	/**
	 *  Flag is set to true if any audio samples are available
	 */
	static _Atomic(uint32_t) outputCounter;
	static _Atomic(AbsoluteTime) last_audio_event;
};

#endif /* kern_hooks_hpp */
