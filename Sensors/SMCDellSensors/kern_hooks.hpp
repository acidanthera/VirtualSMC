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
#include <IOKit/IOService.h>

class KERNELHOOKS {
public:
	bool init();
	void deinit();

	/**
	 *  Atomic flag is used for mutual exlusion of audio output and smm access
	 */
	static atomic_uint active_output;
	
	static void activateTimer();
	
	static void activateTimer(UInt32 delay);

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

	static IOReturn IOAudioEngineUserClient_performClientInput(void *that, UInt32 firstSampleFrame, void *bufferSet);
			
	static IOReturn IOAudioStream_processOutputSamples(void *that, void *clientBuffer, UInt32 firstSampleFrame, UInt32 loopCount, bool samplesAvailable);

	static int64_t IOBluetoothDevice_moreIncomingData(void *that, void *arg1, unsigned int arg2);

	static int64_t IOBluetoothL2CAPChannelUserClient_writeWL(void *that, void *arg1, uint16_t arg2, uint64_t arg3, uint64_t arg4);

	static int64_t IOBluetoothL2CAPChannelUserClient_writeOOLWL(void *that, uint64_t arg1, uint16_t arg2, uint64_t arg3, uint64_t arg4);

	static int64_t IOBluetoothL2CAPChannelUserClient_WriteAsyncAudioData_Trap(void *that, void *arg1, uint16_t arg2, uint64_t arg3, uint64_t arg4);

	static int64_t IOBluetoothL2CAPChannelUserClient_callBackAfterDataIsSent(void *that, IOService *service, void *channel, int arg1, uint64_t arg2, uint64_t arg3);
			
	/**
	 *  Original method
	 */
	mach_vm_address_t orgIOAudioEngineUserClient_performClientOutput {};
	mach_vm_address_t orgIOAudioEngineUserClient_performWatchdogOutput {};
	mach_vm_address_t orgIOAudioEngineUserClient_performClientInput {};
	mach_vm_address_t orgIOAudioStream_processOutputSamples {};
	
	mach_vm_address_t orgIOBluetoothDevice_moreIncomingData {};
	mach_vm_address_t orgIOBluetoothL2CAPChannelUserClient_writeWL {};
	mach_vm_address_t orgIOBluetoothL2CAPChannelUserClient_writeOOLWL {};
	mach_vm_address_t orgIOBluetoothL2CAPChannelUserClient_WriteAsyncAudioData_Trap {};
	mach_vm_address_t orgIOBluetoothL2CAPChannelUserClient_WriteAsyncAudioData_TrapWL {};
	mach_vm_address_t orgIOBluetoothL2CAPChannelUserClient_callBackAfterDataIsSent {};
		
	IOWorkLoop *workLoop {};
	IOTimerEventSource *eventTimer {};
	
	/**
	 *  Current progress mask
	 */
	struct ProcessingState {
		enum {
			NothingReady = 0,
			IOAudioFamilyRouted = 1,
			IOBluetoothFamilyRouted = 2,
			EverythingDone = IOAudioFamilyRouted | IOBluetoothFamilyRouted
		};
	};
	int progressState {ProcessingState::NothingReady};
};

#endif /* kern_hooks_hpp */
