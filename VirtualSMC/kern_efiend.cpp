//
//  kern_efiend.cpp
//  VirtualSMC
//
//  Copyright © 2018 vit9696. All rights reserved.
//

#include "kern_efiend.hpp"

#include <Headers/kern_rtc.hpp>
#include <Headers/kern_crypto.hpp>
#include <VirtualSMCSDK/AppleSmcBridge.hpp>

bool EfiBackend::detectFirmwareBackend() {
	const StatusInfo *info = nullptr;
	uint8_t *infoBuf = nullptr;

	// Firstry try the os-provided NVStorage. If it is loaded, it is not safe to call EFI services.
	NVStorage storage;
	if (storage.init()) {
		uint32_t size = 0;
		infoBuf = storage.read(StatusKey, size, NVStorage::OptRaw);
		// Do not care if the value is a little bigger.
		if (infoBuf && size >= sizeof(StatusInfo)) {
			DBGLOG("efend", "found proper status via normal nvram");
			info = reinterpret_cast<StatusInfo *>(infoBuf);
		} else {
			SYSLOG("efend", "failed to find valid status, VirtualSMC EFI module is broken");
		}

		storage.deinit();
	} else {
		// Otherwise use EFI services if available.
		auto rt = EfiRuntimeServices::get(true);
		if (rt) {
			infoBuf = Buffer::create<uint8_t>(sizeof(StatusInfo));
			if (infoBuf) {
				memset(infoBuf, 0, sizeof(StatusInfo));
				uint64_t size = sizeof(StatusInfo);
				uint32_t attr = 0;
				auto status = rt->getVariable(StatusKeyEfi, StatusKeyGuid(), &attr, &size, infoBuf);

				// Retry with larger size if the value is somehow bigger
				if (status == EFI_ERROR64(EFI_BUFFER_TOO_SMALL)) {
					if (Buffer::resize(infoBuf, size))
						status = rt->getVariable(StatusKeyEfi, StatusKeyGuid(), &attr, &size, infoBuf);
					else
						SYSLOG("efend", "failed to resize vsmc-key tmp buffer to %lld", size);
				}

				// Do not care if vsmc-key is a little bigger.
				if (status == EFI_SUCCESS && size >= sizeof(StatusInfo)) {
					DBGLOG("efend", "found proper status via efi nvram");
					info = reinterpret_cast<StatusInfo *>(infoBuf);
				} else {
					SYSLOG("efend", "failed to find valid status (%llX, %lld), VirtualSMC EFI module is broken", status, size);
				}
			} else {
				SYSLOG("efend", "failed to allocate status tmp buffer");
			}

			rt->put();
		} else {
			SYSLOG("efend", "failed to load efi rt services for status");
		}
	}

	bool valid = info && info->isValid();

	if (infoBuf)
		Buffer::deleter(infoBuf);

	return valid;
}

