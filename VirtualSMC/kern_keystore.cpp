//
//  kern_keystore.cpp
//  VirtualSMC
//
//  Copyright © 2017 vit9696. All rights reserved.
//

#include <libkern/OSByteOrder.h>
#include <Headers/kern_atomic.hpp>
#include <Headers/kern_devinfo.hpp>
#include <Headers/kern_iokit.hpp>
#include <Headers/kern_util.hpp>
#include <Headers/kern_time.hpp>

#include "kern_keys.hpp"
#include "kern_keystore.hpp"

bool VirtualSMCKeystore::init(const OSDictionary *mainprops, const OSDictionary *userprops, const SMCInfo &info, const char *board, int model, bool whbkp) {
	deviceInfo = info;
	deviceInfo.generatorSeed();

	for (size_t i = 0; i < arrsize(pluginData); i++)
		atomic_init(&pluginData[i], nullptr);

	// Hibernation support
	if (!addKey(KeyHBKP, VirtualSMCValueHBKP::withDump(whbkp)))
		return false;

	if (!mergePredefined(board, model)) {
		DBGLOG("kstore", "unable to merge main properties");
		return false;
	}


	if (!mergeProvider(mainprops, board, model)) {
		DBGLOG("kstore", "unable to merge main properties");
		return false;
	}
	
	mergeProvider(userprops, board, model);

	qsort(const_cast<VirtualSMCKeyValue *>(dataStorage.data()), dataStorage.size(), sizeof(VirtualSMCKeyValue), VirtualSMCKeyValue::compare);
	qsort(const_cast<VirtualSMCKeyValue *>(dataHiddenStorage.data()), dataHiddenStorage.size(), sizeof(VirtualSMCKeyValue), VirtualSMCKeyValue::compare);

	if (!findAccessKeys()) {
		DBGLOG("kstore", "unable to find access keys");
		return false;
	}

	int tmp;
	if (PE_parse_boot_argn("-vsmcrpt", &tmp, sizeof(tmp)))
		reportMissingKeys = true;

	if (!PE_parse_boot_argn("vsmcslvl", &serLevel, sizeof(serLevel)) || serLevel > SerializeLevel::Confidential) {
		DBGLOG("kstore", "no serialiser lvl argument, using nromal");
		serLevel = SerializeLevel::Default;
	}

	if (serLevel != SerializeLevel::None) {
		//TODO: do deserialization here from vsmc-data nvram variable if we need it
	}

	return true;
}

bool VirtualSMCKeystore::mergeProvider(const OSDictionary *dict, const char *board, int model) {
	if (!dict) {
		DBGLOG("kstore", "empty merge provider");
		return false;
	}
	
	auto doMerge = [this, &dict](const char *name) {
		if (name) {
			auto entries = OSDynamicCast(OSArray, dict->getObject(name));
			if (entries) {
				DBGLOG("kstore", "merging properties from %s", name);
				if (!merge(entries))
					return false;
			}
		}
		return true;
	};
	
	if (!doMerge(board))
		return false;
	
	auto gen = deviceInfo.getGeneration();
	const char *spec = nullptr;
	if (model == WIOKit::ComputerModel::ComputerDesktop)
		spec = gen >= SMCInfo::Generation::V2 ? "GenericDesktopV2" : "GenericDesktopV1";
	else if (model == WIOKit::ComputerModel::ComputerLaptop)
		spec = gen >= SMCInfo::Generation::V2 ? "GenericLaptopV2" : "GenericLaptopV1";
	if (!doMerge(spec))
		return false;
	
	spec = gen >= SMCInfo::Generation::V2 ? "GenericV2" : "GenericV1";
	if (!doMerge(spec))
		return false;
	
	if (!doMerge("Generic"))
		return false;
	
	return true;
}

bool VirtualSMCKeystore::VirtualSMCKeystore::merge(const OSArray *arr) {
	if (!arr) {
		DBGLOG("kstore", "empty merge array");
		return false;
	}
	
	for (unsigned int i = 0; i < arr->getCount(); i++) {
		auto kvDict = OSDynamicCast(OSDictionary, arr->getObject(i));
		if (!kvDict) {
			DBGLOG("kstore", "non-dictionary entry in merge array");
			continue;
		}
		
		VirtualSMCKeyValue kv ;
		if (!WIOKit::getOSDataValue(kvDict, "name", kv.key) || kv.key == 0) {
			DBGLOG("kstore", "invalid key name at %u", i);
			continue;
		}
		
		atomic_init(&kv.value, VirtualSMCValueVariable::withDictionary(kvDict));
		atomic_init(&kv.backup, nullptr);
		if (!kv.value) {
			DBGLOG("kstore", "invalid value contents at %u", i);
			continue;
		}
		
		auto hiddenObj = OSDynamicCast(OSBoolean, kvDict->getObject("hidden"));
		bool hidden = hiddenObj && hiddenObj->isTrue();
		auto &storage = hidden ? dataHiddenStorage : dataStorage;
		if (storage.push_back<4>(kv)) {
			DBGLOG("kstore", "inserted key [%08X] (%d) at %u", kv.key, hidden, i);
		} else {
			DBGLOG("kstore", "failed to insert key [%08X] (%d) at %u", kv.key, hidden, i);
			delete kv.value;
		}
	}
	
	return true;
}

