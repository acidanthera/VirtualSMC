/*
 * Apple System Management Control (SMC) Tool
 * Copyright (C) 2015 theopolis
 * Copyright (C) 2006 devnull
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

#include <string>
#include <vector>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "smc.h"

// We only need 1 open connection, might as well be global.
io_connect_t kIOConnection;

// When break key iteration into two steps: (1) discovery, (2) enumeration.
std::vector<std::string> kSMCKeys;

UInt32 _strtoul(const char *str, int size, int base) {
  UInt32 total = 0;
  int i;

  for (i = 0; i < size; i++) {
    if (base == 16)
      total += str[i] << ((size - 1 - i) * 8);
    else
      total += (unsigned char)(str[i] << ((size - 1 - i) * 8));
  }
  return total;
}

void _ultostr(char *str, UInt32 val) {
  str[0] = '\0';
  snprintf(str,
           5,
           "%c%c%c%c",
           (unsigned int)val >> 24,
           (unsigned int)val >> 16,
           (unsigned int)val >> 8,
           (unsigned int)val);
}

float _strtof(char *str, int size, int e) {
  float total = 0;
  int i;

  for (i = 0; i < size; i++) {
    if (i == (size - 1))
      total += (str[i] & 0xff) >> e;
    else
      total += str[i] << ((size - 1 - i) * (8 - e));
  }

  return total;
}

void printFPE2(SMCVal_t val) {
  /* FIXME: This decode is incomplete, last 2 bits are dropped */

  printf("%.0f ", _strtof(val.bytes, val.dataSize, 2));
}

void printSInt(SMCVal_t val) {
	int64_t value = val.bytes[0] & 0x80 ? -1 : 0;
	for (int i = 0; i < val.dataSize; i++) {
		value = (int64_t)((uint64_t)value << 8) + (uint8_t)val.bytes[i];
	}
	printf("%lld ", value);
}

void printUInt(SMCVal_t val) {
	uint64_t value = 0;
	for (int i = 0; i < val.dataSize; i++) {
		value = (value << 8) + (uint8_t)val.bytes[i];
	}
	printf("%llu ", value);
}

void printBytesHex(SMCVal_t val) {
  int i;

  printf("(bytes");
  for (i = 0; i < val.dataSize; i++)
    printf(" %02x", (unsigned char)val.bytes[i]);
  printf(")\n");
}

void printVal(SMCVal_t val) {
  printf("  %s  [%-4s]  ", val.key, val.dataType);
  if (val.dataSize > 0) {
    if ((strcmp(val.dataType, DATATYPE_UINT8) == 0) ||
        (strcmp(val.dataType, DATATYPE_UINT16) == 0) ||
        (strcmp(val.dataType, DATATYPE_UINT32) == 0))
      printUInt(val);
	else if ((strcmp(val.dataType, DATATYPE_SINT8) == 0) ||
			 (strcmp(val.dataType, DATATYPE_SINT16) == 0) ||
			 (strcmp(val.dataType, DATATYPE_SINT32) == 0))
		printSInt(val);
    else if (strcmp(val.dataType, DATATYPE_FPE2) == 0)
      printFPE2(val);

    printBytesHex(val);
  } else {
    printf("no data\n");
  }
}

kern_return_t SMCOpen(io_connect_t *conn) {
  kern_return_t result;
  mach_port_t masterPort;
  io_iterator_t iterator;
  io_object_t device;

  result = IOMasterPort(MACH_PORT_NULL, &masterPort);
  if (result != kIOReturnSuccess) {
    printf("Error: IOMasterPort() = %08x\n", result);
    return 1;
  }

  CFMutableDictionaryRef matchingDictionary = IOServiceMatching("AppleSMC");
  result =
      IOServiceGetMatchingServices(masterPort, matchingDictionary, &iterator);
  if (result != kIOReturnSuccess) {
    printf("Error: IOServiceGetMatchingServices() = %08x\n", result);
    return 1;
  }

  device = IOIteratorNext(iterator);
  IOObjectRelease((io_object_t)iterator);
  if (device == 0) {
    printf("Error: no SMC found\n");
    return 1;
  }

  result = IOServiceOpen(device, mach_task_self(), 0, conn);
  IOObjectRelease(device);
  if (result != kIOReturnSuccess) {
    printf("Error: IOServiceOpen() = %08x\n", result);
    return 1;
  }

  return kIOReturnSuccess;
}

kern_return_t SMCClose(io_connect_t conn) { return IOServiceClose(conn); }

