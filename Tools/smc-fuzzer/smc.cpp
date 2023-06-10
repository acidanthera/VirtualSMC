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

#include <algorithm>

#include "smc.h"

// When break key iteration into two steps: (1) discovery, (2) enumeration.
std::vector<std::string> kSMCKeys;

float fpe2ToFlt(char *str, int size) {
  if (size != 2)
    return 0;

  return _OSSwapInt16(*(uint16_t*)str) >> (16U - 0xe);
}

void printFPE2(SMCVal_t val) {
  printf("%.0f ", fpe2ToFlt(val.bytes, val.dataSize));
}

void printSInt(SMCVal_t val) {
	int64_t value = val.bytes[0] & 0x80 ? -1 : 0;
	for (uint32_t i = 0; i < val.dataSize; i++) {
		value = (int64_t)((uint64_t)value << 8) + (uint8_t)val.bytes[i];
	}
	printf("%lld ", value);
}

void printUInt(SMCVal_t val) {
	uint64_t value = 0;
	for (uint32_t i = 0; i < val.dataSize; i++) {
		value = (value << 8) + (uint8_t)val.bytes[i];
	}
	printf("%llu ", value);
}

/* VirtualSMCSDK/kern_vsmcapi.hpp */
static uint32_t getSpIntegral(const char* name) {
  uint32_t ret = 0;
  if (name && strlen(name) >= 3)
    ret = _strtoul(&name[2], static_cast<int>(strlen(name) - 3), 16);

  return ret / 16;
}

template <typename T>
constexpr T getBit(T n) {
  return static_cast<T>(1U) << n;
}

void printSp(SMCVal_t val) {
  uint32_t integral = getSpIntegral(val.dataType);
  if (integral == 0)
    return;
  uint16_t value = ((uint8_t)val.bytes[0] << 8u) | (uint8_t)val.bytes[1];

  double ret = (value & 0x7FFF) / static_cast<double>(getBit<uint16_t>(15 - integral));
  ret = (value & 0x8000) ? -ret : ret;
  printf("%f ", ret);
}

void printBytesHex(SMCVal_t val) {
  uint32_t i;

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
  else if (!strncmp(val.dataType, "sp", 2))
      printSp(val);

    printBytesHex(val);
  } else {
    printf("no data\n");
  }
}

bool SMCPrintAll(const std::vector<std::string> &keys) {
  for (std::vector<std::string>::const_iterator it = keys.begin(); it != keys.end(); it++) {
    SMCVal_t val;
    memset(&val, 0, sizeof(SMCVal_t));

    SMCReadKey(*it, &val);
    printVal(val);
  }

  return true;
}

bool SMCCast(const char spell[33]) {
  SMCVal_t val;
  snprintf(val.key, 5, "KPPW");
  val.dataSize = (uint32_t)strlen(spell);
  snprintf(val.dataType, 5, "ch8*");
  for (size_t i = 0; i < val.dataSize /* 32 */; i++) {
    val.bytes[i] = spell[i];
  }

  bool result = SMCWriteKey(val);
  fprintf(stderr,
          "Wrote '%s' (size %d) to KPPW: %d\n",
          spell,
          (unsigned int)val.dataSize,
          result);

  SMCVal_t result_val;
  result = SMCReadKey("KPST", &result_val);
  if (!result) {
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

bool SMCCompare(const std::vector<std::string> &keys) {
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

          bool result = SMCReadKey(key, &val);
          if (result && val.dataSize > 0) {
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

  return true;
}

bool SMCPrintFans(void) {
  bool result;
  SMCVal_t val;
  UInt32Char_t key;
  int totalFans, i;

  result = SMCReadKey("FNum", &val);
  if (!result)
    return false;

  totalFans = _strtoul(val.bytes, val.dataSize, 10);
  printf("Total fans in system: %d\n", totalFans);

  for (i = 0; i < totalFans; i++) {
    printf("\nFan #%d:\n", i);
    snprintf(key, 5, "F%dAc", i);
    SMCReadKey(key, &val);
    printf("    Actual speed : %.0f Key[%s]\n",
           fpe2ToFlt(val.bytes, val.dataSize),
           key);
    snprintf(key, 5, "F%dMn", i);
    SMCReadKey(key, &val);
    printf("    Minimum speed: %.0f\n", fpe2ToFlt(val.bytes, val.dataSize));
    snprintf(key, 5, "F%dMx", i);
    SMCReadKey(key, &val);
    printf("    Maximum speed: %.0f\n", fpe2ToFlt(val.bytes, val.dataSize));
    snprintf(key, 5, "F%dSf", i);
    SMCReadKey(key, &val);
    printf("    Safe speed   : %.0f\n", fpe2ToFlt(val.bytes, val.dataSize));
    snprintf(key, 5, "F%xTg", i);
    SMCReadKey(key, &val);
    printf("    Target speed : %.0f\n", fpe2ToFlt(val.bytes, val.dataSize));
    SMCReadKey("FS! ", &val);
    if ((_strtoul(val.bytes, 2, 16) & (1 << i)) == 0)
      printf("    Mode         : auto\n");
    else
      printf("    Mode         : forced\n");
  }

  return true;
}

void SMCDetectChange(char value, SMCVal_t write) {
  SMCVal_t before, after;
  SMCReadKey(write.key, &before);
  if (before.dataSize == 1 && before.bytes[0] == write.bytes[0]) {
    write.bytes[0] = before.bytes[0] + 1U;
  }

  bool status = SMCWriteKey(write);
  write.bytes[0] = value;
  if (!status) {
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

bool SMCFuzz(SMCVal_t val, bool fixed_key, bool fixed_val) {
  SMCVal_t write = {};

  if (fixed_key && fixed_val) {
    fprintf(stderr, "Cannot fuzz with a fixed value and key\n");
    return false;
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

    return true;
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

  return true;
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
  // extern int optind, optopt, opterr;

  bool result;
  int op = OP_NONE;
  UInt32Char_t key = "\0";
  char spell[33] = {0};
  SMCVal_t val {};

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

  if (!SMCOpen())
    return 1;

  int retcode = 0;
  switch (op) {
  case OP_LIST:
    SMCGetKeys(kSMCKeys);
    SMCPrintAll(kSMCKeys);

    break;
  case OP_READ:
    if (strlen(key) > 0) {
      result = SMCReadKey(key, &val);
      if (!result) {
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
    if (!result) {
      fprintf(stderr, "Error: SMCPrintFans() = %08x\n", result);
      retcode = 2;
    }
    break;
  case OP_WRITE:
    if (strlen(key) > 0) {
      snprintf(val.key, 5, "%s", key);
      result = SMCWriteKey(val);
      if (!result) {
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
    if (!result) {
      fprintf(stderr, "Error: SMCFuzz() = %08x\n", result);
      retcode = 1;
    }
    break;
  case OP_COMPARE:
    SMCGetKeys(kSMCKeys);
    result = SMCCompare(kSMCKeys);
    if (!result) {
      fprintf(stderr, "Error: SMCCompare() = %08x\n", result);
      retcode = 1;
    }
    break;
  }

  SMCClose();
  return retcode;
}
