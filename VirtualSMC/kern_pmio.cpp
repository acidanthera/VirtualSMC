//
//  kern_pmio.cpp
//  VirtualSMC
//
//  Copyright © 2017 vit9696. All rights reserved.
//

#include <Headers/kern_util.hpp>

#include "kern_vsmc.hpp"
#include "kern_pmio.hpp"

const SMCInfo::Memory SMCProtocolPMIO::MemoryInfo[] {
	{0, SMCProtocolPMIO::WindowAllocSize, VM_PROT_READ}
};

const size_t SMCProtocolPMIO::MemoryInfoSize {arrsize(MemoryInfo)};

void SMCProtocolPMIO::resetDevice() {
	currentResult = SmcSuccess;
	currentStatus = SMC_STATUS_READY;
	currentCommand = 0;
	currentKey = 0;
	currentKeyIndex = 0;
	currentSize = 0;
	resetBuffer();
}

void SMCProtocolPMIO::resetBuffer() {
	bzero(dataBuffer, dataSize);
	dataIndex = dataSize = 0;
}

void SMCProtocolPMIO::loadValueInBuffer() {
	resetBuffer();
	const VirtualSMCValue *value {nullptr};
	currentResult = VirtualSMC::getKeystore()->readValueByName(currentKey, value);
	if (currentResult == SmcSuccess) {
		auto src = value->get(dataSize);
		if (dataSize == currentSize) {
			lilu_os_memcpy(dataBuffer, src, dataSize);
			return;
		}
		currentResult = SmcKeySizeMismatch;
	}
	
	dataSize = currentSize;
	bzero(dataBuffer, dataSize > SMC_MAX_DATA_SIZE ? SMC_MAX_DATA_SIZE : dataSize);
}

void SMCProtocolPMIO::loadKeyInBuffer() {
	resetBuffer();
	SMC_KEY key;
	dataSize = sizeof(SMC_KEY);
	currentResult = VirtualSMC::getKeystore()->readNameByIndex(currentKeyIndex, key);
	if (currentResult == SmcSuccess) {
		lilu_os_memcpy(dataBuffer, &key, dataSize);
	} else {
		bzero(dataBuffer, dataSize);
	}
}

void SMCProtocolPMIO::saveValueFromBuffer() {
	currentResult = VirtualSMC::getKeystore()->writeValueByName(currentKey, dataBuffer);
	resetBuffer();
}

void SMCProtocolPMIO::loadKeyInfoInBuffer() {
	resetBuffer();
	SMC_DATA_SIZE size;
	SMC_KEY_TYPE type;
	SMC_KEY_ATTRIBUTES attr;
	dataSize = sizeof(KeyInfo);
	currentResult = VirtualSMC::getKeystore()->getInfoByName(currentKey, size, type, attr);
	if (currentResult == SmcSuccess) {
		KeyInfo info {};
		info.type = type;
		info.size = size;
		info.attr = attr;
		lilu_os_memcpy(dataBuffer, &info, sizeof(info));
	} else {
		bzero(dataBuffer, dataSize);
	}
}

