//
//  kern_keys.cpp
//  VirtualSMC
//
//  Copyright © 2017 vit9696. All rights reserved.
//

#include <Headers/kern_crypto.hpp>
#include <Headers/kern_rtc.hpp>
#include <Headers/kern_time.hpp>
#include <architecture/i386/pio.h>

#include "kern_keys.hpp"
#include "kern_vsmc.hpp"
#include "kern_efiend.hpp"

VirtualSMCValueVariable *VirtualSMCValueVariable::withDictionary(OSDictionary *dict) {
	auto var = new VirtualSMCValueVariable;
	if (var) {
		if (var->init(dict)) {
			return var;
		} else {
			delete var;
			DBGLOG("var", "unable to initialise");
		}
	} else {
		DBGLOG("var", "unable to allocate");
	}
	
	return nullptr;
}

VirtualSMCValueVariable *VirtualSMCValueVariable::withData(const SMC_DATA *data, SMC_DATA_SIZE size, SMC_KEY_TYPE type, SMC_KEY_ATTRIBUTES attr, SerializeLevel serialize) {
	auto var = new VirtualSMCValueVariable;
	if (var) {
		if (var->init(data, size, type, attr, serialize)) {
			return var;
		} else {
			delete var;
			DBGLOG("var", "unable to initialise with dict");
		}
	} else {
		DBGLOG("var", "unable to allocate");
	}
	
	return nullptr;
}

SMC_RESULT VirtualSMCValueKEY::readAccess() {
	*reinterpret_cast<uint32_t *>(data) = OSSwapInt32(kstore->getPublicKeyAmount());
	return SmcSuccess;
}

VirtualSMCValueKEY *VirtualSMCValueKEY::withStore(VirtualSMCKeystore *store) {
	auto keys = new VirtualSMCValueKEY;
	if (keys) {
		keys->kstore = store;
		if (keys->init(nullptr, sizeof(uint32_t), SmcKeyTypeUint32, SMC_KEY_ATTRIBUTE_CONST|SMC_KEY_ATTRIBUTE_READ)) {
			return keys;
		} else {
			delete keys;
			DBGLOG("keys", "unable to initialise");
		}
	} else {
		DBGLOG("keys", "unable to allocate");
	}
	
	return nullptr;
}

int32_t VirtualSMCValueCLKT::readTime() {
	// Starting with 11.0 there is a lock in getGMTTimeOfDay() implementation.
	// Since we are not allowed to lock in VSMC, I wanted to implement it via a threaded
	// callback. However, for some reason most Z270+ RTC devices report the same value
	// and cause very high CPU load due to stalling within the call. Just read manually
	// for now...

	auto read = [](uint32_t reg) -> uint32_t {
		outb(RTCStorage::R_PCH_RTC_INDEX, reg);
		return inb(RTCStorage::R_PCH_RTC_TARGET);
	};

	auto bcd2dec = [](uint32_t value) -> uint32_t {
		return (value & 0xFU) + (value >> 4U) * 10;
	};

	auto sec = bcd2dec(read(RTCStorage::RTC_SEC));
	auto min = bcd2dec(read(RTCStorage::RTC_MIN));
	auto hour = bcd2dec(read(RTCStorage::RTC_HOUR));

	return static_cast<int32_t>(sec + min * 60 + hour * 60 * 60);
}

SMC_RESULT VirtualSMCValueCLKT::readAccess() {
	*reinterpret_cast<uint32_t *>(data) = OSSwapInt32((readTime() + delta + 86400) % 86400);
	return SmcSuccess;
}

SMC_RESULT VirtualSMCValueCLKT::update(const SMC_DATA *src) {
	delta = readTime() - OSSwapInt32(*reinterpret_cast<uint32_t *>(data));
	return SmcSuccess;
}

