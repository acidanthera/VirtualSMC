//
//  kern_efiend.hpp
//  VirtualSMC
//
//  Copyright © 2018 vit9696. All rights reserved.
//

#ifndef kern_efiend_h
#define kern_efiend_h

#include <Headers/kern_util.hpp>
#include <Headers/kern_nvram.hpp>
#include <Headers/kern_efi.hpp>

namespace EfiBackend {
	/**
	 *  MLB/ROM keys
	 */
	static constexpr const char *ROMKey {NVRAM_PREFIX(NVRAM_APPLE_VENDOR_GUID, "ROM")};
	static constexpr const char16_t *ROMKeyEfi {u"ROM"};
	static constexpr const char *MLBKey {NVRAM_PREFIX(NVRAM_APPLE_VENDOR_GUID, "MLB")};
	static constexpr const char16_t *MLBKeyEfi {u"MLB"};

	/**
	 *  Apple GUID accessor, move it to Lilu?
	 */
	inline const EFI_GUID *AppleVendorKeyGuid() {
		static EFI_GUID AppleGuid { 0x4D1EDE05, 0x38C7, 0x4A6A, { 0x9C, 0xC6, 0x4B, 0xCC, 0xA8, 0xB3, 0x8C, 0x14 } };
		return &AppleGuid;
	}

	/**
	 *  Firmware backend status key at read-only guid
	 */
	static constexpr const char *StatusKey {NVRAM_PREFIX(LILU_READ_ONLY_GUID, "vsmc-status")};
	static constexpr const char16_t *StatusKeyEfi {u"vsmc-status"};
	inline const EFI_GUID *StatusKeyGuid() {
		return &EfiRuntimeServices::LiluReadOnlyGuid;
	}

	/**
	 *  Firmware backend encryption key at write-only guid
	 */
	static constexpr const char *EncryptionKey {NVRAM_PREFIX(LILU_WRITE_ONLY_GUID, "vsmc-key")};
	static constexpr const char16_t *EncryptionKeyEfi {u"vsmc-key"};
	inline const EFI_GUID *EncryptionKeyGuid() {
		return &EfiRuntimeServices::LiluWriteOnlyGuid;
	}

	/**
	 *  Status info contained in nvram status variable
	 */
	struct PACKED StatusInfo {
		/**
		 *  Magic size
		 */
		static constexpr uint32_t MagicSize = 4;

		/**
		 *  Validity magic value
		 */
		static constexpr const char *StatusMagic = "VSMC";

		/**
		 *  Current supported revision
		 */
		static constexpr uint32_t StatusRevision = 1;

		/**
		 *  Validity magic
		 */
		char magic[MagicSize];

		/**
		 *  Revision
		 */
		uint32_t revision;

		/**
		 *  Timestamp validity threshold (15 mins) in seconds
		 */
		static constexpr uint64_t MaxDiff {15 * 60};

		/**
		 *  Check magic validity
		 *
		 *  @return true on success
		 */
		inline bool isValid() const {
			return !memcmp(magic, StatusMagic, MagicSize) && revision == StatusRevision;
		}
	};

	/**
	 *  Encryption info contained in nvram key variable
	 */
	struct PACKED EncryptionInfo {
		/**
		 *  Magic size
		 */
		static constexpr uint32_t MagicSize = 4;

		/**
		 *  Plain text magic
		 */
		static constexpr const char *MagicPlain = "VSPT";

		/**
		 *  Encrypted magic
		 */
		static constexpr const char *MagicEncrypted = "VSEN";

		/**
		 *  Validity magic (VSEN or VSPT)
		 */
		char magic[MagicSize];

		/**
		 *  Encryption data
		 */
		uint8_t data[];

		/**
		 *  Write encryption info
		 *
		 *  @param d           data to write
		 *  @param size        data size
		 *  @param encrypted   whether the data is encrypted
		 *
		 *  @return total size
		 */
		uint32_t write(const uint8_t *d, uint32_t size, bool encrypted) {
			lilu_os_memcpy(magic, encrypted ? MagicEncrypted : MagicPlain, MagicSize);
			lilu_os_memcpy(data, d, size);
			return MagicSize + size;
		}

		/**
		 *  Allocate encryption info of a given size
		 */
		static EncryptionInfo *create(uint32_t size) {
			return reinterpret_cast<EncryptionInfo *>(Buffer::create<uint8_t>(MagicSize + size));
		}

		/**
		 *  Deallocate created encryption info
		 */
		static void deleter(EncryptionInfo *info) {
			Buffer::deleter(reinterpret_cast<uint8_t *>(info));
		}
	};

	/**
	 *  RTC temporary encryption key offset
	 */
	static constexpr uint32_t RtcKeyOffset = 0xD0;

	/**
	 *  RTC temporary encryption key size
	 */
	static constexpr uint32_t RtcKeySize = 16;

	/**
	 *  Obtain firmware support availability and validity
	 *
	 *  @return true on success
	 */
	bool detectFirmwareBackend();

	/**
	 *  Submit HBKP key to NVRAM and RTC (if possible) for firmware usage.
	 *
	 *  @param key              HBKP raw value of at least SMC_HBKP_SIZE bytes
	 *  @param allowEncryption  try to encrypt the value and store a temp key in CMOS
	 *
	 *  @return true on success
	 */
	bool submitEncryptionKey(const uint8_t *key, bool allowEncryption);

	/**
	 *  Tries to erase RTC temporary encryption key if it is present.
	 *
	 *  @return true if can be erased (i.e. RTC memory is available)
	 */
	bool eraseTempEncryptionKey();

	/**
	 *  Tries to obtain MLB and ROM if present.
	 *
	 *  @param mlb      MLB destination buffer
	 *  @param mlbSize  MLB destination buffer size
	 *  @param rom      ROM destination buffer
	 *  @param romSize  ROM destination buffer size
	 *
	 */
	void readSerials(uint8_t *mlb, size_t mlbSize, uint8_t *rom, size_t romSize);
};

#endif /* kern_efiend_h */
