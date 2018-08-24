//
//  kern_mmio.hpp
//  VirtualSMC
//
//  Copyright © 2017 vit9696. All rights reserved.
//

#ifndef kern_mmio_hpp
#define kern_mmio_hpp

#include <mach/vm_types.h>
#include <VirtualSMCSDK/AppleSmcBridge.hpp>
#include <VirtualSMCSDK/kern_smcinfo.hpp>

class SMCProtocolMMIO {
	/**
	 *  Key info
	 */
	struct PACKED KeyInfo {
		SMC_KEY_TYPE type {};
		SMC_DATA unused {};
		SMC_DATA_SIZE size {};
		SMC_KEY_ATTRIBUTES attr {};
	};

	/**
	 *  Mapped device memory base
	 */
	mach_vm_address_t mmioBase {};
	
	/**
	 *  Device response if any
	 */
	SMC_RESULT currentResult {SmcSuccess};
	
	/**
	 *  Device status
	 */
	SMC_STATUS currentStatus {};
	
	/**
	 *  Device event status
	 */
	SMC_STATUS currentEventStatus {};
	
	/**
	 *  Device data buffer, contains prepared return data
	 *  All the values seem to fit 32 bytes
	 */
	SMC_DATA dataBuffer[SMC_MAX_DATA_SIZE] {};
	
	/**
	 *  Device data buffer size
	 */
	SMC_DATA_SIZE dataSize {};

	/**
	 *  Get typed pointer to the device memory area
	 *
	 *  @param T  pointer type
	 *  @param O  mmio offset
	 *
	 *  @return device memory pointer of type T at offset O
	 */
	template <typename T, uint32_t O>
	T *mmioPtr() {
		return reinterpret_cast<T *>(mmioBase + O);
	}

	/**
	 *  Read value from device memory area
	 *
	 *  @param T  pointer type
	 *  @param O  mmio offset
	 *
	 *  @return value read of type T at offset O
	 */
	template <typename T, uint32_t O>
	T mmioRead() {
		return *mmioPtr<T, O>();
	}

	/**
	 *  Write value to device memory area
	 *
	 *  @param T    pointer type
	 *  @param O    mmio offset
	 *  @param val  value to write
	 */
	template <typename T, uint32_t O>
	void mmioWrite(T val) {
		*mmioPtr<T, O>() = val;
	}

	/**
	 *  Send prepared data from dataBuffer to device memory and post an interrupt
	 */
	void submitData();

	/**
	 *  Handle SmcCmdReadValue command
	 */
	void readValue();

	/**
	 *  Handle SmcCmdWriteValue command
	 */
	void writeValue();

	/**
	 *  Handle SmcCmdGetKeyFromIndex command
	 */
	void getKeyFromIndex();

	/**
	 *  Handle SmcCmdGetKeyInfo command
	 */
	void getKeyInfo();

	/**
	 *  Handle SmcCmdReset command
	 */
	void reset();

public:
	
	/**
	 *  Protocol i/o area size
	 */
	static constexpr size_t WindowSize {SMC_MMIO_LENGTH};
	
	/**
	 *  Protocol i/o area alignment requirements
	 */
	static constexpr size_t WindowAlign {PAGE_SIZE};
	
	/**
	 *  Protocol i/o area allocation requirements
	 */
	static constexpr size_t WindowAllocSize {WindowSize};
	
	/**
	 *  Protocol i/o area size in pages
	 */
	static constexpr size_t WindowNPages {WindowAllocSize / WindowAlign};
	
	/**
	 *  Protocol i/o area region memory info rules
	 */
	static const SMCInfo::Memory MemoryInfo[];
	
	/**
	 *  Protocol i/o area region memory info rule entry num
	 */
	static const size_t MemoryInfoSize;

	/**
	 *  Perform the necessary actions (like status clearing) before memory read
	 *
	 *  @param base  mmio virtual region base address
	 *  @param addr  actual read address in mmio virtual region
	 */
	void handleRead(mach_vm_address_t base, mach_vm_address_t addr);

	/**
	 *  Perform the necessary actions (like data submission) after memory write
	 *
	 *  @param base  mmio virtual region base address
	 *  @param addr  actual read address in mmio virtual region
	 */
	void handleWrite(mach_vm_address_t base, mach_vm_address_t addr);
	
	/**
	 *  Prepare for interrupt handling
	 *
	 *  @param code     event code
	 *  @param data     event data (optional)
	 *  @param dataSize event data size (optional)
	 */
	void setInterrupt(SMC_EVENT_CODE code, const void *data, size_t dataSize);
};

#endif /* kern_mmio_hpp */
