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

#ifndef RE_APPLE_APPLESMARTBATTERYCOMMANDS_H
#define RE_APPLE_APPLESMARTBATTERYCOMMANDS_H

/* SMBus Device addresses */
enum {
	kSMBusAppleDoublerAddr                  = 0xd,
	kSMBusBatteryAddr                       = 0xb,
	kSMBusManagerAddr                       = 0xa,
	kSMBusChargerAddr                       = 0x9
};

/*  System Manager Commands                             */
/*  Smart Battery System Manager spec - rev 1.1         */
/*  Section 5 SBSM Interface                            */
enum {
	kMStateCmd                              = 0x01,
	kMStateContCmd                          = 0x02
};

/*  SBSM BatterySystemState bitfields                   */
/*  Smart Battery System Manager spec - rev 1.1         */
/*  Section 5.1                                         */
enum {
	kMChargingBatt_A_Bit                    = 0x0010,
	kMPresentBatt_A_Bit                     = 0x0001
};

/*  SBSM BatterySystemStateCont bitfields               */
/*  Smart Battery System Manager spec - rev 1.1         */
/*  Section 5.2                                         */
enum {
	kMACPresentBit                          = 0x0001,
	kMPowerNotGoodBit                       = 0x0002,
	kMCalibrateBit                          = 0x0040,
};

/*  Smart Battery Commands                              */
/*  Smart Battery Data Specification - rev 1.1          */
/*  Section 5.1 SMBus Host to Smart Battery Messages    */

/* kBManufactureDateCmd
 * Date is published in a bitfield per the Smart Battery Data spec rev 1.1
 * in section 5.1.26
 *   Bits 0...4 => day (value 1-31; 5 bits)
 *   Bits 5...8 => month (value 1-12; 4 bits)
 *   Bits 9...15 => years since 1980 (value 0-127; 7 bits)
 *
 * Custom commands not specific to 'Smart Battery Data spec' are defined
 * with lower 16-bits set to 0.
 */
enum {
	kBManufacturerAccessCmd           = 0x00,     // READ/WRITE WORD
	kBRemainingCapacityAlarmCmd       = 0x01,     // READ/WRITE WORD, not used by Apple
	kBRemainingTimeAlarmCmd           = 0x02,     // READ/WRITE WORD, not used by Apple
	kBTemperatureCmd                  = 0x08,     // READ WORD
	kBVoltageCmd                      = 0x09,     // READ WORD aka B0AV
	kBCurrentCmd                      = 0x0a,     // READ WORD aka B0AC
	kBAverageCurrentCmd               = 0x0b,     // READ WORD
	kBMaxErrorCmd                     = 0x0c,     // READ WORD
	kBRemainingCapacityCmd            = 0x0f,     // READ WORD aka B0RM
	kBFullChargeCapacityCmd           = 0x10,     // READ WORD aka B0FC
	kBRunTimeToEmptyCmd               = 0x11,     // READ WORD
	kBAverageTimeToEmptyCmd           = 0x12,     // READ WORD
	kBAverageTimeToFullCmd            = 0x13,     // READ WORD aka B0TF
	kBChargingCurrentCmd              = 0x14,     // READ/WRITE WORD
	kBBatteryStatusCmd                = 0x16,     // READ WORD aka B0St
	kBCycleCountCmd                   = 0x17,     // READ WORD aka B0CT
	kBDesignCapacityCmd               = 0x18,     // READ WORD
	kBManufactureDateCmd              = 0x1b,     // READ WORD
	kBSerialNumberCmd                 = 0x1c,     // READ WORD
	kBManufacturerNameCmd             = 0x20,     // READ BLOCK
	kBDeviceNameCmd                   = 0x21,     // READ BLOCK
	kBManufacturerDataCmd             = 0x23,     // READ BLOCK
	kBReadCellVoltage4Cmd             = 0x3c,     // READ WORD aka NOOP, should be BC4V (bug?)
	kBReadCellVoltage3Cmd             = 0x3d,     // READ WORD BC3V
	kBReadCellVoltage2Cmd             = 0x3e,     // READ WORD BC2V
	kBReadCellVoltage1Cmd             = 0x3f,     // READ WORD BC1V
	kBManufacturerInfoCmd             = 0x70,     // READ BLOCK
	kBDesignCycleCount9CCmd           = 0x9C,     // READ WORD
	kBPackReserveCmd                  = 0x8B,     // READ WORD
};

/*  Smart Battery Extended Registers                             */
/*  bq20z90-V110 + bq29330 Chipset Technical Reference Manual    */
/*  TI Literature SLUU264                                        */
enum {
	kBExtendedPFStatusCmd             = 0x53,     // READ WORD aka B0PS
	kBExtendedOperationStatusCmd      = 0x54      // READ WORD aka B0OS
};

/* Apple Hardware Serial Number */
enum {
	kBAppleHardwareSerialCmd          = 0x76      // READ BLOCK
};

/*  Smart Battery Status Message Bits                   */
/*  Smart Battery Data Specification - rev 1.1          */
/*  Section 5.4 page 42                                 */
enum {
	kBOverChargedAlarmBit             = 0x8000,
	kBTerminateChargeAlarmBit         = 0x4000,
	kBOverTempAlarmBit                = 0x1000,
	kBTerminateDischargeAlarmBit      = 0x0800,
	kBRemainingCapacityAlarmBit       = 0x0200,
	kBRemainingTimeAlarmBit           = 0x0100,
	kBInitializedStatusBit            = 0x0080,
	kBDischargingStatusBit            = 0x0040,
	kBFullyChargedStatusBit           = 0x0020,
	kBFullyDischargedStatusBit        = 0x0010
};

/* message command for battery status refresh */
/* word in big endian, second byte respresents battery presence */
enum {
	kBMessageStatusCmd = 0x0A
};

/* default value used for kBReadCellVoltage1Cmd and keys BC1V/BC2V/BC3V */
static constexpr uint16_t defaultBatteryCellVoltage = 16000;

#endif /* RE_APPLE_APPLESMARTBATTERYCOMMANDS_H */