VirtualSMCValueCLKT *VirtualSMCValueCLKT::withDelta(int32_t d) {
	auto clkt = new VirtualSMCValueCLKT;
	if (clkt) {
		clkt->delta = d;
		if (clkt->init(nullptr, sizeof(uint32_t), SmcKeyTypeUint32, SMC_KEY_ATTRIBUTE_FUNCTION|SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_WRITE)) {
			return clkt;
		} else {
			delete clkt;
			DBGLOG("clkt", "unable to initialise");
		}
	} else {
		DBGLOG("clkt", "unable to allocate");
	}

	return nullptr;
}

SMC_RESULT VirtualSMCValueCLWK::readAccess() {
	uint64_t lastw = lastWake ? *lastWake : 0;
	if (lastw > 0 && lastw != lastWakeStored) {
		lastWakeStored = lastw;
		pending = false;
		halt = false;
	}

	if (!pending && !halt) {
		uint64_t spent = convertNsToMs(getTimeSinceNs(lastw));
		if (spent >= 0xFFFF)
			counter = 0xFFFF;
		else
			counter = spent;
	}

	*reinterpret_cast<uint16_t *>(data) = OSSwapInt16(counter);
	return SmcSuccess;
}

SMC_RESULT VirtualSMCValueCLWK::update(const SMC_DATA *src) {
	// Wait for a new wake on reset
	if (OSSwapInt16(*reinterpret_cast<const uint16_t *>(src)) == 1) {
		counter = 0;
		pending = true;
		halt = false;
	} else {
		halt = true;
	}

	return SmcSuccess;
}

VirtualSMCValueCLWK *VirtualSMCValueCLWK::withLastWake(uint64_t *lw) {
	auto clwk = new VirtualSMCValueCLWK;
	if (clwk) {
		clwk->pending = true;
		clwk->lastWake = lw;
		if (clwk->init(nullptr, sizeof(uint16_t), SmcKeyTypeUint16, SMC_KEY_ATTRIBUTE_FUNCTION|SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_WRITE)) {
			return clwk;
		} else {
			delete clwk;
			DBGLOG("clwk", "unable to initialise");
		}
	} else {
		DBGLOG("clwk", "unable to allocate");
	}

	return nullptr;
}

VirtualSMCValueKPST *VirtualSMCValueKPST::withUnlocked(bool value) {
	auto kpst = new VirtualSMCValueKPST;
	if (kpst) {
		SMC_DATA data[] {value};
		if (kpst->init(data, sizeof(data), SmcKeyTypeUint8, SMC_KEY_ATTRIBUTE_READ)) {
			return kpst;
		} else {
			delete kpst;
			DBGLOG("kpst", "unable to initialise");
		}
	} else {
		DBGLOG("kpst", "unable to allocate");
	}
	
	return nullptr;
}

bool VirtualSMCValueKPST::unlocked() const {
	return data[0] == 1;
}


void VirtualSMCValueKPST::setUnlocked(bool value) {
	data[0] = value;
}

SMC_RESULT VirtualSMCValueKPPW::update(const SMC_DATA *src) {
	if (generation == SMCInfo::Generation::V1)
		valueKPST->setUnlocked(!memcmp(src, PasswordV1, PasswordSizeV1));
	else
		valueKPST->setUnlocked(!memcmp(src, PasswordV2, PasswordSizeV2));
	return SmcSuccess;
}

VirtualSMCValueKPPW *VirtualSMCValueKPPW::withKPST(VirtualSMCValueKPST *kpst, SMCInfo::Generation gen) {
	if (kpst) {
		auto kppw = new VirtualSMCValueKPPW;
		if (kppw) {
			kppw->valueKPST = kpst;
			kppw->generation = gen;
			if (kppw->init(nullptr, gen == SMCInfo::Generation::V1 ? PasswordSizeV1 : PasswordSizeV2,
						   SmcKeyTypeCh8s, SMC_KEY_ATTRIBUTE_FUNCTION|SMC_KEY_ATTRIBUTE_WRITE)) {
				return kppw;
			} else {
				delete kppw;
				DBGLOG("kppw", "unable to initialise");
			}
		} else {
			DBGLOG("kppw", "unable to allocate");
		}
	} else {
		DBGLOG("kppw", "invalid kpst");
	}
	
	return nullptr;
}

