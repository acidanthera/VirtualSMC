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

#include <IOKit/IOTimerEventSource.h>

static KERNELHOOKS *callbackKERNELHOOKS = nullptr;
_Atomic(uint32_t) KERNELHOOKS::active_output = 0;

static constexpr size_t kextListSize {2};
static constexpr uint32_t delay = 9;
static constexpr int max_attempts = 100;

static const char *kextIOAudioFamily[]     { "/System/Library/Extensions/IOAudioFamily.kext/Contents/MacOS/IOAudioFamily" };
static const char *kextIOBluetoothFamily[] { "/System/Library/Extensions/IOBluetoothFamily.kext/Contents/MacOS/IOBluetoothFamily" };
static KernelPatcher::KextInfo kextList[kextListSize] {
	{ "com.apple.iokit.IOAudioFamily",     kextIOAudioFamily,     1, {true}, {}, KernelPatcher::KextInfo::Unloaded },
	{ "com.apple.iokit.IOBluetoothFamily", kextIOBluetoothFamily, 1, {true}, {}, KernelPatcher::KextInfo::Unloaded }
};

bool KERNELHOOKS::init()
{
	DBGLOG("sdell", "KERNELHOOKS::init is called");
	
	callbackKERNELHOOKS = this;

	lilu.onKextLoadForce(kextList, kextListSize,
	[](void *user, KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
		callbackKERNELHOOKS->processKext(patcher, index, address, size);
	}, this);

	return true;
}

void KERNELHOOKS::deinit()
{
}

void KERNELHOOKS::activateTimer(UInt32 delay)
{
	callbackKERNELHOOKS->eventTimer->cancelTimeout();
	callbackKERNELHOOKS->eventTimer->setTimeoutMS(delay);
}

void KERNELHOOKS::activateTimer()
{
	activateTimer(delay);
}

IOReturn KERNELHOOKS::IOAudioEngineUserClient_performClientOutput(void *that, UInt32 firstSampleFrame, UInt32 loopCount, void *bufferSet, UInt32 sampleIntervalHi, UInt32 sampleIntervalLo)
{
	int attempts = max_attempts;
	while (atomic_load_explicit(&SMIMonitor::busy, memory_order_acquire) && --attempts > 0) IOSleep(0);
	atomic_fetch_add_explicit(&active_output, 1, memory_order_release);
	IOReturn result = FunctionCast(IOAudioEngineUserClient_performClientOutput,
							  callbackKERNELHOOKS->orgIOAudioEngineUserClient_performClientOutput)(that, firstSampleFrame, loopCount, bufferSet, sampleIntervalHi, sampleIntervalLo);
	if (result == KERN_SUCCESS && atomic_load_explicit(&KERNELHOOKS::active_output, memory_order_acquire) == 1)
		activateTimer();
	else if (atomic_load_explicit(&KERNELHOOKS::active_output, memory_order_acquire))
		atomic_fetch_sub_explicit(&active_output, 1, memory_order_release);
	return result;
}

void KERNELHOOKS::IOAudioEngineUserClient_performWatchdogOutput(void *that, void *clientBufferSet, UInt32 generationCount)
{
	int attempts = max_attempts;
	while (atomic_load_explicit(&SMIMonitor::busy, memory_order_acquire) && --attempts > 0) IOSleep(0);
	atomic_fetch_add_explicit(&active_output, 1, memory_order_release);
	FunctionCast(IOAudioEngineUserClient_performWatchdogOutput,
							  callbackKERNELHOOKS->orgIOAudioEngineUserClient_performWatchdogOutput)(that, clientBufferSet, generationCount);
	if (atomic_load_explicit(&KERNELHOOKS::active_output, memory_order_acquire) == 1)
		activateTimer();
	else if (atomic_load_explicit(&KERNELHOOKS::active_output, memory_order_acquire))
		atomic_fetch_sub_explicit(&active_output, 1, memory_order_release);
}

