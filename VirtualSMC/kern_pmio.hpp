//
//  kern_pmio.hpp
//  VirtualSMC
//
//  Copyright © 2017 vit9696. All rights reserved.
//

#ifndef kern_pmio_hpp
#define kern_pmio_hpp

#include <VirtualSMCSDK/AppleSmcBridge.hpp>
#include <VirtualSMCSDK/kern_smcinfo.hpp>

class SMCProtocolPMIO {
	/**
	 *  Key info
	 */
	struct PACKED KeyInfo {
		SMC_DATA_SIZE size;
		SMC_KEY_TYPE type;
		SMC_KEY_ATTRIBUTES attr;
	};
	
	/**
	 *  Key with value len
	 */
	struct PACKED KeyValue {
		SMC_KEY key;
		SMC_DATA_SIZE size;
	};
	
	/**
	 *  Command being executed
	 */
	SMC_COMMAND currentCommand {};
	
	/**
	 *  Device status
	 */
	SMC_STATUS currentStatus {SMC_STATUS_READY};
	
	/**
	 *  Device response if any
	 */
	SMC_RESULT currentResult {SmcSuccess};
	
	/**
	 *  Key being worked with
	 */
	SMC_KEY currentKey {};
	
	/**
	 *  Key index being worked with
	 */
	SMC_KEY_INDEX currentKeyIndex {};
	
	/**
	 *  Value length being worked with
	 */
	SMC_DATA_SIZE currentSize {};
	
	/**
	 *  Device data buffer, contains any i/o information (keys, values, etc.)
	 *  All the values seem to fit 32 bytes
	 */
	SMC_DATA dataBuffer[SMC_MAX_DATA_SIZE] {};
	
	/**
	 *  Device data buffer position relevant for i/o
	 */
	SMC_DATA_SIZE dataIndex {};
	
	/**
	 *  Device data buffer size
	 */
	SMC_DATA_SIZE dataSize {};
	
	/**
	 *  Completely resets device state
	 */
	void resetDevice();
	
	/**
	 *  Resets device data buffer and erases it
	 */
	void resetBuffer();
	
	/**
	 *  Loads key value in data buffer and updates the status
	 */
	void loadValueInBuffer();
	
	/**
	 *  Saves key value from data buffer and updates the status
	 */
	void saveValueFromBuffer();
	
	/**
	 *  Loads key in data buffer by key index
	 */
	void loadKeyInBuffer();
	
	/**
	 *  Loads key info in data buffer
	 */
	void loadKeyInfoInBuffer();

public:
	
	/**
	 *  Protocol i/o area size
	 */
	static constexpr size_t WindowSize {SMC_PORT_LENGTH};
	
	/**
	 *  Protocol i/o area alignment requirements
	 */
	static constexpr size_t WindowAlign {PAGE_SIZE};
	
	/**
	 *  Protocol i/o area allocation requirements
	 */
	static constexpr size_t WindowAllocSize {PAGE_SIZE};
	
	/**
	 *  Protocol i/o area region memory info rules
	 */
	static const SMCInfo::Memory MemoryInfo[];
	
	/**
	 *  Protocol i/o area region memory info rule entry num
	 */
	static const size_t MemoryInfoSize;
	
	/**
	 *  Write operation on data port
	 *
	 *  @param v  value written
	 */
	void writeData(uint8_t v);
	
	/**
	 *  Write operation on command port
	 *
	 *  @param c  command written
	 */
	void writeCommand(SMC_COMMAND c);
	
	/**
	 *  Read operation on data port
	 *
	 *  @return value read
	 */
	uint8_t readData();
	
	/**
	 *  Read operation on status (command) port
	 *
	 *  @return device status
	 */
	SMC_STATUS readStatus();
	
	/**
	 *  Read operation on result port
	 *
	 *  @return device response
	 */
	SMC_RESULT readResult();
	
	/**
	 *  Prepare for interrupt handling
	 *
	 *  @param code     event code
	 *  @param data     event data (optional)
	 *  @param dataSize event data size (optional)
	 */
	void setInterrupt(SMC_EVENT_CODE code, const void *data, size_t dataSize);
};

#endif /* kern_pmio_hpp */