VirtualSMCValueAdr *VirtualSMCValueAdr::withAddr(uint32_t addr) {
	auto adr = new VirtualSMCValueAdr;
	if (adr) {
		if (adr->init(nullptr, sizeof(addr), SmcKeyTypeUint32, SMC_KEY_ATTRIBUTE_CONST|SMC_KEY_ATTRIBUTE_READ)) {
			adr->setAddress(addr);
			return adr;
		} else {
			delete adr;
			DBGLOG("adr", "unable to initialise");
		}
	} else {
		DBGLOG("adr", "unable to allocate");
	}
	
	return nullptr;
}

void VirtualSMCValueAdr::setAddress(uint32_t addr) {
	*reinterpret_cast<uint32_t *>(data) = OSSwapInt32(addr);
}

SMC_RESULT VirtualSMCValueNum::readAccess() {
	data[0] = 1;
	return SmcSuccess;
}

SMC_RESULT VirtualSMCValueNum::update(const SMC_DATA *src) {
	if (src[0] == 0) {
		valueAdr->setAddress(VirtualSMCValueAdr::DefaultAddr);
		return SmcSuccess;
	}
	
	return SmcBadParameter;
}

VirtualSMCValueNum *VirtualSMCValueNum::withAdr(VirtualSMCValueAdr *adr, uint8_t index) {
	if (adr) {
		auto num = new VirtualSMCValueNum;
		if (num) {
			num->valueAdr = adr;
			num->deviceIndex = index;
			if (num->init(nullptr, sizeof(uint8_t), SmcKeyTypeUint8, SMC_KEY_ATTRIBUTE_FUNCTION|SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_WRITE)) {
				return num;
			} else {
				delete num;
				DBGLOG("num", "unable to initialise");
			}
		} else {
			DBGLOG("num", "unable to allocate");
		}
	} else {
		DBGLOG("num", "invalid adr");
	}
	
	return nullptr;
}

SMC_RESULT VirtualSMCValueLDLG::update(const SMC_DATA *src) {
	const char *message = nullptr;
	
	switch (src[0]) {
		case 1:
			message = "Su69965aa55a5a 567";
			break;
		case 2:
			message = "Error: No time for generic phrases";
			break;
		case 3:
			message = "0123456789112345678921234567893123456789412345678951234567896123456789712345678981234567899123456789A123456789B123456789C123456";
			break;
		default:
			if (src[0] == hiddenCode)
				message = hiddenMessage;
	}

	if (message) {
		VirtualSMC::postInterrupt(SmcEventLogMessage, message, static_cast<uint32_t>(strlen(message)+1));
		return SmcSuccess;
	}
	
	return SmcBadParameter;
}

VirtualSMCValueLDLG *VirtualSMCValueLDLG::withEasterEgg(uint8_t code, const char *message) {
	auto ldlg = new VirtualSMCValueLDLG;
	if (ldlg) {
		ldlg->hiddenCode = code;
		ldlg->hiddenMessage = message;
		if (ldlg->init(nullptr, sizeof(uint8_t), SmcKeyTypeCh8s, SMC_KEY_ATTRIBUTE_FUNCTION|SMC_KEY_ATTRIBUTE_WRITE)) {
			return ldlg;
		} else {
			delete ldlg;
			DBGLOG("ldlg", "unable to initialise");
		}
	} else {
		DBGLOG("ldlg", "unable to allocate");
	}
	
	return nullptr;
}

SMC_RESULT VirtualSMCValueHBKP::update(const SMC_DATA *src) {
	DBGLOG("hbkp", "updating hbkp with key %d", findNotEquals(src, SMC_HBKP_SIZE, 0));
	// Do not try to write real value to HBKP key for security reasons.
	if (dumpToNVRAM) {
		if (EfiBackend::submitEncryptionKey(src, dumpEncrypted))
			DBGLOG("hbkp", "writing hbkp success");
		else
			SYSLOG("hbkp", "failed to write hbkp");
	}
	return SmcSuccess;
}