bool EfiBackend::submitEncryptionKey(const uint8_t *key, bool allowEncryption) {
	uint32_t valueSize = 0;
	uint8_t *value = nullptr;

	if (allowEncryption) {
		static_assert(RtcKeySize == Crypto::BlockSize, "Incompatible encryption key");
		auto privkey = Crypto::genUniqueKey(RtcKeySize);

		valueSize = SMC_HBKP_SIZE;
		if (privkey) {
			// Updates valueSize with real size
			value = Crypto::encrypt(privkey, key, valueSize);

			if (value) {
				// We cannot use UserClient API here.
				// When hibernating with FV2 enabled this may cause shutdown hangs.
				// CRC is not updated as the EFI backend will break us anyway.
				if (RTCStorage::writeDirect(RtcKeyOffset, RtcKeySize, privkey, false, true)) {
					DBGLOG("efend", "successfully written privkey to rtc");
				} else {
					SYSLOG("efend", "failed to write privkey to rtc");
					Crypto::zeroMemory(valueSize, value);
					Buffer::deleter(value);
					value = nullptr;
				}
			} else {
				SYSLOG("efend", "value encryption failure");
			}

			Crypto::zeroMemory(RtcKeySize, privkey);
			Buffer::deleter(privkey);
		} else {
			SYSLOG("efend", "privkey generation failure");
		}
	}

	auto outInfo = EncryptionInfo::create(value ? valueSize : SMC_HBKP_SIZE);
	bool writtenValidValue = false;

	if (outInfo) {
		uint32_t totalSize = 0;
		if (value)
			totalSize = outInfo->write(value, valueSize, true);
		else
			totalSize = outInfo->write(key, SMC_HBKP_SIZE, false);

		NVStorage storage;
		if (storage.init()) {
			if (storage.write(EncryptionKey, reinterpret_cast<uint8_t *>(outInfo), totalSize, NVStorage::OptRaw|NVStorage::OptSensitive)) {
				if (storage.sync()) {
					DBGLOG("hbkp", "efend wrote encryption key to nvram");
					writtenValidValue = true;
				} else {
					SYSLOG("efend", "failed to flush encryption key to nvram");
				}
			} else {
				SYSLOG("efend", "failed to write encryption key to nvram");
			}
		} else {
			SYSLOG("efend", "failed to load nvram storage");
		}

		Crypto::zeroMemory(totalSize, outInfo);
		EncryptionInfo::deleter(outInfo);
	} else {
		SYSLOG("efend", "nv out buffer allocation failure");
	}

	if (value) {
		Crypto::zeroMemory(valueSize, value);
		Buffer::deleter(value);
	}

	return writtenValidValue;
}

bool EfiBackend::eraseTempEncryptionKey() {
	bool valid = false;
	RTCStorage rtc;
	if (rtc.init()) {
		if (rtc.checkExtendedMemory()) {
			valid = true;
			uint8_t buffer[RtcKeySize] {};
			// We do not know what memory does RTC interface use.
			// To avoid possible extra overwrites a read is performed first.
			if (rtc.read(RtcKeyOffset, RtcKeySize, buffer)) {
				if (findNotEquals(buffer, RtcKeySize, 0)) {
					Crypto::zeroMemory(RtcKeySize, buffer);
					if (rtc.write(RtcKeyOffset, RtcKeySize, buffer)) {
						DBGLOG("efend", "erased rtc key");
					} else {
						SYSLOG("efend", "failed to erase rtc key");
					}
				} else {
					DBGLOG("efend", "rtc key is null, skipping erase");
				}
			} else {
				SYSLOG("efend", "failed to read rtc key");
			}

			rtc.deinit();
		} else {
			SYSLOG("efend", "no extra rtc memory is present for key erase");
		}
	} else {
		SYSLOG("efend", "no rtc service for key erase");
	}

	return valid;
}

void EfiBackend::readSerials(uint8_t *mlb, size_t mlbSize, uint8_t *rom, size_t romSize) {
	NVStorage storage;
	if (storage.init()) {
		uint32_t size = 0;
		auto buf = storage.read(MLBKey, size, NVStorage::OptRaw);
		if (buf) {
			lilu_os_memcpy(mlb, buf, mlbSize > size ? size : mlbSize);
			Buffer::deleter(buf);
		}
		size = 0;
		buf = storage.read(ROMKey, size, NVStorage::OptRaw);
		if (buf) {
			lilu_os_memcpy(rom, buf, romSize > size ? size : romSize);
			Buffer::deleter(buf);
		}

		storage.deinit();
	} else {
		// Otherwise use EFI services if available.
		auto rt = EfiRuntimeServices::get(true);
		if (rt) {
			uint64_t size = mlbSize;
			uint32_t attr = 0;
			rt->getVariable(MLBKeyEfi, AppleVendorKeyGuid(), &attr, &size, mlb);

			size = romSize;
			attr = 0;
			rt->getVariable(ROMKeyEfi, AppleVendorKeyGuid(), &attr, &size, rom);

			rt->put();
		} else {
			SYSLOG("efend", "failed to load efi rt services for serials");
		}
	}
}