IOReturn KERNELHOOKS::IOAudioEngineUserClient_performClientInput(void *that, UInt32 firstSampleFrame, void *bufferSet)
{
	int attempts = max_attempts;
	while (atomic_load_explicit(&SMIMonitor::busy, memory_order_acquire) && --attempts > 0) IOSleep(0);
	atomic_fetch_add_explicit(&active_output, 1, memory_order_release);
	IOReturn result = FunctionCast(IOAudioEngineUserClient_performClientInput,
							  callbackKERNELHOOKS->orgIOAudioEngineUserClient_performClientInput)(that, firstSampleFrame, bufferSet);
	if (result == KERN_SUCCESS && atomic_load_explicit(&KERNELHOOKS::active_output, memory_order_acquire) == 1)
		activateTimer();
	else if (atomic_load_explicit(&KERNELHOOKS::active_output, memory_order_acquire))
		atomic_fetch_sub_explicit(&active_output, 1, memory_order_release);
	return result;
}

IOReturn KERNELHOOKS::IOAudioStream_processOutputSamples(void *that, void *clientBuffer, UInt32 firstSampleFrame, UInt32 loopCount, bool samplesAvailable)
{
	int attempts = max_attempts;
	while (atomic_load_explicit(&SMIMonitor::busy, memory_order_acquire) && --attempts > 0) IOSleep(0);
	atomic_fetch_add_explicit(&active_output, 1, memory_order_release);
	IOReturn result = FunctionCast(IOAudioStream_processOutputSamples,
							  callbackKERNELHOOKS->orgIOAudioStream_processOutputSamples)(that, clientBuffer, firstSampleFrame, loopCount, samplesAvailable);
	if (result == KERN_SUCCESS && atomic_load_explicit(&KERNELHOOKS::active_output, memory_order_acquire) == 1)
		activateTimer();
	else if (atomic_load_explicit(&KERNELHOOKS::active_output, memory_order_acquire))
		atomic_fetch_sub_explicit(&active_output, 1, memory_order_release);
	return result;
}

int64_t KERNELHOOKS::IOBluetoothDevice_moreIncomingData(void *that, void *arg1, unsigned int arg2)
{
	int attempts = max_attempts;
	while (atomic_load_explicit(&SMIMonitor::busy, memory_order_acquire) && --attempts > 0) IOSleep(0);
	atomic_fetch_add_explicit(&active_output, 1, memory_order_release);
	int64_t result = FunctionCast(IOBluetoothDevice_moreIncomingData,
							  callbackKERNELHOOKS->orgIOBluetoothDevice_moreIncomingData)(that, arg1, arg2);
	if (atomic_load_explicit(&KERNELHOOKS::active_output, memory_order_acquire))
		atomic_fetch_sub_explicit(&active_output, 1, memory_order_release);
	return result;
}

int64_t KERNELHOOKS::IOBluetoothL2CAPChannelUserClient_writeWL(void *that, void *arg1, uint16_t arg2, uint64_t arg3, uint64_t arg4)
{
	int attempts = max_attempts;
	while (atomic_load_explicit(&SMIMonitor::busy, memory_order_acquire) && --attempts > 0) IOSleep(0);
	atomic_fetch_add_explicit(&active_output, 1, memory_order_release);
	int64_t result = FunctionCast(IOBluetoothL2CAPChannelUserClient_writeWL,
							  callbackKERNELHOOKS->orgIOBluetoothL2CAPChannelUserClient_writeWL)(that, arg1, arg2, arg3, arg4);
	// called method will call finally IOBluetoothL2CAPChannelUserClient_callBackAfterDataIsSent (if result is KERN_SUCCESS)
	if (result != KERN_SUCCESS && atomic_load_explicit(&KERNELHOOKS::active_output, memory_order_acquire))
		atomic_fetch_sub_explicit(&active_output, 1, memory_order_release);
	return result;
}