VirtualSMCValueHBKP *VirtualSMCValueHBKP::withDump(bool dump) {
	auto hbkp = new VirtualSMCValueHBKP;
	if (hbkp) {
		if (hbkp->init(nullptr, SMC_HBKP_SIZE, SmcKeyTypeCh8s, SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_WRITE)) {
			// Dump to nvram can only be enabled when VirtualSMC EFI module exists and is functional.
			// However, this is not the only condition, another one is vsmchbkp argument.
			// Note, that we never initialise HBKP with a real key for security reasons.
			uint32_t val = DumpNormal;
			PE_parse_boot_argn("vsmchbkp", &val, sizeof(val));
			if (val >= DumpTotal)
				PANIC("hbkp", "vsmchbkp misuse, must be in 0..2 range");
			hbkp->dumpToNVRAM = dump && val >= DumpNormal;
			hbkp->dumpEncrypted = val == DumpNormal;
			// AppleRTC should be loaded by this time. Only erase when we are allowed to write to RTC.
			if (hbkp->dumpEncrypted)
				hbkp->dumpEncrypted = EfiBackend::eraseTempEncryptionKey();
			return hbkp;
		} else {
			delete hbkp;
			DBGLOG("hbkp", "unable to initialise");
		}
	} else {
		DBGLOG("hbkp", "unable to allocate");
	}

	return nullptr;
}

SMC_RESULT VirtualSMCValueNTOK::update(const SMC_DATA *src) {
	DBGLOG("ntok", "received new value %02X", src[0]);
	VirtualSMC::setInterrupts(src[0] == 1);
	return SmcSuccess;
}

VirtualSMCValueNTOK *VirtualSMCValueNTOK::withState(bool enable) {
	auto ntok = new VirtualSMCValueNTOK;
	if (ntok) {
		VirtualSMC::setInterrupts(enable);
		if (ntok->init(nullptr, sizeof(uint8_t), SmcKeyTypeUint8, SMC_KEY_ATTRIBUTE_FUNCTION|SMC_KEY_ATTRIBUTE_WRITE)) {
			return ntok;
		} else {
			delete ntok;
			DBGLOG("ntok", "unable to initialise");
		}
	} else {
		DBGLOG("ntok", "unable to allocate");
	}
	
	return nullptr;
}

SMC_RESULT VirtualSMCValueTimer::readAccess() {
	DBGLOG("nati/oswd", "read %04X", OSSwapInt16(*reinterpret_cast<const uint16_t *>(data)));
	if (jobStartTime > 0) {
		uint64_t timeout  = convertScToNs(OSSwapInt16(*reinterpret_cast<uint16_t *>(data)));
		uint64_t current  = getCurrentTimeNs();
		uint16_t timeleft = convertNsToSc(getTimeLeftNs(jobStartTime, timeout, current));
		if (timeleft > 0) {
			*reinterpret_cast<uint16_t *>(data) = OSSwapInt16(timeleft);
			jobStartTime = current;
		} else {
			*reinterpret_cast<uint16_t *>(data) = 0;
			jobStartTime = 0;
		}
	}

	return SmcSuccess;
}

uint16_t VirtualSMCValueTimer::startCountdown() {
	uint16_t timeout = OSSwapInt16(*reinterpret_cast<uint16_t *>(data));
	jobStartTime = timeout > 0 ? getCurrentTimeNs() : 0;
	return timeout;
}

SMC_RESULT VirtualSMCValueNATi::update(const SMC_DATA *src) {
	jobStartTime = 0;
	lilu_os_memcpy(data, src, sizeof(uint16_t));
	return SmcSuccess;
}

VirtualSMCValueNATi *VirtualSMCValueNATi::withCountdown(uint16_t countdown) {
	auto nati = new VirtualSMCValueNATi;
	if (nati) {
		if (nati->init(nullptr, sizeof(uint16_t), SmcKeyTypeUint16, SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_WRITE)) {
			*reinterpret_cast<uint16_t *>(nati->data) = OSSwapInt16(countdown);
			return nati;
		} else {
			delete nati;
			DBGLOG("nati", "unable to initialise");
		}
	} else {
		DBGLOG("nati", "unable to allocate");
	}
	
	return nullptr;
}

