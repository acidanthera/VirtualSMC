//
//  kern_keystore.hpp
//  VirtualSMC
//
//  Copyright © 2017 vit9696. All rights reserved.
//

#ifndef kern_keystore_hpp
#define kern_keystore_hpp

#include <Headers/kern_api.hpp>
#include <Headers/kern_nvram.hpp>
#include <VirtualSMCSDK/AppleSmcBridge.hpp>
#include <VirtualSMCSDK/kern_vsmcapi.hpp>
#include <VirtualSMCSDK/kern_smcinfo.hpp>
#include <VirtualSMCSDK/kern_value.hpp>
#include <VirtualSMCSDK/kern_keyvalue.hpp>

#include <IOKit/IORegistryEntry.h>
#include <libkern/c++/OSArray.h>
#include <libkern/c++/OSData.h>
#include <libkern/c++/OSDictionary.h>
#include <libkern/libkern.h>
#include <stdint.h>
#include <stdatomic.h>

class VirtualSMCKeystore {
	/**
	 *  Key name definitions
	 */
	static constexpr SMC_KEY KeyHBKP = SMC_MAKE_IDENTIFIER('H', 'B', 'K', 'P');
	static constexpr SMC_KEY KeyEPCI = SMC_MAKE_IDENTIFIER('E', 'P', 'C', 'I');
	static constexpr SMC_KEY KeyEPST = SMC_MAKE_IDENTIFIER('K', 'P', 'S', 'T');
	static constexpr SMC_KEY KeyKEY  = SMC_MAKE_IDENTIFIER('#', 'K', 'E', 'Y');
	static constexpr SMC_KEY KeyAdr  = SMC_MAKE_IDENTIFIER('$', 'A', 'd', 'r');
	static constexpr SMC_KEY KeyNum  = SMC_MAKE_IDENTIFIER('$', 'N', 'u', 'm');
	static constexpr SMC_KEY KeyBEMB = SMC_MAKE_IDENTIFIER('B', 'E', 'M', 'B');
	static constexpr SMC_KEY KeyLDKN = SMC_MAKE_IDENTIFIER('L', 'D', 'K', 'N');
	static constexpr SMC_KEY KeyRMde = SMC_MAKE_IDENTIFIER('R', 'M', 'd', 'e');
	static constexpr SMC_KEY KeyNTOK = SMC_MAKE_IDENTIFIER('N', 'T', 'O', 'K');
	static constexpr SMC_KEY KeyRGEN = SMC_MAKE_IDENTIFIER('R', 'G', 'E', 'N');
	static constexpr SMC_KEY KeyCRCA = SMC_MAKE_IDENTIFIER('C', 'R', 'C', 'A');
	static constexpr SMC_KEY KeyCRCa = SMC_MAKE_IDENTIFIER('C', 'R', 'C', 'a');
	static constexpr SMC_KEY KeyCRCB = SMC_MAKE_IDENTIFIER('C', 'R', 'C', 'B');
	static constexpr SMC_KEY KeyCRCb = SMC_MAKE_IDENTIFIER('C', 'R', 'C', 'b');
	static constexpr SMC_KEY KeyCRCC = SMC_MAKE_IDENTIFIER('C', 'R', 'C', 'C');
	static constexpr SMC_KEY KeyCRCc = SMC_MAKE_IDENTIFIER('C', 'R', 'C', 'c');
	static constexpr SMC_KEY KeyCRCU = SMC_MAKE_IDENTIFIER('C', 'R', 'C', 'U');
	static constexpr SMC_KEY KeyCRCu = SMC_MAKE_IDENTIFIER('C', 'R', 'C', 'u');
	static constexpr SMC_KEY KeyCRCR = SMC_MAKE_IDENTIFIER('C', 'R', 'C', 'R');
	static constexpr SMC_KEY KeyCRCr = SMC_MAKE_IDENTIFIER('C', 'R', 'C', 'r');
	static constexpr SMC_KEY KeyCRCF = SMC_MAKE_IDENTIFIER('C', 'R', 'C', 'F');
	static constexpr SMC_KEY KeyCRCK = SMC_MAKE_IDENTIFIER('C', 'R', 'C', 'K');
	static constexpr SMC_KEY KeyREV  = SMC_MAKE_IDENTIFIER('R', 'E', 'V', ' ');
	static constexpr SMC_KEY KeyRVBF = SMC_MAKE_IDENTIFIER('R', 'V', 'B', 'F');
	static constexpr SMC_KEY KeyRVUF = SMC_MAKE_IDENTIFIER('R', 'V', 'U', 'F');
	static constexpr SMC_KEY KeyRBr  = SMC_MAKE_IDENTIFIER('R', 'B', 'r', ' ');
	static constexpr SMC_KEY KeyRPlt = SMC_MAKE_IDENTIFIER('R', 'P', 'l', 't');
	static constexpr SMC_KEY KeyRVCR = SMC_MAKE_IDENTIFIER('R', 'V', 'C', 'R');
	static constexpr SMC_KEY KeyRMAC = SMC_MAKE_IDENTIFIER('R', 'M', 'A', 'C');
	static constexpr SMC_KEY KeyRMSN = SMC_MAKE_IDENTIFIER('R', 'M', 'S', 'N');
	static constexpr SMC_KEY KeyRSSN = SMC_MAKE_IDENTIFIER('R', 'S', 'S', 'N');
	static constexpr SMC_KEY KeyCLKH = SMC_MAKE_IDENTIFIER('C', 'L', 'K', 'H');
	static constexpr SMC_KEY KeyCLKT = SMC_MAKE_IDENTIFIER('C', 'L', 'K', 'T');
	static constexpr SMC_KEY KeyCLWK = SMC_MAKE_IDENTIFIER('C', 'L', 'W', 'K');
	static constexpr SMC_KEY KeyDPLM = SMC_MAKE_IDENTIFIER('D', 'P', 'L', 'M');
	static constexpr SMC_KEY KeyLDSP = SMC_MAKE_IDENTIFIER('L', 'D', 'S', 'P');
	static constexpr SMC_KEY KeyLDLG = SMC_MAKE_IDENTIFIER('L', 'D', 'L', 'G');
	static constexpr SMC_KEY KeyMSSD = SMC_MAKE_IDENTIFIER('M', 'S', 'S', 'D');
	static constexpr SMC_KEY KeyMSDW = SMC_MAKE_IDENTIFIER('M', 'S', 'D', 'W');
	static constexpr SMC_KEY KeyMSFW = SMC_MAKE_IDENTIFIER('M', 'S', 'F', 'W');
	static constexpr SMC_KEY KeyMSSP = SMC_MAKE_IDENTIFIER('M', 'S', 'S', 'P');
	static constexpr SMC_KEY KeyMSSW = SMC_MAKE_IDENTIFIER('M', 'S', 'S', 'W');
	static constexpr SMC_KEY KeyMSWr = SMC_MAKE_IDENTIFIER('M', 'S', 'W', 'r');
	static constexpr SMC_KEY KeyMSPC = SMC_MAKE_IDENTIFIER('M', 'S', 'P', 'C');
	static constexpr SMC_KEY KeyMSPP = SMC_MAKE_IDENTIFIER('M', 'S', 'P', 'P');
	static constexpr SMC_KEY KeyMSPS = SMC_MAKE_IDENTIFIER('M', 'S', 'P', 'S');
	static constexpr SMC_KEY KeyNATi = SMC_MAKE_IDENTIFIER('N', 'A', 'T', 'i');
	static constexpr SMC_KEY KeyNATJ = SMC_MAKE_IDENTIFIER('N', 'A', 'T', 'J');
	static constexpr SMC_KEY KeyOSWD = SMC_MAKE_IDENTIFIER('O', 'S', 'W', 'D');
	static constexpr SMC_KEY KeyWKTP = SMC_MAKE_IDENTIFIER('W', 'K', 'T', 'P');
	static constexpr SMC_KEY KeyMSQC = SMC_MAKE_IDENTIFIER('M', 'S', 'Q', 'C');
	static constexpr SMC_KEY KeyEVCT = SMC_MAKE_IDENTIFIER('E', 'V', 'C', 'T');
	static constexpr SMC_KEY KeyEVRD = SMC_MAKE_IDENTIFIER('E', 'V', 'R', 'D');
	static constexpr SMC_KEY KeyEVHF = SMC_MAKE_IDENTIFIER('E', 'V', 'H', 'F');
	static constexpr SMC_KEY KeyEFBM = SMC_MAKE_IDENTIFIER('E', 'F', 'B', 'M');
	static constexpr SMC_KEY KeyEFBP = SMC_MAKE_IDENTIFIER('E', 'F', 'B', 'P');
	static constexpr SMC_KEY KeyEFBS = SMC_MAKE_IDENTIFIER('E', 'F', 'B', 'S');
	static constexpr SMC_KEY KeyMSPR = SMC_MAKE_IDENTIFIER('M', 'S', 'P', 'R');
	static constexpr SMC_KEY KeyDUSR = SMC_MAKE_IDENTIFIER('D', 'U', 'S', 'R');
	static constexpr SMC_KEY KeyFAC0 = SMC_MAKE_IDENTIFIER('F', 'A', 'C', '0');
	static constexpr SMC_KEY KeyKPST = SMC_MAKE_IDENTIFIER('K', 'P', 'S', 'T');
	static constexpr SMC_KEY KeyKPPW = SMC_MAKE_IDENTIFIER('K', 'P', 'P', 'W');
	static constexpr SMC_KEY KeyOSK0 = SMC_MAKE_IDENTIFIER('O', 'S', 'K', '0');
	static constexpr SMC_KEY KeyOSK1 = SMC_MAKE_IDENTIFIER('O', 'S', 'K', '1');
	static constexpr SMC_KEY Key____ = SMC_MAKE_IDENTIFIER('_', '_', '_', '_');

