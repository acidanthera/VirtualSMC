//
//  kern_keys.hpp
//  VirtualSMC
//
//  Copyright © 2017 vit9696. All rights reserved.
//

#ifndef kern_keys_hpp
#define kern_keys_hpp

#include "kern_keystore.hpp"

class VirtualSMCValueVariable : public VirtualSMCValue {
public:
	static VirtualSMCValueVariable *withDictionary(OSDictionary *dict);
	static VirtualSMCValueVariable *withData(const SMC_DATA *data, SMC_DATA_SIZE size, SMC_KEY_TYPE type, SMC_KEY_ATTRIBUTES attr, SerializeLevel serialize = SerializeLevel::None);
	virtual ~VirtualSMCValueVariable() = default;
};

class VirtualSMCValueKEY : public VirtualSMCValue {
	VirtualSMCKeystore *kstore {nullptr};
protected:
	SMC_RESULT readAccess() override;
public:
	static VirtualSMCValueKEY *withStore(VirtualSMCKeystore *store);
};

class VirtualSMCValueKPST : public VirtualSMCValue {
public:
	static VirtualSMCValueKPST *withUnlocked(bool value);
	bool unlocked() const;
	void setUnlocked(bool value);
};

class VirtualSMCValueKPPW : public VirtualSMCValue {
	static constexpr const char *PasswordV1 = "SpecialisRevelio";
	static constexpr SMC_DATA_SIZE PasswordSizeV1 {16};
	static constexpr const char *PasswordV2 = "SMC The place to be, definitely!";
	static constexpr SMC_DATA_SIZE PasswordSizeV2 {32};
	VirtualSMCValueKPST *valueKPST {nullptr};
	SMCInfo::Generation generation {SMCInfo::Generation::V1};
public:
	SMC_RESULT update(const SMC_DATA *src) override;
	static VirtualSMCValueKPPW *withKPST(VirtualSMCValueKPST *kpst, SMCInfo::Generation gen);
};

class VirtualSMCValueAdr : public VirtualSMCValue {
public:
	static constexpr uint32_t DefaultAddr {0x300};
	static VirtualSMCValueAdr *withAddr(uint32_t addr = DefaultAddr);
	void setAddress(uint32_t addr);
};

class VirtualSMCValueNum : public VirtualSMCValue {
	VirtualSMCValueAdr *valueAdr {nullptr};
	uint8_t deviceIndex {0};
protected:
	SMC_RESULT readAccess() override;
public:
	SMC_RESULT update(const SMC_DATA *src) override;
	static VirtualSMCValueNum *withAdr(VirtualSMCValueAdr *adr, uint8_t index = 0);
};

class VirtualSMCValueCLKT : public VirtualSMCValue {
	int32_t delta {0};
	int32_t readTime();
protected:
	SMC_RESULT readAccess() override;
public:
	SMC_RESULT update(const SMC_DATA *src) override;
	static VirtualSMCValueCLKT *withDelta(int32_t d = 0);
};

class VirtualSMCValueCLWK : public VirtualSMCValue {
	uint64_t lastWakeStored {0};
	uint16_t counter {0};
	uint64_t *lastWake {};
	bool pending {false};
	bool halt {false};
protected:
	SMC_RESULT readAccess() override;
public:
	SMC_RESULT update(const SMC_DATA *src) override;
	static VirtualSMCValueCLWK *withLastWake(uint64_t *lw);
};

class VirtualSMCValueLDLG : public VirtualSMCValue {
	const char *hiddenMessage {nullptr};
	uint8_t hiddenCode {};
public:
	SMC_RESULT update(const SMC_DATA *src) override;
	static VirtualSMCValueLDLG *withEasterEgg(uint8_t code = 0x96, const char *message = "By Pb & vit9696, from Russia with love!");
};

class VirtualSMCValueHBKP : public VirtualSMCValue {
	enum {
		DumpProhibited = 0,
		DumpNormal = 1,
		// For compatibility reasons with firmwares having issues with RTC writes.
		DumpUnencrypted = 2,
		DumpTotal = 3
	};
	bool dumpToNVRAM {false};
	bool dumpEncrypted {true};
public:
	SMC_RESULT update(const SMC_DATA *src) override;
	static VirtualSMCValueHBKP *withDump(bool dump);
};

class VirtualSMCValueNTOK : public VirtualSMCValue {
public:
	SMC_RESULT update(const SMC_DATA *src) override;
	static VirtualSMCValueNTOK *withState(bool enable=false);
};

/**
 *  Shared code for OSWD and NATi
 */
class VirtualSMCValueTimer : public VirtualSMCValue {
public:
	uint16_t startCountdown();
protected:
	uint64_t jobStartTime {0};
	SMC_RESULT readAccess() override;
};

class VirtualSMCValueNATi : public VirtualSMCValueTimer {
public:
	SMC_RESULT update(const SMC_DATA *src) override;
	static VirtualSMCValueNATi *withCountdown(uint16_t countdown = 0);
};

class VirtualSMCValueNATJ : public VirtualSMCValue {
	VirtualSMCValueNATi *valueNATi {nullptr};
public:
	SMC_RESULT update(const SMC_DATA *src) override;
	static VirtualSMCValueNATJ *withNATi(VirtualSMCValueNATi *nati);
};

class VirtualSMCValueOSWD : public VirtualSMCValueTimer {
public:
	SMC_RESULT update(const SMC_DATA *src) override;
	static VirtualSMCValueOSWD *withCountdown(uint16_t countdown = 0);
};

class VirtualSMCValueEVRD : public VirtualSMCValue {
public:
	static constexpr size_t KeySize {32};
	static VirtualSMCValueEVRD *withEvents(uint8_t *events = nullptr);
};

class VirtualSMCValueEVCT : public VirtualSMCValue {
	VirtualSMCValueEVRD *evrd {nullptr};
public:
	SMC_RESULT update(const SMC_DATA *src) override;
	static VirtualSMCValueEVCT *withBuffer(VirtualSMCValueEVRD *buffer);
};

class VirtualSMCValueEFBS : public VirtualSMCValue {
public:
	SMC_RESULT update(const SMC_DATA *src) override;
	static VirtualSMCValueEFBS *withBootStatus(uint8_t status);
};

class VirtualSMCValueDUSR : public VirtualSMCValue {
public:
	SMC_RESULT update(const SMC_DATA *src) override;
	static VirtualSMCValueDUSR *create();
};

class VirtualSMCValueOSK : public VirtualSMCValue {
	static constexpr size_t KeySize {32};
public:
	static VirtualSMCValueOSK *withIndex(uint8_t index);
};

#endif /* kern_keys_hpp */