kern_return_t SMCCall(uint32_t selector,
                      SMCKeyData_t *inputStructure,
                      SMCKeyData_t *outputStructure) {
  size_t structureInputSize;
  size_t structureOutputSize;

  structureInputSize = sizeof(SMCKeyData_t);
  structureOutputSize = sizeof(SMCKeyData_t);

  return IOConnectCallStructMethod(kIOConnection,
                                   selector,
                                   inputStructure,
                                   structureInputSize,
                                   outputStructure,
                                   &structureOutputSize);
}

kern_return_t SMCReadKey(const std::string &key, SMCVal_t *val) {
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
    return result;

  val->dataSize = outputStructure.keyInfo.dataSize;
  _ultostr(val->dataType, outputStructure.keyInfo.dataType);
  inputStructure.keyInfo.dataSize = val->dataSize;
  inputStructure.data8 = SMC_CMD_READ_BYTES;

  result = SMCCall(KERNEL_INDEX_SMC, &inputStructure, &outputStructure);
  if (result != kIOReturnSuccess)
    return result;

  memcpy(val->bytes, outputStructure.bytes, sizeof(outputStructure.bytes));

  return kIOReturnSuccess;
}

kern_return_t SMCWriteKey(SMCVal_t writeVal) {
  kern_return_t result;
  SMCKeyData_t inputStructure;
  SMCKeyData_t outputStructure;

  SMCVal_t readVal;

  result = SMCReadKey(writeVal.key, &readVal);
  if (result != kIOReturnSuccess) {
    return result;
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
    return result;
  }

  return kIOReturnSuccess;
}

UInt32 SMCReadIndexCount(void) {
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
  UInt32 totalKeys = SMCReadIndexCount();
  for (UInt32 i = 0; i < totalKeys; i++) {
    SMCKeyData_t inputStructure;
    SMCKeyData_t outputStructure;

    memset(&inputStructure, 0, sizeof(SMCKeyData_t));
    memset(&outputStructure, 0, sizeof(SMCKeyData_t));

    inputStructure.data8 = SMC_CMD_READ_INDEX;
    inputStructure.data32 = i;

    kern_return_t result =
        SMCCall(KERNEL_INDEX_SMC, &inputStructure, &outputStructure);
    if (result != kIOReturnSuccess) {
      continue;
    }

    UInt32Char_t key;
    _ultostr(key, outputStructure.key);
    keys.push_back(key);
  }
}

kern_return_t SMCPrintAll(const std::vector<std::string> &keys) {
  for (std::vector<std::string>::const_iterator it = keys.begin(); it != keys.end(); it++) {
    SMCVal_t val;
    memset(&val, 0, sizeof(SMCVal_t));

    SMCReadKey(*it, &val);
    printVal(val);
  }

  return kIOReturnSuccess;
}

bool SMCCast(const char spell[33]) {
  SMCVal_t val;
  snprintf(val.key, 5, "KPPW");
  val.dataSize = (UInt32)strlen(spell);
  snprintf(val.dataType, 5, "ch8*");
  for (size_t i = 0; i < val.dataSize /* 32 */; i++) {
    val.bytes[i] = spell[i];
  }

  kern_return_t result = SMCWriteKey(val);
  fprintf(stderr,
          "Wrote '%s' (size %d) to KPPW: %d\n",
          spell,
          (unsigned int)val.dataSize,
          result);

  SMCVal_t result_val;
  result = SMCReadKey("KPST", &result_val);
  if (result != kIOReturnSuccess) {
    fprintf(stderr, "Could not read KPST value: %d\n", result);
  }

  uint8_t response = result_val.bytes[0];
  if (response != 1) {
    fprintf(stderr, "Your spell: '%s' was incorrect: (%d)\n", spell, response);
  } else {
    fprintf(stderr, "Success KPST: %d\n", response);
  }

  return (response == 1);
}

kern_return_t SMCCompare(const std::vector<std::string> &keys) {
  // Iterate each key using the fixed or choosen value.
  UInt32Char_t key = {0};

  char k = 33;
  size_t max = 125 - 33;
  for (size_t i = 0; i < max; i++) {
    key[0] = k + i;
    for (size_t ii = 0; ii < max; ii++) {
      key[1] = k + ii;
      for (size_t iii = 0; iii < max; iii++) {
        key[2] = k + iii;
        for (size_t iiii = 0; iiii < max; iiii++) {
          key[3] = k + iiii;
          SMCVal_t val;

          kern_return_t result = SMCReadKey(key, &val);
          if (result == kIOReturnSuccess && val.dataSize > 0) {
            if (std::find(keys.begin(), keys.end(), key) != keys.end()) {
              continue;
            }
            // This key was not in the discovered key list.
            printVal(val);
          }
        }
      }
    }
  }

  return kIOReturnSuccess;
}