int64_t KERNELHOOKS::IOBluetoothL2CAPChannelUserClient_writeOOLWL(void *that, uint64_t arg1, uint16_t arg2, uint64_t arg3, uint64_t arg4)
{
	int attempts = max_attempts;
	while (atomic_load_explicit(&SMIMonitor::busy, memory_order_acquire) && --attempts > 0) IOSleep(0);
	atomic_fetch_add_explicit(&active_output, 1, memory_order_release);
	int64_t result = FunctionCast(IOBluetoothL2CAPChannelUserClient_writeOOLWL,
							  callbackKERNELHOOKS->orgIOBluetoothL2CAPChannelUserClient_writeOOLWL)(that, arg1, arg2, arg3, arg4);
	// called method will call finally IOBluetoothL2CAPChannelUserClient_callBackAfterDataIsSent (if result is KERN_SUCCESS)
	if (result != KERN_SUCCESS && atomic_load_explicit(&KERNELHOOKS::active_output, memory_order_acquire))
		atomic_fetch_sub_explicit(&active_output, 1, memory_order_release);
	return result;
}

int64_t KERNELHOOKS::IOBluetoothL2CAPChannelUserClient_WriteAsyncAudioData_Trap(void *that, void *arg1, uint16_t arg2, uint64_t arg3, uint64_t arg4)
{
	int attempts = max_attempts;
	while (atomic_load_explicit(&SMIMonitor::busy, memory_order_acquire) && --attempts > 0) IOSleep(0);
	atomic_fetch_add_explicit(&active_output, 1, memory_order_release);

	int64_t result = FunctionCast(IOBluetoothL2CAPChannelUserClient_WriteAsyncAudioData_Trap,
							  callbackKERNELHOOKS->orgIOBluetoothL2CAPChannelUserClient_WriteAsyncAudioData_Trap)(that, arg1, arg2, arg3, arg4);
	// called method will call finally IOBluetoothL2CAPChannelUserClient_callBackAfterDataIsSent (if result is KERN_SUCCESS)
	if (result != KERN_SUCCESS && atomic_load_explicit(&KERNELHOOKS::active_output, memory_order_acquire))
		atomic_fetch_sub_explicit(&active_output, 1, memory_order_release);
	return result;
}

int64_t KERNELHOOKS::IOBluetoothL2CAPChannelUserClient_callBackAfterDataIsSent(void *that, IOService *service, void *channel, int arg1, uint64_t arg2, uint64_t arg3)
{
	int attempts = max_attempts;
	while (atomic_load_explicit(&SMIMonitor::busy, memory_order_acquire) && --attempts > 0) IOSleep(0);
	int64_t result = FunctionCast(IOBluetoothL2CAPChannelUserClient_callBackAfterDataIsSent,
								  callbackKERNELHOOKS->orgIOBluetoothL2CAPChannelUserClient_callBackAfterDataIsSent)(that, service, channel, arg1, arg2, arg3);
	if (atomic_load_explicit(&KERNELHOOKS::active_output, memory_order_acquire) == 1)
		activateTimer();
	else if (atomic_load_explicit(&KERNELHOOKS::active_output, memory_order_acquire))
		atomic_fetch_sub_explicit(&active_output, 1, memory_order_release);
	return result;
}