SMC_RESULT VirtualSMCValueNATJ::update(const SMC_DATA *src) {
	auto timeout = valueNATi->startCountdown();
	data[0] = src[0];
	DBGLOG("natj", "got job %02X with timer %04X", src[0], timeout);
	VirtualSMC::postWatchDogJob(src[0], convertScToMs(timeout));
	return SmcSuccess;
}

VirtualSMCValueNATJ *VirtualSMCValueNATJ::withNATi(VirtualSMCValueNATi *nati) {
	if (nati) {
		auto natj = new VirtualSMCValueNATJ;
		if (natj) {
			natj->valueNATi = nati;
			if (natj->init(nullptr, sizeof(uint8_t), SmcKeyTypeUint8, SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_WRITE)) {
				return natj;
			} else {
				delete natj;
				DBGLOG("natj", "unable to initialise");
			}
		} else {
			DBGLOG("natj", "unable to allocate");
		}
	} else {
		DBGLOG("natj", "invalid adr");
	}
	
	return nullptr;
}

SMC_RESULT VirtualSMCValueOSWD::update(const SMC_DATA *src) {
	lilu_os_memcpy(data, src, sizeof(uint16_t));
	uint16_t timeout = startCountdown();
	DBGLOG("oswd", "got reboot job with timer %04X", timeout);
	if (timeout > 0)
		VirtualSMC::postWatchDogJob(VirtualSMC::WatchDogForceRestart, convertScToMs(timeout));
	else
		VirtualSMC::postWatchDogJob(VirtualSMC::WatchDogDoNothing, 0);

	return SmcSuccess;
}

VirtualSMCValueOSWD *VirtualSMCValueOSWD::withCountdown(uint16_t countdown) {
	auto oswd = new VirtualSMCValueOSWD;
	if (oswd) {
		if (oswd->init(nullptr, sizeof(uint16_t), SmcKeyTypeUint16, SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_WRITE)) {
			*reinterpret_cast<uint16_t *>(oswd->data) = OSSwapInt16(countdown);
			return oswd;
		} else {
			delete oswd;
			DBGLOG("oswd", "unable to initialise");
		}
	} else {
		DBGLOG("oswd", "unable to allocate");
	}

	return nullptr;
}

VirtualSMCValueEVRD *VirtualSMCValueEVRD::withEvents(uint8_t *events) {
	auto evrd = new VirtualSMCValueEVRD;
	if (evrd) {
		if (evrd->init(events, KeySize, SmcKeyTypeCh8s, SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_FUNCTION)) {
			return evrd;
		} else {
			delete evrd;
			DBGLOG("evrd", "unable to initialise");
		}
	} else {
		DBGLOG("evrd", "unable to allocate");
	}

	return nullptr;
}

SMC_RESULT VirtualSMCValueEVCT::update(const SMC_DATA *) {
	//TODO: we should implement event support properly!
	return SmcNotWritable;
}

VirtualSMCValueEVCT *VirtualSMCValueEVCT::withBuffer(VirtualSMCValueEVRD *buffer) {
	auto evct = new VirtualSMCValueEVCT;
	if (evct) {
		if (evct->init(nullptr, sizeof(uint16_t), SmcKeyTypeHex, SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_WRITE|SMC_KEY_ATTRIBUTE_FUNCTION)) {
			evct->evrd = buffer;
			return evct;
		} else {
			delete evct;
			DBGLOG("evct", "unable to initialise");
		}
	} else {
		DBGLOG("evct", "unable to allocate");
	}

	return nullptr;
}

SMC_RESULT VirtualSMCValueEFBS::update(const SMC_DATA *) {
	//TODO: we should implement event support properly!
	return SmcNotWritable;
}