void VirtualSMCKeystore::handlePowerOff() {
	lastSleepTime = getCurrentTimeNs();

	//FIXME: This is when NVRAM serialization should happen.
}

void VirtualSMCKeystore::handlePowerOn() {
	lastWakeTime = getCurrentTimeNs();
}

IOReturn VirtualSMCKeystore::loadPlugin(VirtualSMCAPI::Plugin *plugin) {
	DBGLOG("kstore", "loading %s (%lu), api: %lu", plugin->product, plugin->version, plugin->apiver);

	if (plugin->apiver != VirtualSMCAPI::Version) {
		SYSLOG("kstore", "failed to load plugin %s (%lu), api %lu vs %lu", plugin->product, plugin->version, plugin->apiver, VirtualSMCAPI::Version);
		return kIOReturnInvalid;
	}

	IOReturn code = kIOReturnSuccess;
	VirtualSMCAPI::KeyStorage ovrData[2];
	VirtualSMCAPI::KeyStorage *pData[2] {&plugin->data, &plugin->dataHidden};
	VirtualSMCAPI::KeyStorage *sData[2] {&dataStorage, &dataHiddenStorage};
	for (size_t i = 0; code == kIOReturnSuccess && i < arrsize(ovrData); i++) {
		auto &currPData = *pData[i];
		auto &currSData = *sData[i];
		size_t j = 0;
		while (j < currPData.size()) {
			VirtualSMCKeyValue *tVal = nullptr;
			if (getByName(currSData, currPData[j].key, tVal) == SmcSuccess) {
				if (ovrData[i].push_back<2>(currPData[j])) {
					atomic_store_explicit(&currPData[j].value, nullptr, memory_order_relaxed);
					currPData.erase(j);
				} else {
					code = kIOReturnNoMemory;
					break;
				}
				continue;
			}
			j++;
		}
	}

	if (code == kIOReturnSuccess) {
		code = kIOReturnNoSpace;
		for (size_t i = 0; i < VirtualSMCAPI::PluginMax; i++) {
			VirtualSMCAPI::Plugin * nullData = nullptr;
			if (atomic_compare_exchange_strong_explicit(&pluginData[i], &nullData, plugin, memory_order_relaxed, memory_order_relaxed)) {
				code = kIOReturnSuccess;
				break;
			}
		}

		if (code == kIOReturnSuccess) {
			for (size_t i = 0; code == kIOReturnSuccess && i < arrsize(ovrData); i++) {
				auto &currOData = ovrData[i];
				auto &currSData = *sData[i];
				for (size_t j = 0; j < currOData.size(); j++) {
					VirtualSMCKeyValue *tVal = nullptr;
					if (getByName(currSData, currOData[j].key, tVal) == SmcSuccess) {
						// Obtain any current value (we will check if it changed later).
						VirtualSMCValue *orgValue = atomic_load_explicit(&tVal->value, memory_order_relaxed);
						// Overwrite it with the new value.
						atomic_store_explicit(&tVal->value, currOData[j].value, memory_order_relaxed);
						// Protect the replaced value from deletion (there are no data races here).
						atomic_store_explicit(&currOData[j].value, nullptr, memory_order_relaxed);
						// Synchronise access by checking for any extra overrides.
						// Note, a pointer could be lost in the first load & store pair due to a race-condition.
						// However, this is fine, because that is a contract violation which must lead to a kernel panic.
						VirtualSMCValue *nullValue = nullptr;
						if (!atomic_compare_exchange_strong_explicit(&currOData[j].backup, &nullValue, orgValue,
																	 memory_order_relaxed, memory_order_relaxed))
							PANIC("kstore", "attempt to override twice [%08X] by %s", currOData[j].key, plugin->product);
					} else {
						code = kIOReturnInvalid;
						break;
					}
				}
			}
		}
	}

	for (auto &entry : ovrData)
		entry.deinit();

	return code;
}

uint32_t VirtualSMCKeystore::getPublicKeyAmount() {
	auto sz = dataStorage.size();
	for (size_t i = 0; i < VirtualSMCAPI::PluginMax; i++) {
		auto p = atomic_load_explicit(&pluginData[i], memory_order_relaxed);
		if (!p) break;
		sz += p->data.size();
	}
	return static_cast<uint32_t>(sz);
}