	/**
	 *  Registered keys and hidden keys in the keystore
	 */
	VirtualSMCAPI::KeyStorage dataStorage, dataHiddenStorage;

	/**
	 *  Registered plugin key arrays in the keystore
	 */
	_Atomic(VirtualSMCAPI::Plugin *) pluginData[VirtualSMCAPI::PluginMax] {};

	/**
	 *  Quick access pointers to access keys necessary used for r/w privilege management
	 */
	VirtualSMCValue *valueKPST {nullptr}, *valueEPCI {nullptr};

	/**
	 *  Emulated device information
	 */
	SMCInfo deviceInfo {};

	/**
	 *  Last wake (boot) time
	 */
	uint64_t lastWakeTime {0};

	/**
	 *  Last sleep (shutdown) time
	 */
	uint64_t lastSleepTime {0};

	/**
	 *  Report missing keys to system log when true (configured by -vsmcrpt flag)
	 */
	bool reportMissingKeys {false};

	/**
	 *  Obtain access keys (KPST/EPCI) for caching
	 *
	 *  @return true if found
	 */
	bool findAccessKeys();

	/**
	 *  Create and merge predefined key implementations
	 *
	 *  @param  board     current board-id if present, otherwise nullptr
	 *  @param  model     computer model except any, see WIOKit::ComputerModel
	 *
	 *  @return true on success
	 */
	bool mergePredefined(const char *board, int model);