kern_return_t SMCPrintFans(void) {
  kern_return_t result;
  SMCVal_t val;
  UInt32Char_t key;
  int totalFans, i;

  result = SMCReadKey("FNum", &val);
  if (result != kIOReturnSuccess)
    return kIOReturnError;

  totalFans = _strtoul(val.bytes, val.dataSize, 10);
  printf("Total fans in system: %d\n", totalFans);

  for (i = 0; i < totalFans; i++) {
    printf("\nFan #%d:\n", i);
    snprintf(key, 5, "F%dAc", i);
    SMCReadKey(key, &val);
    printf("    Actual speed : %.0f Key[%s]\n",
           _strtof(val.bytes, val.dataSize, 2),
           key);
    snprintf(key, 5, "F%dMn", i);
    SMCReadKey(key, &val);
    printf("    Minimum speed: %.0f\n", _strtof(val.bytes, val.dataSize, 2));
    snprintf(key, 5, "F%dMx", i);
    SMCReadKey(key, &val);
    printf("    Maximum speed: %.0f\n", _strtof(val.bytes, val.dataSize, 2));
    snprintf(key, 5, "F%dSf", i);
    SMCReadKey(key, &val);
    printf("    Safe speed   : %.0f\n", _strtof(val.bytes, val.dataSize, 2));
    sprintf(key, "F%dTg", i);
    SMCReadKey(key, &val);
    printf("    Target speed : %.0f\n", _strtof(val.bytes, val.dataSize, 2));
    SMCReadKey("FS! ", &val);
    if ((_strtoul(val.bytes, 2, 16) & (1 << i)) == 0)
      printf("    Mode         : auto\n");
    else
      printf("    Mode         : forced\n");
  }

  return kIOReturnSuccess;
}

void SMCDetectChange(char value, SMCVal_t write) {
  SMCVal_t before, after;
  SMCReadKey(write.key, &before);
  if (before.dataSize == 1 && before.bytes[0] == write.bytes[0]) {
    write.bytes[0] = before.bytes[0] + 1U;
  }

  kern_return_t status = SMCWriteKey(write);
  write.bytes[0] = value;
  if (status != kIOReturnSuccess) {
    return;
  }

  bool value_changed = false;
  SMCReadKey(write.key, &after);
  if (before.dataSize != after.dataSize) {
    value_changed = true;
  } else {
    for (size_t i = 0; i < before.dataSize; i++) {
      if (before.bytes[0] != after.bytes[0]) {
        value_changed = true;
        break;
      }
    }
  }

  printf("%s writable %s using value %d\n",
         write.key,
         (value_changed) ? "and modifiable" : "only",
         write.bytes[0]);
}

kern_return_t SMCFuzz(SMCVal_t val, bool fixed_key, bool fixed_val) {
  SMCVal_t write;

  if (fixed_key && fixed_val) {
    fprintf(stderr, "Cannot fuzz with a fixed value and key\n");
    return kIOReturnError;
  }

  if (fixed_val) {
    // If using a fixed value, the fuzzer will iterate through keys
    // and supply a choosen value.
    val.key[0] = 0;
    fprintf(stderr, "Fuzzing using a fixed value: ");
    printVal(val);
  } else {
    // If not using a fixed value or fixed value the choosen value
    // for fuzzing all keys is 0x01.
    // This could use improvement.
    fprintf(stderr, "Fuzzing keys using: 0x01\n");
    write.bytes[0] = 1;
    write.dataSize = 1;
  }

  if (fixed_key) {
    fprintf(stderr, "Fuzzing using a fixed key: %s\n", val.key);
    memcpy(write.key, val.key, sizeof(val.key));
    for (size_t i = 0; i < 0xFF; i++) {
      write.bytes[0] = i;
      SMCDetectChange(i, write);
    }

    return kIOReturnSuccess;
  }

  // Iterate each key using the fixed or choosen value.
  char og = val.bytes[0];
  char k = 33;
  size_t max = 125 - 33;
  for (size_t i = 0; i < max; i++) {
    write.key[0] = k + i;
    for (size_t ii = 0; ii < max; ii++) {
      write.key[1] = k + ii;
      for (size_t iii = 0; iii < max; iii++) {
        write.key[2] = k + iii;
        for (size_t iiii = 0; iiii < max; iiii++) {
          write.key[3] = k + iiii;
          SMCDetectChange(og, write);
        }
      }
    }
  }

  return kIOReturnSuccess;
}