struct PACKED SerializedDataHeader {
	static constexpr uint32_t Magic = 'SMC1';
	uint32_t magic;
	uint32_t size;
};

bool VirtualSMCKeystore::deserialize(const uint8_t *src, uint32_t size) {
	if (!src || sizeof(SerializedDataHeader) > size) {
		DBGLOG("kstore", "invalid buffer");
		return false;
	}
	
	auto header = reinterpret_cast<const SerializedDataHeader *>(src);
	if (header->magic != SerializedDataHeader::Magic) {
		DBGLOG("kstore", "unexpected keystore magic %08X", header->magic);
		return false;
	}
	
	src += sizeof(SerializedDataHeader);
	size -= sizeof(SerializedDataHeader);
	
	for (uint32_t i = 0; i < size; i++) {
		SMC_KEY name;
		SMC_DATA data[SMC_MAX_DATA_SIZE];
		SMC_DATA_SIZE dataSize;
		if (!VirtualSMCKeyValue::deserialize(src, size, name, data, dataSize)) {
			DBGLOG("kstore", "failed to deserialize %u key", i);
			return false;
		}
		
		VirtualSMCKeyValue *kv;
		if (getByName(name, kv, false) == SmcSuccess || getByName(name, kv, true) == SmcSuccess) {
			//TODO: design proper updating
		} else {
			//TODO: Support delayed merging
		}
	}
	
	return true;
}

uint8_t *VirtualSMCKeystore::serialize(size_t &size) const {
	auto serializedSize = [this](auto &data, uint32_t &count) {
		size_t ret = 0;
		for (size_t i = 0; i < data.size(); i++) {
			if (data[i].serializable(serLevel == SerializeLevel::Confidential)) {
				ret += data[i].serializedSize();
				count++;
			}
		}
		return ret;
	};
	
	auto serializeData = [this](uint8_t *&buf, auto &data) {
		for (size_t i = 0; i < data.size(); i++)
			if (data[i].serializable(serLevel == SerializeLevel::Confidential))
				data[i].serialize(buf);
	};
	
	uint32_t count = 0;
	size = sizeof(SerializedDataHeader) + serializedSize(dataStorage, count) + serializedSize(dataHiddenStorage, count);
	auto ret = Buffer::create<uint8_t>(size);
	if (!ret) {
		DBGLOG("kstore", "failed to allocate memory for serialization");
		size = 0;
		return nullptr;
	}
	
	auto header = reinterpret_cast<SerializedDataHeader *>(ret);
	header->magic = SerializedDataHeader::Magic;
	header->size = count;
	auto buf = ret + sizeof(SerializedDataHeader);
	
	serializeData(buf, dataStorage);
	serializeData(buf, dataHiddenStorage);
	
	return ret;
}

bool VirtualSMCKeystore::findAccessKeys() {
	VirtualSMCKeyValue *kvEPCI, *kvKPST;

	if (getByName(KeyEPCI, kvEPCI, false) == SmcSuccess &&
		getByName(KeyEPST, kvKPST, true) == SmcSuccess) {
		valueEPCI = kvEPCI->value;
		valueKPST = kvKPST->value;
		return true;
	}
	
	DBGLOG("kstore", "EPCI or KPST are not present in the keystore");
	return false;
}