VirtualSMCValueEFBS *VirtualSMCValueEFBS::withBootStatus(uint8_t status) {
	auto efbs = new VirtualSMCValueEFBS;
	if (efbs) {
		if (efbs->init(&status, sizeof(uint8_t), SmcKeyTypeUint8, SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_WRITE|SMC_KEY_ATTRIBUTE_FUNCTION)) {
			return efbs;
		} else {
			delete efbs;
			DBGLOG("efbs", "unable to initialise");
		}
	} else {
		DBGLOG("efbs", "unable to allocate");
	}

	return nullptr;
}

SMC_RESULT VirtualSMCValueDUSR::update(const SMC_DATA *src) {
	// Is this key designed to freeze the os in some known state?
	// If so, that's the closest I can think of.
	while (1) asm volatile ("cli; hlt");
	return SmcSuccess;
}

VirtualSMCValueDUSR *VirtualSMCValueDUSR::create() {
	auto dusr = new VirtualSMCValueDUSR;
	if (dusr) {
		if (dusr->init(nullptr, sizeof(uint8_t), SmcKeyTypeUint8, SMC_KEY_ATTRIBUTE_WRITE)) {
			return dusr;
		} else {
			delete dusr;
			DBGLOG("dusr", "unable to initialise");
		}
	} else {
		DBGLOG("dusr", "unable to allocate");
	}

	return nullptr;
}

VirtualSMCValueOSK *VirtualSMCValueOSK::withIndex(uint8_t index) {
	if (index <= 1) {
		auto osk = new VirtualSMCValueOSK;
		if (osk) {
			// This logic is reversed from SMC firmware, no idea what exactly it does :)
			
			const uint8_t oskSourceA[KeySize*2]  {
				0xF0, 0xA0, 0x4B, 0xA3, 0xCA, 0x27, 0xD3, 0xC7, 0x7C, 0xD8, 0x33, 0xE7, 0x2A, 0x7F, 0xEE, 0xB7,
				0x12, 0xE1, 0x08, 0xFE, 0x51, 0x92, 0x28, 0x72, 0x26, 0x9D, 0x30, 0x27, 0x6D, 0x35, 0x8A, 0x61,
				0x61, 0x5A, 0x81, 0x00, 0x3F, 0x66, 0x69, 0xE3, 0x58, 0x99, 0x53, 0x9E, 0x83, 0xF7, 0x8F, 0x0E,
				0x36, 0xEB, 0xFC, 0x6A, 0xF2, 0x64, 0xBA, 0x8A, 0xE7, 0x76, 0xF9, 0x26, 0xFA, 0xF8, 0xB3, 0x18,
			};
			
			const uint8_t oskSourceB[KeySize*2] {
				0x9F, 0xD5, 0x39, 0xCB, 0xAB, 0x55, 0xB7, 0xB0, 0x13, 0xAA, 0x58, 0x85, 0x53, 0x0B, 0x86, 0xD2,
				0x61, 0x84, 0x7F, 0x91, 0x23, 0xF6, 0x5B, 0x15, 0x53, 0xFC, 0x42, 0x43, 0x08, 0x51, 0xFA, 0x0D,
				0x04, 0x3B, 0xF2, 0x65, 0x5B, 0x09, 0x07, 0x97, 0x2B, 0xED, 0x36, 0xFF, 0xEF, 0xDF, 0xEC, 0x27,
				0x77, 0x9B, 0x8C, 0x06, 0x97, 0x27, 0xD5, 0xE7, 0x97, 0x03, 0x8D, 0x43, 0x88, 0xB1, 0xDD, 0x7B
			};
			
			uint8_t data[KeySize];
			for (size_t i = 0; i < KeySize; i++)
				data[i] = oskSourceA[i + index*KeySize] ^ oskSourceB[i + index*KeySize];
			
			if (osk->init(data, KeySize, SmcKeyTypeCh8s, SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_FUNCTION)) {
				return osk;
			} else {
				delete osk;
				DBGLOG("osk", "unable to initialise with dict");
			}
		} else {
			DBGLOG("osk", "unable to allocate");
		}
	} else {
		DBGLOG("osk", "invalid index");
	}
	
	return nullptr;
}