void SMCProtocolPMIO::writeData(uint8_t v) {
	// Any i/o removes got command flag
	currentStatus &= ~SMC_STATUS_GOT_COMMAND;
	
	// Ignore any further writing and hope for device reset by a cmd
	if (dataSize >= SMC_MAX_DATA_SIZE) {
		DBGTRACE("pmio", "writedata detected oob write %u to %u", dataSize, SMC_MAX_DATA_SIZE);
		//FIXME: find correct result
		currentResult = SmcSpuriousData;
		return;
	}
	
	// Simply ignore an unexpected write
	if (currentStatus != (SMC_STATUS_READY | SMC_STATUS_BUSY)) {
		DBGTRACE("pmio", "writedata invalid status %X", currentStatus);
		//FIXME: find correct result
		currentResult = SmcCommCollision;
		return;
	}
	
	dataBuffer[dataIndex++] = v;
	dataSize++;
	
	switch (currentCommand) {
		case SmcCmdReadValue:
			if (dataSize == sizeof(KeyValue)) {
				currentStatus |= SMC_STATUS_AWAITING_DATA;
				currentKey = reinterpret_cast<KeyValue *>(dataBuffer)->key;
				currentSize = reinterpret_cast<KeyValue *>(dataBuffer)->size;
				loadValueInBuffer();
			}
			break;
	
		case SmcCmdWriteValue:
			if (currentSize == dataSize) {
				currentStatus = SMC_STATUS_READY;
				saveValueFromBuffer();
			} else if (currentSize == 0 && dataSize == sizeof(KeyValue)) {
				currentKey = reinterpret_cast<KeyValue *>(dataBuffer)->key;
				currentSize = reinterpret_cast<KeyValue *>(dataBuffer)->size;
				resetBuffer();
			}
			break;
			
		case SmcCmdGetKeyFromIndex:
			if (dataSize == sizeof(SMC_KEY_INDEX)) {
				currentStatus |= SMC_STATUS_AWAITING_DATA;
				currentKeyIndex = OSSwapInt32(*reinterpret_cast<SMC_KEY_INDEX *>(dataBuffer));
				loadKeyInBuffer();
			}
			break;
			
		case SmcCmdGetKeyInfo:
			if (dataSize == sizeof(SMC_KEY)) {
				currentStatus |= SMC_STATUS_AWAITING_DATA;
				currentKey = reinterpret_cast<SMC_KEY *>(dataBuffer)[0];
				loadKeyInfoInBuffer();
			}
			break;
			
		case SmcCmdReset:
			if (dataSize == sizeof(SMC_RESET_MODE)) {
				currentStatus = SMC_STATUS_READY;
				currentResult = SmcSuccess;
				DBGLOG("pmio", "writedata reset cmd mode %02X", dataBuffer[0]);
			}
			break;
			
		default:
			// This should not be reachable
			PANIC("pmio", "writedata unimplemented cmd %04X", currentCommand);
	}
}

void SMCProtocolPMIO::writeCommand(SMC_COMMAND c) {
	// This is a fixup for mmio interrupts
	if (currentStatus == SMC_STATUS_KEY_DONE) {
		currentStatus = SMC_STATUS_READY;
	}
	
	if (currentStatus != SMC_STATUS_READY) {
		SYSLOG("pmio", "writecmd got unexpected state %02X for cmd %02X, resetting", currentStatus, c);
		// According to AppleSMC::smcSMCInABadState SMC could recover itself by a cmd in busy mode.
		// Apple uses SmcCmdReadValue here, but I think it might be anything.
		resetDevice();
		//FIXME: find correct result
		currentResult = SmcCommCollision;
		return;
	}
	
	resetDevice();
	switch (c) {
		case SmcCmdReadValue:
		case SmcCmdWriteValue:
		case SmcCmdGetKeyInfo:
		case SmcCmdGetKeyFromIndex:
		case SmcCmdReset:
			currentStatus |= SMC_STATUS_BUSY | SMC_STATUS_GOT_COMMAND;
			currentCommand = c;
			break;
		default:
			SYSLOG("pmio", "writecmd got unsupported cmd %02X", c);
			currentResult = SmcBadCommand;
	}
	
}

uint8_t SMCProtocolPMIO::readData() {
	// Any i/o removes got command flag
	currentStatus &= ~SMC_STATUS_GOT_COMMAND;
	
	if (dataIndex >= dataSize) {
		// Ignore any further reading and hope for device reset by a cmd
		DBGTRACE("pmio", "readdata detected oob read %u from %u", dataIndex, dataSize);
		//FIXME: find correct result
		currentResult = SmcSpuriousData;
		return 0;
	}
	
	uint8_t r = dataBuffer[dataIndex++];
	
	// Simply ignore an unexpected read
	if (currentStatus != (SMC_STATUS_READY | SMC_STATUS_AWAITING_DATA | SMC_STATUS_BUSY)) {
		DBGTRACE("pmio", "readdata invalid status %X", currentStatus);
		//FIXME: find correct result
		currentResult = SmcCommCollision;
		return 0;
	}
	
	switch (currentCommand) {
		case SmcCmdReadValue:
		case SmcCmdGetKeyFromIndex:
		case SmcCmdGetKeyInfo:
			if (dataIndex == dataSize)
				currentStatus = SMC_STATUS_READY;
			break;
			
		default:
			// This should not be reachable
			PANIC("pmio", "readdata unimplemented cmd %04X", currentCommand);
	}
	
	return r;
}

SMC_STATUS SMCProtocolPMIO::readStatus() {
	return currentStatus;
}

SMC_RESULT SMCProtocolPMIO::readResult() {
	return currentResult;
}

void SMCProtocolPMIO::setInterrupt(SMC_EVENT_CODE code, const void *data, size_t dataSize) {
	//TODO: properly implement PMIO interrupts
	currentStatus = SMC_STATUS_READY;
}