bool VirtualSMCKeystore::mergePredefined(const char *board, int model) {
	bool nextGen = deviceInfo.getGeneration() >= SMCInfo::Generation::V2;
	
	// Generic feature keys

	if (!addKey(KeyKEY, VirtualSMCValueKEY::withStore(this)))
		return false;
	
	auto valueAdr = VirtualSMCValueAdr::withAddr();
	if (!addKey(KeyAdr, valueAdr))
		return false;
	
	auto valueNum = VirtualSMCValueNum::withAdr(valueAdr);
	if (!addKey(KeyNum, valueNum))
		return false;
	
	SMC_DATA dataBEMB[] {BaseDeviceInfo::get().modelType == WIOKit::ComputerModel::ComputerLaptop};
	if (!addKey(KeyBEMB, VirtualSMCValueVariable::withData(
		dataBEMB, sizeof(dataBEMB), SmcKeyTypeFlag, SMC_KEY_ATTRIBUTE_READ)))
		return false;
	
	SMC_DATA dataEPCI[] {0x08, 0x10, 0xF0, 0x00};
	if (!addKey(KeyEPCI, VirtualSMCValueVariable::withData(
		dataEPCI, sizeof(dataEPCI), SmcKeyTypeUint32, SMC_KEY_ATTRIBUTE_READ)))
		return false;

	uint8_t gen = 1;
	if (deviceInfo.getGeneration() == SMCInfo::Generation::V3)
		gen = 3;
	else if (deviceInfo.getGeneration() == SMCInfo::Generation::V2)
		gen = 2;

	// This may be present in V1 as well, just should report 1.
	SMC_DATA ldknValue = gen >= 2 ? 2 : 1;
	if (!addKey(KeyLDKN, VirtualSMCValueVariable::withData(
		&ldknValue, sizeof(ldknValue), SmcKeyTypeUint8,
		SMC_KEY_ATTRIBUTE_CONST|SMC_KEY_ATTRIBUTE_READ)))
		return false;
	
	SMC_DATA dataRMde[] {SMC_MODE_APPCODE};
	if (!addKey(KeyRMde, VirtualSMCValueVariable::withData(
		dataRMde, sizeof(dataRMde), SmcKeyTypeChar, SMC_KEY_ATTRIBUTE_CONST|SMC_KEY_ATTRIBUTE_READ)))
		return false;
	
	if (!addKey(KeyNTOK, VirtualSMCValueNTOK::withState()))
		return false;

	if (gen >= 3) {
		// Force 2nd generation on 3rd as it currently causes sleep issues due to AppleSMCPMC
		// being used by AppleIntelPCHPMC.kext. More details can be found in
		// https://github.com/acidanthera/bugtracker/issues/388
		gen = 2;
	}

	if (!addKey(KeyRGEN, VirtualSMCValueVariable::withData(
		&gen, sizeof(uint8_t), SmcKeyTypeUint8, SMC_KEY_ATTRIBUTE_CONST|SMC_KEY_ATTRIBUTE_READ)))
		return false;

	// Checksum keys
	union {
		uint64_t raw;
		uint8_t bytes[4];
	} dataCRC;

	// Some of those may not be present in V1, but they are harmless
	dataCRC.raw = deviceInfo.generatorRand();
	if (!addKey(KeyCRCA, VirtualSMCValueVariable::withData(
		dataCRC.bytes, sizeof(dataCRC.bytes), SmcKeyTypeUint32, SMC_KEY_ATTRIBUTE_READ)))
		return false;
	
	if (!addKey(KeyCRCa, VirtualSMCValueVariable::withData(
		dataCRC.bytes, sizeof(dataCRC.bytes), SmcKeyTypeUint32, SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_FUNCTION)))
		return false;
	
	dataCRC.raw = deviceInfo.generatorRand();
	if (!addKey(KeyCRCB, VirtualSMCValueVariable::withData(
		dataCRC.bytes, sizeof(dataCRC.bytes), SmcKeyTypeUint32, SMC_KEY_ATTRIBUTE_READ)))
		return false;
	
	if (!addKey(KeyCRCb, VirtualSMCValueVariable::withData(
		dataCRC.bytes, sizeof(dataCRC.bytes), SmcKeyTypeUint32, SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_FUNCTION)))
		return false;
	
	dataCRC.raw = deviceInfo.generatorRand();
	if (!addKey(KeyCRCC, VirtualSMCValueVariable::withData(
		dataCRC.bytes, sizeof(dataCRC.bytes), SmcKeyTypeUint32, SMC_KEY_ATTRIBUTE_READ)))
		return false;
	
	if (!addKey(KeyCRCc, VirtualSMCValueVariable::withData(
		dataCRC.bytes, sizeof(dataCRC.bytes), SmcKeyTypeUint32, SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_FUNCTION)))
		return false;
	
	dataCRC.raw = deviceInfo.generatorRand();
	if (!addKey(KeyCRCU, VirtualSMCValueVariable::withData(
		dataCRC.bytes, sizeof(dataCRC.bytes), SmcKeyTypeUint32, SMC_KEY_ATTRIBUTE_READ)))
		return false;
	
	if (!addKey(KeyCRCu, VirtualSMCValueVariable::withData(
		dataCRC.bytes, sizeof(dataCRC.bytes), SmcKeyTypeUint32, SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_FUNCTION)))
		return false;
	
	dataCRC.raw = deviceInfo.generatorRand();
	if (!addKey(KeyCRCR, VirtualSMCValueVariable::withData(
		dataCRC.bytes, sizeof(dataCRC.bytes), SmcKeyTypeUint32, SMC_KEY_ATTRIBUTE_READ)))
		return false;
	
	if (!addKey(KeyCRCr, VirtualSMCValueVariable::withData(
		dataCRC.bytes, sizeof(dataCRC.bytes), SmcKeyTypeUint32, SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_FUNCTION)))
		return false;
	
	dataCRC.raw = deviceInfo.generatorRand();
	if (!addKey(KeyCRCF, VirtualSMCValueVariable::withData(
		dataCRC.bytes, sizeof(dataCRC.bytes), SmcKeyTypeUint32, SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_FUNCTION)))
		return false;
	
	if (!addKey(KeyCRCK, VirtualSMCValueVariable::withData(
		nullptr, sizeof(uint32_t), SmcKeyTypeUint32, SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_FUNCTION)))
		return false;
	
	// Revision keys
	bool hasRevs    = false;
	auto tmpBuf     = deviceInfo.getBuffer(SMCInfo::Buffer::RevMain);
	auto tmpBufSize = deviceInfo.getBufferSize(SMCInfo::Buffer::RevMain);
	for (size_t i = 0; i < tmpBufSize; i++) {
		if (tmpBuf[i] != 0) {
			hasRevs = true;
			break;
		}
	}

	if (hasRevs && !addKey(KeyREV, VirtualSMCValueVariable::withData(
		deviceInfo.getBuffer(SMCInfo::Buffer::RevMain), deviceInfo.getBufferSize(SMCInfo::Buffer::RevMain),
		SmcKeyTypeRev, SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_FUNCTION)))
		return false;
	
	if (hasRevs && !addKey(KeyRVBF, VirtualSMCValueVariable::withData(
		deviceInfo.getBuffer(SMCInfo::Buffer::RevFlasherBase), deviceInfo.getBufferSize(SMCInfo::Buffer::RevFlasherBase),
		SmcKeyTypeRev, SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_FUNCTION)))
		return false;
	
	if (hasRevs && !addKey(KeyRVUF, VirtualSMCValueVariable::withData(
		deviceInfo.getBuffer(SMCInfo::Buffer::RevFlasherUpdate), deviceInfo.getBufferSize(SMCInfo::Buffer::RevFlasherUpdate),
		SmcKeyTypeRev, SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_FUNCTION)))
		return false;
	
	if (hasRevs && !addKey(KeyRBr, VirtualSMCValueVariable::withData(
		deviceInfo.getBuffer(SMCInfo::Buffer::Branch), deviceInfo.getBufferSize(SMCInfo::Buffer::Branch),
		SmcKeyTypeCh8s, SMC_KEY_ATTRIBUTE_CONST|SMC_KEY_ATTRIBUTE_READ)))
		return false;
	
	if (!addKey(KeyRPlt, VirtualSMCValueVariable::withData(
		deviceInfo.getBuffer(SMCInfo::Buffer::Platform), deviceInfo.getBufferSize(SMCInfo::Buffer::Platform),
		SmcKeyTypeCh8s, SMC_KEY_ATTRIBUTE_CONST|SMC_KEY_ATTRIBUTE_READ)))
		return false;
	
	SMC_DATA dataRVCR[SMCInfo::RevisionSize] {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	if (!addKey(KeyRVCR, VirtualSMCValueVariable::withData(
		dataRVCR, SMCInfo::RevisionSize, SmcKeyTypeRev, SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_FUNCTION)))
		return false;

	if (!addKey(KeyRMAC, VirtualSMCValueVariable::withData(
		deviceInfo.getBuffer(SMCInfo::Buffer::MacAddress), deviceInfo.getBufferSize(SMCInfo::Buffer::MacAddress),
		SmcKeyTypeCh8s, SMC_KEY_ATTRIBUTE_ATOMIC|SMC_KEY_ATTRIBUTE_WRITE|SMC_KEY_ATTRIBUTE_READ)))
		return false;

	if (!addKey(KeyRMSN, VirtualSMCValueVariable::withData(
		deviceInfo.getBuffer(SMCInfo::Buffer::MotherboardSerial), deviceInfo.getBufferSize(SMCInfo::Buffer::MotherboardSerial),
		SmcKeyTypeCh8s, SMC_KEY_ATTRIBUTE_ATOMIC|SMC_KEY_ATTRIBUTE_WRITE|SMC_KEY_ATTRIBUTE_READ)))
		return false;

	if (!addKey(KeyRSSN, VirtualSMCValueVariable::withData(
		deviceInfo.getBuffer(SMCInfo::Buffer::Serial), deviceInfo.getBufferSize(SMCInfo::Buffer::Serial),
		SmcKeyTypeCh8s, SMC_KEY_ATTRIBUTE_ATOMIC|SMC_KEY_ATTRIBUTE_WRITE|SMC_KEY_ATTRIBUTE_READ)))
		return false;
	
	// Extra functionality keys

	if (!addKey(KeyCLKH, VirtualSMCValueVariable::withData(
		nullptr, sizeof(SMC_DATA[8]), SmcKeyTypeClh, SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_WRITE|SMC_KEY_ATTRIBUTE_FUNCTION)))
		return false;

	if (!addKey(KeyCLKT, VirtualSMCValueCLKT::withDelta()))
		return false;

	if (!addKey(KeyCLWK, VirtualSMCValueCLWK::withLastWake(&lastWakeTime)))
		return false;

	if (!addKey(KeyDPLM, VirtualSMCValueVariable::withData(
		nullptr, nextGen ? sizeof(SMC_DATA[5]) : sizeof(SMC_DATA[4]), SmcKeyTypeLim,
		SMC_KEY_ATTRIBUTE_PRIVATE_WRITE|SMC_KEY_ATTRIBUTE_WRITE|SMC_KEY_ATTRIBUTE_FUNCTION)))
		return false;

	// Note, that it is present on desktop machines.
	if (!addKey(KeyLDSP, VirtualSMCValueVariable::withData(
		nullptr, sizeof(uint8_t), SmcKeyTypeFlag, SMC_KEY_ATTRIBUTE_WRITE)))
		return false;

	if (nextGen && !addKey(KeyLDLG, VirtualSMCValueLDLG::withEasterEgg()))
		return false;

	// Not much we could do with shutdown causes, but let's at least report they were successful
	//FIXME: These should be saved, but can we write anything reasonable here?
	SMC_DATA dataMSSD[] {0x5};
	if (!addKey(KeyMSSD, VirtualSMCValueVariable::withData(
		dataMSSD, sizeof(dataMSSD), SmcKeyTypeSint8, SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_WRITE|SMC_KEY_ATTRIBUTE_FUNCTION)))
		return false;
	
	if (!addKey(KeyMSDW, VirtualSMCValueVariable::withData(
		nullptr, sizeof(uint8_t), SmcKeyTypeFlag, SMC_KEY_ATTRIBUTE_WRITE|SMC_KEY_ATTRIBUTE_FUNCTION)))
		return false;

	if (!addKey(KeyMSFW, VirtualSMCValueVariable::withData(
		nullptr, sizeof(uint8_t), SmcKeyTypeFlag, SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_WRITE)))
		return false;

	SMC_DATA dataMSSP[] {0x5};
	if (!addKey(KeyMSSP, VirtualSMCValueVariable::withData(
		dataMSSP, sizeof(dataMSSP), SmcKeyTypeSint8, SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_WRITE)))
		return false;

	if (!addKey(KeyMSSW, VirtualSMCValueVariable::withData(
		nullptr, sizeof(uint8_t), SmcKeyTypeFlag, SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_WRITE)))
		return false;

	if (!addKey(KeyMSWr, VirtualSMCValueVariable::withData(
		nullptr, sizeof(uint8_t), SmcKeyTypeUint8, SMC_KEY_ATTRIBUTE_READ)))
		return false;

	SMC_DATA dataMSPC[] {0x19};
	if (!addKey(KeyMSPC, VirtualSMCValueVariable::withData(
		dataMSPC, sizeof(dataMSPC), SmcKeyTypeUint8, SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_WRITE|SMC_KEY_ATTRIBUTE_FUNCTION)))
		return false;
	
	SMC_DATA dataMSPP[] {0x00};
	if (!addKey(KeyMSPP, VirtualSMCValueVariable::withData(
		dataMSPP, sizeof(dataMSPP), SmcKeyTypeUint8, SMC_KEY_ATTRIBUTE_READ)))
		return false;
	
	SMC_DATA dataMSPS[] {dataMSPP[0], 0x04};
	if (!addKey(KeyMSPS, VirtualSMCValueVariable::withData(
		dataMSPS, sizeof(dataMSPS), SmcKeyTypeHex, SMC_KEY_ATTRIBUTE_READ)))
		return false;

	auto valueNATi = VirtualSMCValueNATi::withCountdown();
	if (!addKey(KeyNATi, valueNATi))
		return false;
	
	auto valueNATJ = VirtualSMCValueNATJ::withNATi(valueNATi);
	if (!addKey(KeyNATJ, valueNATJ))
		return false;

	auto valueOSWD = VirtualSMCValueOSWD::withCountdown();
	if (!addKey(KeyOSWD, valueOSWD))
		return false;
	
	if (!addKey(KeyWKTP, VirtualSMCValueVariable::withData(
		nullptr, sizeof(uint8_t), SmcKeyTypeUint8, SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_WRITE|SMC_KEY_ATTRIBUTE_FUNCTION)))
		return false;
	
	if (!addKey(KeyMSQC, VirtualSMCValueVariable::withData(
		nullptr, sizeof(uint8_t), SmcKeyTypeUint8, SMC_KEY_ATTRIBUTE_READ)))
		return false;

	auto valueEVRD = VirtualSMCValueEVRD::withEvents();
	if (!addKey(KeyEVRD, valueEVRD))
		return false;

	if (!addKey(KeyEVCT, VirtualSMCValueEVCT::withBuffer(valueEVRD)))
		return false;

	if (!addKey(KeyEVHF, VirtualSMCValueVariable::withData(
		nullptr, 28, SmcKeyTypeCh8s, SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_FUNCTION)))
		return false;

	if (!addKey(KeyEFBM, VirtualSMCValueVariable::withData(
		nullptr, sizeof(uint8_t), SmcKeyTypeUint8, SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_WRITE)))
		return false;

	if (!addKey(KeyEFBP, VirtualSMCValueVariable::withData(
		nullptr, sizeof(uint8_t), SmcKeyTypeUint8, SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_WRITE)))
		return false;

	if (!addKey(KeyEFBS, VirtualSMCValueEFBS::withBootStatus(13)))
		return false;

	SMC_DATA mspr[2] {0x00, 0x01};
	if (!addKey(KeyMSPR, VirtualSMCValueVariable::withData(
		mspr, sizeof(uint16_t), SmcKeyTypeUint8, SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_CONST)))
		return false;

	if (!addKey(KeyDUSR, VirtualSMCValueDUSR::create()))
		return false;

	if (!addKey(KeyFAC0, VirtualSMCValueVariable::withData(
		nullptr, sizeof(uint8_t), SmcKeyTypeUint8, SMC_KEY_ATTRIBUTE_READ|SMC_KEY_ATTRIBUTE_WRITE)))
		return false;

	// Generic private keys
	auto kpst = VirtualSMCValueKPST::withUnlocked(false);
	if (!addKey(KeyKPST, kpst, true))
		return false;
	
	if (!addKey(KeyKPPW, VirtualSMCValueKPPW::withKPST(
		kpst, deviceInfo.getGeneration()), true))
		return false;
	
	if (!addKey(KeyOSK0, VirtualSMCValueOSK::withIndex(0), true))
		return false;
	if (!addKey(KeyOSK1, VirtualSMCValueOSK::withIndex(1), true))
		return false;
	
	SMC_DATA dataDash[] {0x1};
	if (!addKey(Key____, VirtualSMCValueVariable::withData(
		dataDash, sizeof(dataDash), SmcKeyTypeFlag, SMC_KEY_ATTRIBUTE_CONST|SMC_KEY_ATTRIBUTE_READ), true))
		return false;
	
	return true;
}

SMC_RESULT VirtualSMCKeystore::getByName(SMC_KEY name, VirtualSMCKeyValue *&val, bool hidden) {
	SMC_RESULT r = getByName(hidden ? dataHiddenStorage : dataStorage, name, val);

	if (r != SmcSuccess) {
		for (size_t i = 0; i < VirtualSMCAPI::PluginMax; i++) {
			auto p = atomic_load_explicit(&pluginData[i], memory_order_relaxed);
			if (!p) break;
			r = getByName(hidden ? p->dataHidden : p->data, name, val);
			if (r == SmcSuccess)
				break;
		}
	}

	if (valueKPST)
		static_cast<VirtualSMCValueKPST *>(valueKPST)->setUnlocked(false);

	return r;
}

SMC_RESULT VirtualSMCKeystore::getByName(VirtualSMCAPI::KeyStorage &storage, SMC_KEY name, VirtualSMCKeyValue *&val) {
	if (storage.size() > 0) {
		ssize_t start = 0;
		ssize_t end = storage.size() - 1;
		ssize_t curr;
		
		while (start <= end) {
			curr = (start + end) / 2;
			auto cmp = VirtualSMCKeyValue::compare(storage[curr].key, name);
			
			if (cmp == 0) {
				val = &storage[curr];
				return SmcSuccess;
			} else if (cmp > 0) {
				end = curr - 1;
			} else {
				start = curr + 1;
			}
		}
	}

	return SmcNotFound;
}

SMC_RESULT VirtualSMCKeystore::getByIndex(SMC_KEY_INDEX idx, VirtualSMCKeyValue *&val) {
	if (idx < dataStorage.size()) {
		val = &dataStorage[idx];
		return SmcSuccess;
	}

	auto nidx = idx - dataStorage.size();
	for (size_t i = 0; i < VirtualSMCAPI::PluginMax; i++) {
		auto p = atomic_load_explicit(&pluginData[i], memory_order_relaxed);
		if (!p) break;
		if (nidx < p->data.size()) {
			val = &p->data[nidx];
			return SmcSuccess;
		}
		nidx -= p->data.size();
	}
	
	DBGLOG("kstore", "key at %u not found", idx);
	return SmcNotFound;
}

bool VirtualSMCKeystore::addKey(SMC_KEY key, VirtualSMCValue *val, bool hidden) {
	if (val) {
		auto &d = hidden ? dataHiddenStorage : dataStorage;
		if (d.push_back<4>(VirtualSMCKeyValue::create(key, val))) {
			DBGLOG("kstore", "inserted key [%08X] (%d)", key, hidden);
			return true;
		} else {
			DBGLOG("kstore", "failed to insert key [%08X] (%d)", key, hidden);
			delete val;
		}
	} else {
		DBGLOG("kstore", "missing merge value for [%08X] (%d)", key, hidden);
	}

	return false;
}

SMC_RESULT VirtualSMCKeystore::readValueByName(SMC_KEY key, const VirtualSMCValue *&value) {
	VirtualSMCKeyValue *kv {nullptr};
	auto res = getByName(key, kv, false);
	if (res != SmcSuccess)
		res = getByName(key, kv, true);

	if (res == SmcSuccess) {
		// Any valid value for the time being.
		auto currval = atomic_load_explicit(&kv->value, memory_order_relaxed);

		// Check if readable
		if (!(currval->attr & SMC_KEY_ATTRIBUTE_READ))
			return SmcNotReadable;
		
		// Check if privately readable
		if (currval->attr & SMC_KEY_ATTRIBUTE_PRIVATE_READ &&
			(!static_cast<VirtualSMCValueKPST *>(valueKPST)->unlocked() ||
			 OSSwapInt32(*reinterpret_cast<uint32_t *>(valueEPCI->data) & 0xFF00) == 0xF000))
		return SmcNotReadable;
		
		// Update internal buffers
		res = currval->readAccess();
		if (res == SmcSuccess)
			value = currval;
	} else {
		SYSLOG_COND(reportMissingKeys || ADDPR(debugEnabled), "kstore", "key [%c%c%c%c] not found for reading",
					reinterpret_cast<char *>(&key)[0], reinterpret_cast<char *>(&key)[1],
					reinterpret_cast<char *>(&key)[2], reinterpret_cast<char *>(&key)[3]);
	}
	
	return res;
}

SMC_RESULT VirtualSMCKeystore::readNameByIndex(SMC_KEY_INDEX idx, SMC_KEY &key) {
	VirtualSMCKeyValue *kv {nullptr};
	auto res = getByIndex(idx, kv);
	if (res == SmcSuccess)
		key = kv->key;
		
	return res;

}

SMC_RESULT VirtualSMCKeystore::writeValueByName(SMC_KEY key, const SMC_DATA *data) {
	VirtualSMCKeyValue *kv {nullptr};
	auto res = getByName(key, kv, false);
	if (res != SmcSuccess)
		res = getByName(key, kv, true);
	
	if (res == SmcSuccess) {
		// Any valid value for the time being.
		auto currval = atomic_load_explicit(&kv->value, memory_order_relaxed);

		// Check if writable
		if (!(currval->attr & SMC_KEY_ATTRIBUTE_WRITE))
			return SmcNotWritable;
		
		// Check if privately writable
		if (currval->attr & SMC_KEY_ATTRIBUTE_PRIVATE_WRITE &&
			(!static_cast<VirtualSMCValueKPST *>(valueKPST)->unlocked() ||
			 OSSwapInt32(*reinterpret_cast<uint32_t *>(valueEPCI->data) & 0xFF00) == 0xF000))
			return SmcNotReadable;
		
		// Update internal buffers
		res = currval->writeAccess();
		if (res == SmcSuccess) {
			res = currval->update(data);
		}
	} else {
		SYSLOG_COND(reportMissingKeys || ADDPR(debugEnabled), "kstore", "key [%c%c%c%c] not found for writing",
					reinterpret_cast<char *>(&key)[0], reinterpret_cast<char *>(&key)[1],
					reinterpret_cast<char *>(&key)[2], reinterpret_cast<char *>(&key)[3]);
	}
	
	return res;
}

SMC_RESULT VirtualSMCKeystore::getInfoByName(SMC_KEY key, SMC_DATA_SIZE &size, SMC_KEY_TYPE &type, SMC_KEY_ATTRIBUTES &attr) {
	VirtualSMCKeyValue *kv {nullptr};
	auto res = getByName(key, kv, false);
	if (res != SmcSuccess)
		res = getByName(key, kv, true);
	
	if (res == SmcSuccess) {
		// Any valid value for the time being.
		auto currval = atomic_load_explicit(&kv->value, memory_order_relaxed);;
		size = currval->size;
		type = currval->type;
		attr = currval->attr & ~SMC_KEY_ATTRIBUTE_CONST;

		bool epciStrip = OSSwapInt32(*reinterpret_cast<uint32_t *>(valueEPCI->data) & 0xFF00) == 0xF000;
		if ((attr & SMC_KEY_ATTRIBUTE_PRIVATE_READ) && epciStrip && !static_cast<VirtualSMCValueKPST *>(valueKPST)->unlocked())
			attr &= ~SMC_KEY_ATTRIBUTE_READ;
		if ((attr & SMC_KEY_ATTRIBUTE_PRIVATE_WRITE) && epciStrip && !static_cast<VirtualSMCValueKPST *>(valueKPST)->unlocked())
			attr &= ~SMC_KEY_ATTRIBUTE_WRITE;
	} else {
		SYSLOG_COND(reportMissingKeys || ADDPR(debugEnabled), "kstore", "key [%c%c%c%c] not found for info",
					reinterpret_cast<char *>(&key)[0], reinterpret_cast<char *>(&key)[1],
					reinterpret_cast<char *>(&key)[2], reinterpret_cast<char *>(&key)[3]);
	}
	
	return res;
}
