//
//  rtcread.c
//  rtcread
//
//  Copyright © 2018 vit9696. All rights reserved.
//

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <IOKit/IOKitLib.h>

enum AppleRTCMethods {
	// Used for reading from RTC
	AppleRTC_ReadBytes     = 0,
	// Used for writing to RTC (requires root access), automatically updates CRC
	AppleRTC_WriteBytes    = 1,
	// Used for writing IOHibernateSMCVariables to RTC (requires com.apple.private.securityd.stash entitlement)
	AppleRTC_WriteKeyStash = 2
};

// Relevant binaries:
// /usr/bin/pmset
// /usr/libexec/efiupdater
// /usr/sbin/bless - oss (https://opensource.apple.com/tarballs/bless/)

#define RTC_ADDRESS_SECONDS            0x00  // R/W  Range 0..59
#define RTC_ADDRESS_SECONDS_ALARM      0x01  // R/W  Range 0..59
#define RTC_ADDRESS_MINUTES            0x02  // R/W  Range 0..59
#define RTC_ADDRESS_MINUTES_ALARM      0x03  // R/W  Range 0..59
#define RTC_ADDRESS_HOURS              0x04  // R/W  Range 1..12 or 0..23 Bit 7 is AM/PM
#define RTC_ADDRESS_HOURS_ALARM        0x05  // R/W  Range 1..12 or 0..23 Bit 7 is AM/PM
#define RTC_ADDRESS_DAY_OF_THE_WEEK    0x06  // R/W  Range 1..7
#define RTC_ADDRESS_DAY_OF_THE_MONTH   0x07  // R/W  Range 1..31
#define RTC_ADDRESS_MONTH              0x08  // R/W  Range 1..12
#define RTC_ADDRESS_YEAR               0x09  // R/W  Range 0..99
#define RTC_ADDRESS_REGISTER_A         0x0A  // R/W[0..6]  R0[7]
#define RTC_ADDRESS_REGISTER_B         0x0B  // R/W
#define RTC_ADDRESS_REGISTER_C         0x0C  // RO
#define RTC_ADDRESS_REGISTER_D         0x0D  // RO

// May be set to 01 FE /usr/libexec/efiupdater, which is DefaultBackgroundColor
#define APPLERTC_BG_COLOUR_ADDR1       0x30
#define APPLERTC_BG_COLOUR_ADDR2       0x31

#define APPLERTC_HASHED_ADDR           0x0E  // Checksum is calculated starting from this address.
#define APPLERTC_TOTAL_SIZE            0x100 // All Apple hardware cares about 256 bytes of RTC memory
#define APPLERTC_CHECKSUM_ADDR1        0x58
#define APPLERTC_CHECKSUM_ADDR2        0x59

#define APPLERTC_BOOT_STATUS_ADDR      0x5C // 0x5 - Exit Boot Services, 0x4 - recovery? (AppleBds.efi)

#define APPLERTC_HIBERNATION_KEY_ADDR  0x80 // Contents of IOHibernateRTCVariables
#define APPLERTC_HIBERNATION_KEY_LEN   0x2C // sizeof (AppleRTCHibernateVars)

// 0x00 - after memory check if AppleSmcIo and AppleRtcRam are present (AppleBds.efi)
// 0x02 - when blessing to recovery for EFI firmware update (closed source bless)
#define APPLERTC_BLESS_BOOT_TARGET     0xAC // used for boot overrides at Apple
#define APPLERTC_RECOVERYCHECK_STATUS  0xAF // 0x0 - at booting into recovery? connected to 0x5C

#define APPLERTC_POWER_BYTES_ADDR      0xB0 // used by /usr/bin/pmset
// Valid values could be found in stringForPMCode (https://opensource.apple.com/source/PowerManagement/PowerManagement-494.30.1/pmconfigd/PrivateLib.c.auto.html)
#define APPLERTC_POWER_BYTE_PM_ADDR    0xB4 // special PM byte describing Power State 
#define APPLERTC_POWER_BYTES_LEN       0x08