void KERNELHOOKS::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size)
{
	if (!eventTimer) {
		if (!workLoop)
			workLoop = IOWorkLoop::workLoop();

		if (workLoop) {
			eventTimer = IOTimerEventSource::timerEventSource(workLoop,
			[](OSObject *owner, IOTimerEventSource *) {
				while (atomic_load_explicit(&KERNELHOOKS::active_output, memory_order_acquire))
					atomic_fetch_sub_explicit(&active_output, 1, memory_order_release);
			});

			if (eventTimer) {
				IOReturn result = workLoop->addEventSource(eventTimer);
				if (result != kIOReturnSuccess) {
					SYSLOG("sdell", "KERNELHOOKS addEventSource failed");
					OSSafeReleaseNULL(eventTimer);
				}
			}
			else
				SYSLOG("sdell", "KERNELHOOKS timerEventSource failed");
		}
		else
			SYSLOG("sdell", "KERNELHOOKS IOService instance does not have workLoop");
	}

	if (!eventTimer || !workLoop)
		return;

	if (kextList[0].loadIndex == index) {
		DBGLOG("sdell", "%s found", kextList[0].id);
		PANIC_COND(progressState & ProcessingState::IOAudioFamilyRouted, "sdell", "IOAudioFamily is already routed");

		KernelPatcher::RouteRequest requests[] {
			{"__ZN23IOAudioEngineUserClient19performClientOutputEjjP22IOAudioClientBufferSetjj", IOAudioEngineUserClient_performClientOutput, orgIOAudioEngineUserClient_performClientOutput},
			{"__ZN23IOAudioEngineUserClient21performWatchdogOutputEP22IOAudioClientBufferSetj", IOAudioEngineUserClient_performWatchdogOutput, orgIOAudioEngineUserClient_performWatchdogOutput},
			{"__ZN23IOAudioEngineUserClient18performClientInputEjP22IOAudioClientBufferSet", IOAudioEngineUserClient_performClientInput, orgIOAudioEngineUserClient_performClientInput},
			{"__ZN13IOAudioStream20processOutputSamplesEP19IOAudioClientBufferjjb", IOAudioStream_processOutputSamples, orgIOAudioStream_processOutputSamples}
		};
		patcher.routeMultiple(index, requests, address, size);
		if (patcher.getError() == KernelPatcher::Error::NoError) {
			DBGLOG("sdell", "%s routed is successful", kextList[0].id);
		} else {
			SYSLOG("sdell", "failed to resolve at least one of symbols in %s, error = %d", kextList[0].id, patcher.getError());
			patcher.clearError();
		}
		
		progressState |= ProcessingState::IOAudioFamilyRouted;
	}

	if (kextList[1].loadIndex == index) {
		DBGLOG("sdell", "%s found", kextList[1].id);
		PANIC_COND(progressState & ProcessingState::IOBluetoothFamilyRouted, "sdell", "IOBluetoothFamily is already routed");

		KernelPatcher::RouteRequest requests[] {
			{"__ZN17IOBluetoothDevice16moreIncomingDataEPvj", IOBluetoothDevice_moreIncomingData, orgIOBluetoothDevice_moreIncomingData},
			{"__ZN33IOBluetoothL2CAPChannelUserClient7writeWLEPvtyy", IOBluetoothL2CAPChannelUserClient_writeWL, orgIOBluetoothL2CAPChannelUserClient_writeWL},
			{"__ZN33IOBluetoothL2CAPChannelUserClient10writeOOLWLEytyy", IOBluetoothL2CAPChannelUserClient_writeOOLWL, orgIOBluetoothL2CAPChannelUserClient_writeOOLWL},
			{"__ZN33IOBluetoothL2CAPChannelUserClient24WriteAsyncAudioData_TrapEPvtyy", IOBluetoothL2CAPChannelUserClient_WriteAsyncAudioData_Trap, orgIOBluetoothL2CAPChannelUserClient_WriteAsyncAudioData_Trap},
			{"__ZN33IOBluetoothL2CAPChannelUserClient23callBackAfterDataIsSentEP9IOServiceP23IOBluetoothL2CAPChanneliyy", IOBluetoothL2CAPChannelUserClient_callBackAfterDataIsSent, orgIOBluetoothL2CAPChannelUserClient_callBackAfterDataIsSent}
		};
		patcher.routeMultiple(index, requests, address, size);
		if (patcher.getError() == KernelPatcher::Error::NoError) {
			DBGLOG("sdell", "%s routed is successful", kextList[1].id);
		} else {
			SYSLOG("sdell", "failed to resolve at least one of symbols in %s, error = %d", kextList[1].id, patcher.getError());
			patcher.clearError();
		}
		
		progressState |= ProcessingState::IOBluetoothFamilyRouted;
	}
}
