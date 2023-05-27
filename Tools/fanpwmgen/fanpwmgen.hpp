//
//  fanpwmgen.hpp
//  fanpwmgen
//
//  Created by curi0 on 2023-05-26.
//  Copyright © 2023 vit9696. All rights reserved.
//

#ifndef fanpwmgen_hpp
#define fanpwmgen_hpp

#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <string>
#include <vector>

#define OP_NONE 0
#define OP_LIST 1
#define OP_READ 2
#define OP_READ_FAN 3
#define OP_WRITE 4
#define OP_FUZZ 5
#define OP_COMPARE 6
#define OP_CAST 7

#define DATATYPE_FPE2 "fpe2"
#define DATATYPE_UINT8 "ui8 "
#define DATATYPE_UINT16 "ui16"
#define DATATYPE_UINT32 "ui32"
#define DATATYPE_SINT8 "si8 "
#define DATATYPE_SINT16 "si16"
#define DATATYPE_SINT32 "si32"
#define DATATYPE_SP78 "sp78"

// key values
/*#define SMC_KEY_CPU_TEMP      "TC0D"
#define SMC_KEY_FAN0_RPM_MIN  "F0Tg"
#define SMC_KEY_FAN1_RPM_MIN  "F1Tg"
#define SMC_KEY_FAN0_RPM_CUR  "F0Ac"
#define SMC_KEY_FAN1_RPM_CUR  "F1Ac"*/

typedef struct {
  char major;
  char minor;
  char build;
  char reserved[1];
  uint16_t release;
} SMCKeyData_vers_t;

typedef struct {
  uint16_t version;
  uint16_t length;
  uint32_t cpuPLimit;
  uint32_t gpuPLimit;
  uint32_t memPLimit;
} SMCKeyData_pLimitData_t;

typedef struct {
  uint32_t dataSize;
  uint32_t dataType;
  char dataAttributes;
} SMCKeyData_keyInfo_t;

typedef char SMCBytes_t[32];

typedef struct {
  uint32_t key;
  SMCKeyData_vers_t vers;
  SMCKeyData_pLimitData_t pLimitData;
  SMCKeyData_keyInfo_t keyInfo;
  char result;
  char status;
  char data8;
  uint32_t data32;
  SMCBytes_t bytes;
} SMCKeyData_t;

typedef char UInt32Char_t[5];

typedef struct {
  UInt32Char_t key;
  uint32_t dataSize;
  UInt32Char_t dataType;
  SMCBytes_t bytes;
} SMCVal_t;

// prototypes
double SMCGetTemperature(char *key);
bool SMCSetFanRpm(char *key, int32_t rpm);
int SMCGetFanRpm(char *key);

bool SMCOpen();
bool SMCReadKey(const std::string &key, SMCVal_t *val);
bool SMCWriteKey(SMCVal_t& writeVal);
void SMCGetKeys(std::vector<std::string> &keys);
bool SMCClose();
uint32_t SMCReadIndexCount(void);

uint32_t _strtoul(const char *str, int size, int base);
void _ultostr(char *str, uint32_t val);

#endif /* fanpwmgen_hpp */
