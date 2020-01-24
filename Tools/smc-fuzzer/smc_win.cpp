#include <Windows.h>

#include "smc.h"

enum MacHalControlCode : uint32_t {
    READ_BYTES = 0x9C402454, /* NUL-terminated key, VLA data */
    WRITE_BYTES = 0x9C402458, /* NUL-terminated key, VLA data */
    READ_BYTES_INFO = 0x9C402460, /* read to KeyInfo, fixed sz */
    READ_NAME_IDX = 0x9C40245C /* 4b idx in, 5b name out */
};

struct [[gnu::packed]] KeyInfo {
    union {
        uint32_t v32;
        uint16_t v16;
    };
    char type[4];
    uint32_t attr;
};

static_assert(sizeof(KeyInfo) == 12, "");

HANDLE macHal {INVALID_HANDLE_VALUE};

bool SMCOpen() {
    macHal = CreateFile("\\\\.\\MacHALDriver", GENERIC_READ, 0, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    if (macHal == INVALID_HANDLE_VALUE)
        printf("Error: CreateFile failed with 0x%lx\n", GetLastError());

    return macHal != INVALID_HANDLE_VALUE;
}

bool SMCClose() {
    CloseHandle(macHal);
    return true;
}

/* FIXME: There doesn't seem to be a way to get value size directly */
static uint32_t lenFromType(SMCVal_t& val) {
    uint32_t ret {};

    if (!strcmp(val.dataType, DATATYPE_UINT8) ||
        !strcmp(val.dataType, DATATYPE_SINT8))
        return 1;
    if (!strcmp(val.dataType, DATATYPE_UINT16) ||
        !strcmp(val.dataType, DATATYPE_SINT8) ||
        !strcmp(val.dataType, DATATYPE_FPE2) ||
        !strncmp(val.dataType, "sp", 2))
        return 2;
    if (!strcmp(val.dataType, DATATYPE_UINT32) ||
        !strcmp(val.dataType, DATATYPE_SINT32))
        return 4;
    if (!strcmp(val.dataType, "ch8*") ||
        !strcmp(val.dataType, "hex_"))
        return strnlen(val.bytes, sizeof(val.bytes));

    return ret;
}

bool SMCReadKey(const std::string &key, SMCVal_t *val) {
    if (macHal == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Error: MacHal handle not open\n");
        return false;
    }

    if (key.size() != 4) {
        fprintf(stderr, "Error: Wrong key string size %zu\n", key.size());
        return false;
    }

    KeyInfo ki {};
    char name[5];
    memcpy(name, key.c_str(), 4);
    name[4] = '\0';

    DWORD nread {};
    auto res = DeviceIoControl(macHal, MacHalControlCode::READ_BYTES_INFO,
        name, sizeof(name),
        &ki, sizeof(ki),
        &nread, nullptr);

    if (!res)
        fprintf(stderr, "Error: Failed to read with 0x%lx\n", GetLastError());
    else if (nread != sizeof(ki))
        fprintf(stderr, "Error: Read unexpected amount of data 0x%lx\n", nread);
    else {
        // fprintf(stderr, "nread 0x%lx\n", nread);
        // fprintf(stderr, "val 0x%x type %c%c%c%c attr 0x%x\n", ki.v32,
        //     ki.type[0], ki.type[1], ki.type[2], ki.type[3],
        //     ki.attr);

        memcpy(val->key, key.c_str(), 5);
        memcpy(val->dataType, ki.type, sizeof(ki.type));
        val->dataType[4] = '\0';

        res = DeviceIoControl(macHal, MacHalControlCode::READ_BYTES,
            name, sizeof(name),
            val->bytes, sizeof(val->bytes),
            &nread, nullptr);
        val->dataSize = lenFromType(*val);
        // fprintf(stderr, "nread 0x%lx type %s size 0x%x\n", nread, val->dataType, val->dataSize);
    }

    return res;
}

bool SMCWriteKey(SMCVal_t& writeVal) {
    SMCVal_t readVal;

    /* Read to determine type and size */
    if (!SMCReadKey(writeVal.key, &readVal))
        return false;

    if (readVal.dataSize != writeVal.dataSize) {
        writeVal.dataSize = readVal.dataSize;
    }

    static uint8_t buf[sizeof(writeVal.key) + sizeof(writeVal.bytes)];
    memcpy(buf, writeVal.key, sizeof(writeVal.key));
    memcpy(&buf[sizeof(writeVal.key)], writeVal.bytes, writeVal.dataSize);

    auto res = DeviceIoControl(macHal, MacHalControlCode::WRITE_BYTES,
            buf, sizeof(writeVal.key) + writeVal.dataSize,
            nullptr, 0,
            nullptr, nullptr);
    if (!res)
        fprintf(stderr, "Error: Failed to write with 0x%lx\n", GetLastError());

    return res;
}

static bool nameForIdx(uint32_t idx, char out[5]) {
    uint8_t in[4] {};

    memcpy(in, &idx, sizeof(idx));

    DWORD nread {};
    auto res = DeviceIoControl(macHal, MacHalControlCode::READ_NAME_IDX,
        in, sizeof(in),
        out, 5,
        &nread, nullptr);

    return res;
}

void SMCGetKeys(std::vector<std::string> &keys) {
    uint32_t totalKeys = SMCReadIndexCount();

    for (uint32_t i = 0; i < totalKeys; i++) {
        char name[5];
        if (!nameForIdx(i, name))
            continue;

        keys.push_back(name);
    }
}
