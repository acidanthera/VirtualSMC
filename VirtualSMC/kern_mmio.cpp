//
//  kern_mmio.cpp
//  VirtualSMC
//
//  Copyright © 2017 vit9696. All rights reserved.
//

#include <Headers/kern_util.hpp>
#include <Headers/kern_mach.hpp>
#include <libkern/OSByteOrder.h>

#include "kern_vsmc.hpp"
#include "kern_mmio.hpp"

const SMCInfo::Memory SMCProtocolMMIO::MemoryInfo[] {
	{0x00000, 0x04000, VM_PROT_READ}, // Main area, read only
	{0x04000, 0x01000, VM_PROT_NONE}, // Status area, for catching read access
	{0x05000, 0x0B000, VM_PROT_READ}  // Rest unused area, read only
};

const size_t SMCProtocolMMIO::MemoryInfoSize {arrsize(MemoryInfo)};

static_assert(0x10000 == SMCProtocolMMIO::WindowAllocSize, "MMIO area has unexpected size");
static_assert(0x01000 == PAGE_SIZE, "PAGE_SIZE has unexpected size");

void SMCProtocolMMIO::submitData() {
	if (MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) == KERN_SUCCESS) {
		mmioWrite<SMC_KEY, SMC_MMIO_WRITE_KEY>(0);
		
		static_assert(SMC_MMIO_WRITE_INDEX == SMC_MMIO_WRITE_KEY, "Write Index is uncleared");
		mmioWrite<SMC_KEY_ATTRIBUTES, SMC_MMIO_WRITE_KEY_ATTRIBUTES>(0);
		
		mmioWrite<SMC_RESULT, SMC_MMIO_READ_RESULT>(currentResult);
		static_assert(SMC_MMIO_WRITE_COMMAND == SMC_MMIO_READ_RESULT, "Write Command is uncleared");
		
		mmioWrite<SMC_DATA_SIZE, SMC_MMIO_WRITE_DATA_SIZE>(dataSize);
		lilu_os_memcpy(mmioPtr<SMC_DATA, 0>(), dataBuffer, SMC_MAX_DATA_SIZE);
		
		MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
	}

	if (dataSize != 0) {
		bzero(dataBuffer, dataSize);
		dataSize = 0;
	}
	
	currentStatus = SMC_STATUS_KEY_DONE;
	VirtualSMC::postInterrupt(SmcEventKeyDone);
}

void SMCProtocolMMIO::handleRead(mach_vm_address_t base, mach_vm_address_t addr) {
	uint32_t off = static_cast<uint32_t>(addr-base);
	DBGLOG("mmio", "read access at %08X", off);

	if (off == SMC_MMIO_READ_KEY_STATUS) {
		mmioBase = base;
		
		mmioWrite<SMC_STATUS, SMC_MMIO_READ_KEY_STATUS>(currentStatus);
		currentStatus = 0;
	} else if (off == SMC_MMIO_READ_EVENT_STATUS) {
		mmioBase = base;
		
		auto code = VirtualSMC::getInterrupt();
		if (code == SmcEventKeyDone)
			currentStatus = 0;
		mmioWrite<SMC_STATUS, SMC_MMIO_READ_EVENT_STATUS>(currentEventStatus);
		currentEventStatus = 0;
	}
}

void SMCProtocolMMIO::handleWrite(mach_vm_address_t base, mach_vm_address_t addr) {
	uint32_t off = static_cast<uint32_t>(addr-base);
	DBGLOG("mmio", "write access at %08X", off);
	
	if (off == SMC_MMIO_WRITE_COMMAND) {
		mmioBase = base;
		
		auto cmd = mmioRead<SMC_COMMAND, SMC_MMIO_WRITE_COMMAND>();
		switch (cmd) {
			case SmcCmdReadValue:
				readValue();
				break;
			case SmcCmdWriteValue:
				writeValue();
				break;
			case SmcCmdGetKeyFromIndex:
				getKeyFromIndex();
				break;
			case SmcCmdGetKeyInfo:
				getKeyInfo();
				break;
			case SmcCmdReset:
				reset();
				break;
			default:
				SYSLOG("mmio", "io got unsupported cmd %02X", cmd);
				currentResult = SmcBadCommand;
		}
		
		submitData();
	}
}

