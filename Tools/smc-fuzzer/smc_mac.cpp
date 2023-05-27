#include <IOKit/IOKitLib.h>

#include "smc.h"

#define KERNEL_INDEX_SMC 2

#define SMC_CMD_READ_BYTES 5
#define SMC_CMD_WRITE_BYTES 6
#define SMC_CMD_READ_INDEX 8
#define SMC_CMD_READ_KEYINFO 9
#define SMC_CMD_READ_PLIMIT 11
#define SMC_CMD_READ_VERS 12

// We only need 1 open connection, might as well be global.
io_connect_t kIOConnection;

uint32_t _strtoul(const char *str, int size, int base) {
  uint32_t total = 0;
  int i;

  for (i = 0; i < size; i++) {
	if (base == 16)
	  total += str[i] << ((size - 1 - i) * 8);
	else
	  total += (unsigned char)(str[i] << ((size - 1 - i) * 8));
  }
  return total;
}

void _ultostr(char *str, uint32_t val) {
  str[0] = '\0';
  snprintf(str,
		   5,
		   "%c%c%c%c",
		   (unsigned int)val >> 24,
		   (unsigned int)val >> 16,
		   (unsigned int)val >> 8,
		   (unsigned int)val);
}

kern_return_t SMCCall(uint32_t selector,
            SMCKeyData_t *inputStructure,
            SMCKeyData_t *outputStructure) {
  size_t structureInputSize;
  size_t structureOutputSize;

  structureInputSize = sizeof(SMCKeyData_t);
  structureOutputSize = sizeof(SMCKeyData_t);

  auto res = IOConnectCallStructMethod(kIOConnection,
                                   selector,
                                   inputStructure,
                                   structureInputSize,
                                   outputStructure,
                                   &structureOutputSize);
  if (res != kIOReturnSuccess)
    printf("Error: IOConnectCallStructMethod 0x%x\n", res);
  return res;
}

uint32_t SMCReadIndexCount(void) {
  SMCVal_t val;
  int num = 0;

  SMCReadKey("#KEY", &val);
  num = ((int)val.bytes[2] << 8) + ((unsigned)val.bytes[3] & 0xff);
  printf("Num: b0=%x b1=%x b2=%x b3=%x size=%u\n",
		 val.bytes[0],
		 val.bytes[1],
		 val.bytes[2],
		 val.bytes[3],
		 (unsigned int)val.dataSize);
  return num;
}

void SMCGetKeys(std::vector<std::string> &keys) {
  uint32_t totalKeys = SMCReadIndexCount();
  for (uint32_t i = 0; i < totalKeys; i++) {
    SMCKeyData_t inputStructure;
    SMCKeyData_t outputStructure;

    memset(&inputStructure, 0, sizeof(SMCKeyData_t));
    memset(&outputStructure, 0, sizeof(SMCKeyData_t));

    inputStructure.data8 = SMC_CMD_READ_INDEX;
    inputStructure.data32 = i;

    bool result =
        SMCCall(KERNEL_INDEX_SMC, &inputStructure, &outputStructure);
    if (result != kIOReturnSuccess) {
      continue;
    }

    UInt32Char_t key;
    _ultostr(key, outputStructure.key);
    keys.push_back(key);
  }
}

bool SMCOpen() {
  kern_return_t result;
  mach_port_t masterPort;
  io_iterator_t iterator;
  io_object_t device;

  result = IOMasterPort(MACH_PORT_NULL, &masterPort);
  if (result != kIOReturnSuccess) {
    printf("Error: IOMasterPort() = %08x\n", result);
    return false;
  }

  CFMutableDictionaryRef matchingDictionary = IOServiceMatching("AppleSMC");
  result =
      IOServiceGetMatchingServices(masterPort, matchingDictionary, &iterator);
  if (result != kIOReturnSuccess) {
    printf("Error: IOServiceGetMatchingServices() = %08x\n", result);
    return false;
  }

  device = IOIteratorNext(iterator);
  IOObjectRelease((io_object_t)iterator);
  if (device == 0) {
    printf("Error: no SMC found\n");
    return false;
  }

  result = IOServiceOpen(device, mach_task_self(), 0, &kIOConnection);
  IOObjectRelease(device);
  if (result != kIOReturnSuccess) {
    printf("Error: IOServiceOpen() = %08x\n", result);
    return false;
  }

  return result == kIOReturnSuccess;
}

bool SMCClose() { return IOServiceClose(kIOConnection) == kIOReturnSuccess; }

bool SMCReadKey(const std::string &key, SMCVal_t *val) {
  kern_return_t result;
  SMCKeyData_t inputStructure;
  SMCKeyData_t outputStructure;

  memset(&inputStructure, 0, sizeof(SMCKeyData_t));
  memset(&outputStructure, 0, sizeof(SMCKeyData_t));
  memset(val, 0, sizeof(SMCVal_t));

  inputStructure.key = _strtoul(key.c_str(), 4, 16);
  snprintf(val->key, 5, "%s", key.c_str());
  inputStructure.data8 = SMC_CMD_READ_KEYINFO;

  result = SMCCall(KERNEL_INDEX_SMC, &inputStructure, &outputStructure);
  if (result != kIOReturnSuccess)
    return false;

  val->dataSize = outputStructure.keyInfo.dataSize;
  _ultostr(val->dataType, outputStructure.keyInfo.dataType);
  inputStructure.keyInfo.dataSize = val->dataSize;
  inputStructure.data8 = SMC_CMD_READ_BYTES;

  result = SMCCall(KERNEL_INDEX_SMC, &inputStructure, &outputStructure);
  if (result != kIOReturnSuccess)
    return false;

  memcpy(val->bytes, outputStructure.bytes, sizeof(outputStructure.bytes));

  return result == kIOReturnSuccess;
}

bool SMCWriteKey(SMCVal_t& writeVal) {
  kern_return_t result;
  SMCKeyData_t inputStructure;
  SMCKeyData_t outputStructure;

  SMCVal_t readVal;

  result = SMCReadKey(writeVal.key, &readVal);
  if (!result) {
    return false;
  }

  if (readVal.dataSize != writeVal.dataSize) {
    writeVal.dataSize = readVal.dataSize;
  }

  memset(&inputStructure, 0, sizeof(SMCKeyData_t));
  memset(&outputStructure, 0, sizeof(SMCKeyData_t));

  inputStructure.key = _strtoul(writeVal.key, 4, 16);
  inputStructure.data8 = SMC_CMD_WRITE_BYTES;
  inputStructure.keyInfo.dataSize = writeVal.dataSize;
  memcpy(inputStructure.bytes, writeVal.bytes, sizeof(writeVal.bytes));

  result = SMCCall(KERNEL_INDEX_SMC, &inputStructure, &outputStructure);
  if (result != kIOReturnSuccess) {
    return false;
  }

  return result == kIOReturnSuccess;
}