	/**
	 *  Get value for a key in the kestore
	 *
	 *  @param name    key name
	 *  @param val     resulting value
	 *  @param hidden  look for hidden keys
	 *
	 *  @return SmcSuccess if the value was found
	 */
	SMC_RESULT getByName(SMC_KEY name, VirtualSMCKeyValue *&kv, bool hidden);

	/**
	 *  Get value for a key in the kestore
	 *
	 *  @param storage  look up key storage
	 *  @param name     key name
	 *  @param val      resulting value
	 *
	 *  @return SmcSuccess if the value was found
	 */
	SMC_RESULT getByName(VirtualSMCAPI::KeyStorage &storage, SMC_KEY name, VirtualSMCKeyValue *&kv);

	/**
	 *  Get value for an index of an entry in the keystore
	 *  This method searches only the public storage
	 *
	 *  @param idx  index of an entry
	 *  @param val  resulting value
	 *  @return SmcSuccess if the value was found and read
	 */
	SMC_RESULT getByIndex(SMC_KEY_INDEX idx, VirtualSMCKeyValue *&val);

	/**
	 *  Add key to the keystore
	 *
	 *  @param key     key name
	 *  @param val     value to store
	 *  @param hidden  key visibility
	 *
	 *  @return true on success
	 */
	bool addKey(SMC_KEY key, VirtualSMCValue *val, bool hidden=false);

	/**
	 *  Loaded serialisation mode
	 */
	SerializeLevel serLevel {SerializeLevel::None};

	/**
	 *  Keystore serialiser and deserialiser
	 */
	NVStorage *serializer {nullptr};

	/**
	 *  Import serialized binary data into keystore
	 *
	 *  @param src   binary data buffer
	 *  @param size  binary data size
	 *
	 *  @return true on success
	 */
	bool deserialize(const uint8_t *src, uint32_t size);

	/**
	 *  Serialize keystore into binary data
	 *
	 *  @param size  size of the return buffer
	 *  @return serialized keystore buffer allocated by Buffer::create
	 */
	uint8_t *serialize(size_t &size) const;
public:

	/**
	 *  Merges keys into keystore from a provider OSDictionary
	 *
	 *  @param  dict      source provider
	 *  @param  board     current board-id if present, otherwise nullptr
	 *  @param  model     computer model except any, see WIOKit::ComputerModel
	 *
	 *  @return true on success
	 *
	 *  Source provider is a dictionary of OSArrays with KeyValue entries.
	 *  Each dictionary has one of the following names, that have highest to lowest priority.
	 *  Firstly all the properties with higher priority are parsed, then the ones with lower priority.
	 *  If two key names collide the ones with higher priority are used, if either is missing it is not an error.
	 *
	 *
	 *  Property       Definition                                Explanation
	 *  -------------------------------------------------------------------------------------------------------
	 *  Mac-XXXXXXXXX  XXXXXXXXX is board-id                     Properties of this mac model
	 *  GenericMMMMVY  MMMM is Laptop or Desktop, Y is 1 or 2    Properties of SMC version Y, computer type MMMM
	 *  GenericVY      Y is either 1 or 2                        Properties of SMC version Y
	 *  Generic                                                  Common properties
	 *
	 *  The entry dictionary contained in OSArray is described in merge method.
	 */
	bool mergeProvider(const OSDictionary *dict, const char *board, int model);

	/**
	 *
	 *  Merges keys into keystore from an array of KeyValue OSDictionaries
	 *
	 *  @param  arr     source array
	 *
	 *  @return true on success
	 *
	 *  Each array entry describes KeyValue OSDictionary:
	 *
	 *  Property         Type        Size                  Value              Notes
	 *  -------------------------------------------------------------------------------------------------------
	 *  name             OSData      4 bytes               Key name
	 *  type             OSData      4 bytes               Key type
	 *  attr             OSNumber    8 bit+                Key attributes
	 *  value            OSData      <= SMC_MAX_DATA_SIZE  Initial value      [optional, null by default]
	 *  size             OSNumber    8 bit+                Value size         [optional, if value is present]
	 *  hidden           OSBoolean                         Hidden key or not  [optional, false by default]
	 *  serialize        OSBoolean                         Serialise or not   [optional, false by default]
	 */
	bool merge(const OSArray *arr);
	
	/**
	 *  Create a keystore given key providers and device information
	 *
	 *  @param  mainprops  main KeyValue provider (see mergeProvider)
	 *  @param  mainprops  user KeyValue provider (has lower priority)
	 *  @param  info       device info
	 *  @param  board      current board-id if present, otherwise nullptr
	 *  @param  model      computer model except any, see WIOKit::ComputerModel
	 *  @param  whbkp      allow hbkp usage
	 *
	 *  @return true on success
	 */
	bool init(const OSDictionary *mainprops, const OSDictionary *userprops, const SMCInfo &info, const char *board, int model, bool whibkey);

	/**
	 *  Obtain key value from the keystore by its name
	 *
	 *  @param name    key name
	 *  @param val     resulting value
	 *
	 *  @return SmcSuccess if the value was found, was read-accessible, and the data was read
	 */
	SMC_RESULT readValueByName(SMC_KEY name, const VirtualSMCValue *&value);

	/**
	 *  Obtain key value from the keystore by its index
	 *
	 *  @param idx     index of an entry
	 *  @param val     resulting value
	 *
	 *  @return SmcSuccess if the value was found, was read-accessible, and the data was read
	 */
	SMC_RESULT readNameByIndex(SMC_KEY_INDEX idx, SMC_KEY &key);

	/**
	 *  Set key value in the keystore by its name
	 *
	 *  @param name    key name
	 *  @param data    data buffer with new content (equal or bigger than SMC_MAX_DATA_SIZE)
	 *
	 *  @return SmcSuccess if the value was found, was write-accessible, and the data was written
	 */
	SMC_RESULT writeValueByName(SMC_KEY name, const SMC_DATA *data);

	/**
	 *  Obtain key information from the keystore by its name
	 *
	 *  @param name    key name
	 *  @param size    key value size
	 *  @param type    key value type
	 *  @param attr    key value attributes
	 *
	 *  @return SmcSuccess if the value was found and size, type, and attr variables were updated
	 */
	SMC_RESULT getInfoByName(SMC_KEY name, SMC_DATA_SIZE &size, SMC_KEY_TYPE &type, SMC_KEY_ATTRIBUTES &attr);

	/**
	 *  Get total number of publicly available keys
	 *
	 *  @return number of keys
	 */
	uint32_t getPublicKeyAmount();

	/**
	 *  Handle power off (sleep, shutdown, reboot)
	 */
	void handlePowerOff();

	/**
	 *  Handle power on (boot, wake)
	 */
	void handlePowerOn();

	/**
	 *  Load plugin
	 *
	 *  @param plugin  plugin pointer
	 *
	 *  @return kIOReturnSuccess on success
	 */
	IOReturn loadPlugin(VirtualSMCAPI::Plugin *plugin);

	/**
	 *  Obtain device info
	 *
	 *  @return pointer to unmodifiable deviceInfo
	 */
	const SMCInfo &getDeviceInfo() {
		return deviceInfo;
	}
};

#endif /* kern_keystore_hpp */