void SMCProtocolMMIO::setInterrupt(SMC_EVENT_CODE code, const void *data, size_t size) {
	//TODO: Actually reverse AppleSMC::smcHandleInterruptEvent
	if (code == SmcEventLogMessage) {
		if (MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) == KERN_SUCCESS) {
			auto writtenSize = size > SMC_MAX_LOG_SIZE ? SMC_MAX_DATA_SIZE : size;
			lilu_os_memcpy(mmioPtr<SMC_LOG, SMC_MMIO_READ_LOG>(), data, writtenSize);
			if (writtenSize < SMC_MAX_LOG_SIZE)
				bzero(mmioPtr<SMC_LOG, SMC_MMIO_READ_LOG>() + writtenSize, SMC_MAX_LOG_SIZE-writtenSize);
			MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
		}
		currentEventStatus = SMC_STATUS_KEY_DONE;
	} else if (code == SmcEventKeyDone) {
		currentEventStatus = SMC_STATUS_KEY_DONE;
	} else if (code == SmcEventALSChange) {
		currentEventStatus = SMC_STATUS_KEY_DONE;
	} else {
		PANIC("mmio", "unsupported interrupt %u", code);
	}
}

void SMCProtocolMMIO::readValue() {
	auto key = mmioRead<SMC_KEY, SMC_MMIO_WRITE_KEY>();
	auto attr = mmioRead<SMC_KEY_ATTRIBUTES, SMC_MMIO_WRITE_KEY_ATTRIBUTES>();
	
	if (attr == 0) {
		const VirtualSMCValue *value {nullptr};
		currentResult = VirtualSMC::getKeystore()->readValueByName(key, value);
		if (currentResult == SmcSuccess) {
			auto src = value->get(dataSize);
			lilu_os_memcpy(dataBuffer, src, dataSize);
			return;
		}
	} else {
		DBGLOG("mmio", "read got non-zero attr %02X", attr);
		currentResult = SmcBadCommand;
	}
}

void SMCProtocolMMIO::writeValue() {
	auto key = mmioRead<SMC_KEY, SMC_MMIO_WRITE_KEY>();
	auto attr = mmioRead<SMC_KEY_ATTRIBUTES, SMC_MMIO_WRITE_KEY_ATTRIBUTES>();
	auto size = mmioRead<SMC_DATA_SIZE, SMC_MMIO_WRITE_DATA_SIZE>();
	
	if (attr == 0) {
		if (size <= SMC_MAX_DATA_SIZE)
			currentResult = VirtualSMC::getKeystore()->writeValueByName(key, mmioPtr<SMC_DATA, 0>());
		else
			currentResult = SmcKeySizeMismatch;
	} else {
		DBGLOG("mmio", "write got non-zero attr %02X", attr);
		currentResult = SmcBadCommand;
	}
}

void SMCProtocolMMIO::getKeyFromIndex() {
	auto index = mmioRead<SMC_KEY_INDEX, SMC_MMIO_WRITE_INDEX>();
	auto attr = mmioRead<SMC_KEY_ATTRIBUTES, SMC_MMIO_WRITE_KEY_ATTRIBUTES>();
	
	if (attr == 0) {
		SMC_KEY key;
		currentResult = VirtualSMC::getKeystore()->readNameByIndex(OSSwapInt32(index), key);
		if (currentResult == SmcSuccess) {
			dataSize = sizeof(SMC_KEY);
			lilu_os_memcpy(dataBuffer, &key, dataSize);
		}
	} else {
		DBGLOG("mmio", "getkey got non-zero attr %02X", attr);
		currentResult = SmcBadCommand;
	}
}

void SMCProtocolMMIO::getKeyInfo() {
	auto key = mmioRead<SMC_KEY, SMC_MMIO_WRITE_KEY>();
	auto attr = mmioRead<SMC_KEY_ATTRIBUTES, SMC_MMIO_WRITE_KEY_ATTRIBUTES>();
	
	if (attr == 0) {
		SMC_DATA_SIZE size;
		SMC_KEY_TYPE type;
		currentResult = VirtualSMC::getKeystore()->getInfoByName(key, size, type, attr);
		if (currentResult == SmcSuccess) {
			KeyInfo info {};
			info.type = type;
			info.size = size;
			info.attr = attr;
			lilu_os_memcpy(dataBuffer, &info, sizeof(info));
		}
	} else {
		DBGLOG("mmio", "getkeyinfo got non-zero attr %02X", attr);
		currentResult = SmcBadCommand;
	}
}

void SMCProtocolMMIO::reset() {
	DBGLOG("mmio", "reset %02X", mmioRead<SMC_RESET_MODE, SMC_MMIO_WRITE_MODE>());
	currentResult = SmcBadCommand;
}