// Currently unused?
#define APPLERTC_HDD_UNWRAP_KEY_ADDR   0xD0 // Conents of IOHibernateSMCVariables (HBKP shadow copy)
#define APPLERTC_HDD_UNWRAP_KEY_LEN    0x20 // normally 16 bytes
#define APPLERTC_HDD_UNWRAP_STAT_ADDR  0xF0 // Set to 0x41 when HBKP should be updated through RTC via AppleBds.efi

io_connect_t rtc_connect(void) {
	io_connect_t con = 0;
	CFMutableDictionaryRef dict = IOServiceMatching("AppleRTC");
	if (dict) {
		io_iterator_t iterator = 0;
		kern_return_t result = IOServiceGetMatchingServices(kIOMasterPortDefault, dict, &iterator);
		if (result == kIOReturnSuccess && IOIteratorIsValid(iterator)) {
			io_object_t device = IOIteratorNext(iterator);
			if (device) {
				result = IOServiceOpen(device, mach_task_self(), 0, &con);
				if (result != kIOReturnSuccess)
					fprintf(stderr, "Unable to open AppleRTC service %08X\n", result);
				IOObjectRelease(device);
			} else {
				fprintf(stderr, "Unable to locate AppleRTC device\n");
			}
			IOObjectRelease(iterator);
		} else {
			fprintf(stderr, "Unable to get AppleRTC service %08X\n", result);
		}
	} else {
		fprintf(stderr, "Unable to create AppleRTC matching dict\n");
	}
	
	return con;
}

kern_return_t rtc_read(uint64_t offset, uint8_t *dst, size_t sz) {
	kern_return_t ret = KERN_FAILURE;
	io_connect_t con = rtc_connect();
	if (con) {
		ret = IOConnectCallMethod(con, AppleRTC_ReadBytes, &offset, 1, NULL, 0, NULL, NULL, dst, &sz);
		IOServiceClose(con);
	}
	return ret;
}

kern_return_t rtc_write(uint64_t offset, uint8_t *src, size_t sz) {
	kern_return_t ret = KERN_FAILURE;
	io_connect_t con = rtc_connect();
	if (con) {
		ret = IOConnectCallMethod(con, AppleRTC_WriteBytes, &offset, 1, src, sz, NULL, NULL, NULL, NULL);
		IOServiceClose(con);
	}
	return ret;
}

bool rtc_verify(void) {
	uint8_t rtc[APPLERTC_TOTAL_SIZE] = {};
	kern_return_t ret = rtc_read(0, rtc, sizeof(rtc));

	if (ret != KERN_SUCCESS) {
		fprintf(stderr, "AppleRTC I/O failure %X\n", ret);
		fprintf(stderr, "Make sure you have enough permissions, e.g. run with sudo\n");
		return false;
	}

	for (size_t i = 0; i < APPLERTC_TOTAL_SIZE; i+= 16) {
		printf("%02zX: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n", i,
			rtc[i+0], rtc[i+1], rtc[i+2], rtc[i+3], rtc[i+4], rtc[i+5], rtc[i+6], rtc[i+7],
			rtc[i+8], rtc[i+9], rtc[i+10], rtc[i+11], rtc[i+12], rtc[i+13], rtc[i+14], rtc[i+15]);
	}

	uint16_t checksum = 0;
	for (size_t i = APPLERTC_HASHED_ADDR; i < APPLERTC_TOTAL_SIZE; i++) {
		checksum ^= (i == APPLERTC_CHECKSUM_ADDR1 || i == APPLERTC_CHECKSUM_ADDR2) ? 0 : rtc[i];
		for (size_t j = 0; j < 7; j++) {
			bool even = checksum & 1;
			checksum = (checksum & 0xFFFE) >> 1;
			if (even) checksum ^= 0x2001;
		}
	}

	bool valid = (checksum >> 8) == rtc[APPLERTC_CHECKSUM_ADDR1] && 
		(checksum & 0xFF) == rtc[APPLERTC_CHECKSUM_ADDR2];
	printf ("Checksum is %02X %02X vs %02X %02X from rtc (%d)\n", checksum >> 8, checksum & 0xFF,
		rtc[APPLERTC_CHECKSUM_ADDR1], rtc[APPLERTC_CHECKSUM_ADDR2], valid);

	return valid;
}

int main(void) {
	return rtc_verify() ? 0 : -1;
}
