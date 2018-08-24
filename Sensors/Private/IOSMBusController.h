/*
 * Copyright (c) 2018 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 *
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#ifndef RE_APPLE_IOSMBUSCONTROLLER_H
#define RE_APPLE_IOSMBUSCONTROLLER_H

// This header represents a mostly valid IOSMBusController class implementation useful for our needs.
// Compatible with macOS 10.13.5. Based on PureDarwin header:
// https://raw.githubusercontent.com/PureDarwin/PureDarwin/master/patches/PowerManagement-158.10.9.IOSmbusController.p1.patch

#include <Library/LegacyIOService.h>
#include <IOKit/IOCommand.h>
#include <IOKit/IOCommandGate.h>

// SMBus Status register type
typedef uint8_t IOSMBusStatus;

// Status Register (SMB_STS) Codes
// See section 12.9.1.1 of ACPI 4.0a specification
enum : IOSMBusStatus {
	kIOSMBusStatusOK                                    = 0x00,
	kIOSMBusStatusUnknownFailure                        = 0x07,
	kIOSMBusStatusDeviceAddressNotAcknowledged          = 0x10,
	kIOSMBusStatusDeviceError                           = 0x11,
	kIOSMBusStatusDeviceCommandAccessDenied             = 0x12,
	kIOSMBusStatusUnknownHostError                      = 0x13,
	kIOSMBusStatusDeviceAccessDenied                    = 0x17,
	kIOSMBusStatusTimeout                               = 0x18,
	kIOSMBusStatusHostUnsupportedProtocol               = 0x19,
	kIOSMBusStatusBusy                                  = 0x1A,
	kIOSMBusStatusPECError                              = 0x1F,
};

// Protocol Register (SMB_PRTCL) Types
// See section 12.9.1.2 of ACPI 4.0a specification
enum {
	kIOSMBusProtocolControllerNotInUse                  = 0x00,
	kIOSMBusProtocolReserved                            = 0x01,
	kIOSMBusProtocolWriteQuickCommand                   = 0x02,
	kIOSMBusProtocolReadQuickCommand                    = 0x03,
	kIOSMBusProtocolSendByte                            = 0x04,
	kIOSMBusProtocolReceiveByte                         = 0x05,
	kIOSMBusProtocolWriteByte                           = 0x06,
	kIOSMBusProtocolReadByte                            = 0x07,
	kIOSMBusProtocolWriteWord                           = 0x08,
	kIOSMBusProtocolReadWord                            = 0x09,
	kIOSMBusProtocolWriteBlock                          = 0x0A,
	kIOSMBusProtocolReadBlock                           = 0x0B,
	kIOSMBusProtocolProcessCall                         = 0x0C,
	kIOSMBusProtocolBlockWriteBlockReadProcessCall      = 0x0D,

	// Same as above but with (P)acket (E)rror (C)hecking enabled
	kIOSMBusProtocolPECControllerNotInUse               = 0x80,
	kIOSMBusProtocolPECReserved                         = 0x81,
	kIOSMBusProtocolPECWriteQuickCommand                = 0x82,
	kIOSMBusProtocolPECReadQuickCommand                 = 0x83,
	kIOSMBusProtocolPECSendByte                         = 0x84,
	kIOSMBusProtocolPECReceiveByte                      = 0x85,
	kIOSMBusProtocolPECWriteByte                        = 0x86,
	kIOSMBusProtocolPECReadByte                         = 0x87,
	kIOSMBusProtocolPECWriteWord                        = 0x88,
	kIOSMBusProtocolPECReadWord                         = 0x89,
	kIOSMBusProtocolPECWriteBlock                       = 0x8A,
	kIOSMBusProtocolPECReadBlock                        = 0x8B,
	kIOSMBusProtocolPECProcessCall                      = 0x8C,
	kIOSMBusProtocolPECBlockWriteBlockReadProcessCall   = 0x8D,
};

// SMBus alarm message type
enum {
	kIOMessageSMBusAlarm = iokit_family_msg(sub_iokit_smbus, 0x10),
};

static_assert(kIOMessageSMBusAlarm == 0xE002C010, "Invalid kIOMessageSMBusAlarm value");

// See section 12.9.1.7 of ACPI 4.0a specification
static constexpr size_t kSMBusAlarmDataSize = 2;

struct IOSMBusAlarmMessage {
	uint8_t fromAddress;
	uint8_t data[kSMBusAlarmDataSize];
};

// Maximum size of SMBus Data Register Array (SMB_DATA)
// See section 12.9.1.5 of ACPI 4.0a specification
static constexpr size_t kSMBusMaximumDataSize = 32;

// Holds data related to a SMBus transaction
struct IOSMBusTransaction {
	uint8_t  address;
	uint8_t  command;
	uint8_t  protocol;
	uint64_t timeout;
	uint32_t sendDataCount;
	uint8_t  sendData[kSMBusMaximumDataSize];
	IOSMBusStatus status;
	uint32_t receiveDataCount;
	uint8_t  receiveData[kSMBusMaximumDataSize];
};

static_assert(sizeof(IOSMBusTransaction) == 0x60, "Invalid IOSMBusTransaction size");
#if defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 5 || __GNUC__ > 3)
static_assert(offsetof(IOSMBusTransaction, timeout) == 0x08, "Invalid IOSMBusTransaction::timeout offset");
static_assert(offsetof(IOSMBusTransaction, sendDataCount) == 0x10, "Invalid IOSMBusTransaction::sendDataCount offset");
static_assert(offsetof(IOSMBusTransaction, sendData) == 0x14, "Invalid IOSMBusTransaction::sendData offset");
static_assert(offsetof(IOSMBusTransaction, receiveDataCount) == 0x38, "Invalid IOSMBusTransaction::receiveDataCount offset");
static_assert(offsetof(IOSMBusTransaction, receiveData) == 0x3C, "Invalid IOSMBusTransaction::receiveData offset");
#endif

// Transaction completion callback
typedef void (*IOSMBusTransactionCompletion)(OSObject *target, void *reference, IOSMBusTransaction *transaction);

// Transaction request for later execution.
//TODO: complete the fields.
class IOSMBusRequest : public IOCommand {
	OSDeclareDefaultStructors(IOSMBusRequest)
public:
	queue_chain_t requestChain;
	IOSMBusTransaction *transaction;
	IOSMBusTransactionCompletion completionHandler;
	OSObject *completionHandlerInstance;
	void *completionHandlerArgument;
	void *unknown[6];
};

static_assert(sizeof(IOSMBusRequest) == 0x80, "Invalid IOSMBusRequest size");

// Main SMBus controller base class.
class IOACPIPlatformDevice;
class IOSMBusController : public IOService {
	OSDeclareDefaultStructors(IOSMBusController)
public:
	virtual IOReturn performTransaction(IOSMBusTransaction *transaction, IOSMBusTransactionCompletion completion = 0,
										OSObject *target = 0, void *reference = 0);
	virtual IOSMBusRequest *createRequest();
	virtual IOSMBusStatus startRequest(IOSMBusRequest *request);
	virtual IOSMBusStatus completeRequest(IOSMBusRequest *request);
	virtual IOSMBusStatus sendAlarm(IOSMBusAlarmMessage *alarmMessage);

	OSMetaClassDeclareReservedUnused(IOSMBusController, 0);
	OSMetaClassDeclareReservedUnused(IOSMBusController, 1);
	OSMetaClassDeclareReservedUnused(IOSMBusController, 2);
	OSMetaClassDeclareReservedUnused(IOSMBusController, 3);
	OSMetaClassDeclareReservedUnused(IOSMBusController, 4);
	OSMetaClassDeclareReservedUnused(IOSMBusController, 5);
	OSMetaClassDeclareReservedUnused(IOSMBusController, 6);
	OSMetaClassDeclareReservedUnused(IOSMBusController, 7);
	OSMetaClassDeclareReservedUnused(IOSMBusController, 8);
	OSMetaClassDeclareReservedUnused(IOSMBusController, 9);
	OSMetaClassDeclareReservedUnused(IOSMBusController, 10);
	OSMetaClassDeclareReservedUnused(IOSMBusController, 11);
	OSMetaClassDeclareReservedUnused(IOSMBusController, 12);
	OSMetaClassDeclareReservedUnused(IOSMBusController, 13);
	OSMetaClassDeclareReservedUnused(IOSMBusController, 14);
	OSMetaClassDeclareReservedUnused(IOSMBusController, 15);

public:
	//FIXME: This is roughly unknown and incomplete.
	void *fPrivateUnknown;
	IOCommandGate *fPrivateCommandGate;
	queue_chain_t fPrivateCommandQueue;
	void *fPrivateUnknown2[5];
};

static_assert(sizeof(IOSMBusController) == 0xD0, "Invalid IOSMBusController size");
#if defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 5 || __GNUC__ > 3)
static_assert(offsetof(IOSMBusController, fPrivateCommandGate) == 0x90, "Invalid IOSMBusController::fCommandGate offset");
static_assert(offsetof(IOSMBusController, fPrivateCommandQueue) == 0x98, "Invalid IOSMBusController::fCommandQueue offset");
#endif

#endif /* RE_APPLE_IOSMBUSCONTROLLER_H */