void usage(char *prog) {
  printf("Apple System Management Control (SMC) tool %s\n", VERSION);
  printf("Usage:\n");
  printf("%s [options]\n", prog);
  printf("    -c <spell> : cast a spell\n");
  printf("    -q         : attempt to discover 'hidden' keys\n");
  printf("    -z         : fuzz all possible keys (or one key using -k)\n");
  printf("    -f         : fan info decoded\n");
  printf("    -h         : help\n");
  printf("    -k <key>   : key to manipulate\n");
  printf("    -l         : list all keys and values\n");
  printf("    -r         : read the value of a key\n");
  printf("    -w <value> : write the specified value to a key\n");
  printf("    -v         : version\n");
  printf("\n");
}

int main(int argc, char *argv[]) {
  int c;
  extern char *optarg;
  extern int optind, optopt, opterr;

  kern_return_t result;
  int op = OP_NONE;
  UInt32Char_t key = "\0";
  char spell[33] = {0};
  SMCVal_t val;

  bool fixed_key = false, fixed_val = false;
	while ((c = getopt(argc, argv, "fhk:lr:w:c:vzq")) != -1) {
    switch (c) {
    case 'f':
      op = OP_READ_FAN;
      break;
    case 'c':
      snprintf(spell, 33, "%s", optarg);
      op = OP_CAST;
      break;
    case 'k':
      fixed_key = true;
      snprintf(key, 5, "%s", optarg);
      break;
    case 'l':
      op = OP_LIST;
      break;
    case 'r':
      op = OP_READ;
      snprintf(key, 5, "%s", optarg);
      break;
    case 'v':
      printf("%s\n", VERSION);
      return 0;
      break;
    case 'w':
      fixed_val = true;
      if (op != OP_FUZZ)
        op = OP_WRITE;
      val.dataSize = 0;
      if (strlen(optarg) % 2 != 0)
        sscanf(optarg++, "%1hhx", reinterpret_cast<unsigned char *>(&val.bytes[val.dataSize++]));
      for (size_t i = 0, len = strlen(optarg)/2; i < len; i++, optarg += 2)
        sscanf(optarg, "%2hhx", reinterpret_cast<unsigned char *>(&val.bytes[val.dataSize++]));
      break;
    case 'h':
    case '?':
      op = OP_NONE;
      break;
    case 'z':
      // Allow the fuzzing request to override write/read.
      op = OP_FUZZ;
      break;
    case 'q':
      op = OP_COMPARE;
      break;
    }
  }

  if (op == OP_NONE) {
    usage(argv[0]);
    return 1;
  }

  SMCOpen(&kIOConnection);

  int retcode = 0;
  switch (op) {
  case OP_LIST:
    SMCGetKeys(kSMCKeys);
    result = SMCPrintAll(kSMCKeys);
    if (result != kIOReturnSuccess) {
      fprintf(stderr, "Error: SMCPrintAll() = %08x\n", result);
      retcode = 1;
    }
    break;
  case OP_READ:
    if (strlen(key) > 0) {
      result = SMCReadKey(key, &val);
      if (result != kIOReturnSuccess) {
        fprintf(stderr, "Error: SMCReadKey() = %08x\n", result);
        retcode = 1;
      }

      else {
        printVal(val);
      }
    } else {
      printf("Error: specify a key to read\n");
      retcode = 2;
    }
    break;
  case OP_READ_FAN:
    result = SMCPrintFans();
    if (result != kIOReturnSuccess) {
      fprintf(stderr, "Error: SMCPrintFans() = %08x\n", result);
      retcode = 2;
    }
    break;
  case OP_WRITE:
    if (strlen(key) > 0) {
      snprintf(val.key, 5, "%s", key);
      result = SMCWriteKey(val);
      if (result != kIOReturnSuccess) {
        fprintf(stderr, "Error: SMCWriteKey() = %08x\n", result);
        retcode = 1;
      }
    } else {
      printf("Error: specify a key to write\n");
      retcode = 2;
    }
    break;
  case OP_CAST:
    if (!SMCCast(spell)) {
      retcode = 1;
    }
    break;
  case OP_FUZZ:
    if (strlen(key) > 0) {
      snprintf(val.key, 5, "%s", key);
    }
    result = SMCFuzz(val, fixed_key, fixed_val);
    if (result != kIOReturnSuccess) {
      fprintf(stderr, "Error: SMCFuzz() = %08x\n", result);
      retcode = 1;
    }
    break;
  case OP_COMPARE:
    SMCGetKeys(kSMCKeys);
    result = SMCCompare(kSMCKeys);
    if (result != kIOReturnSuccess) {
      fprintf(stderr, "Error: SMCCompare() = %08x\n", result);
      retcode = 1;
    }
    break;
  }

  SMCClose(kIOConnection);
  return retcode;
}
