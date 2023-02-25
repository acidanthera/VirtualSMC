//
//  smcread.c
//  smcread
//
//  Copyright © 2017 vit9696. All rights reserved.
//
//  For use to dump libSMC.dylib add the following linker flags:
//  -Wl,-sectcreate,__RESTRICT,__restrict,$(PROJECT_DIR)/Tools/smcread/restrict -Wl,-segaddr,__RESTRICT,0x7FFF00000000
//

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <sys/stat.h>
#include <IOKit/IOKitLib.h>

typedef struct {
	char key[4];
	uint8_t attr;
	uint8_t len;
	uint16_t zero;
	/* Among various types there are 'ui8 ' and 'ui8\0', hopefully they are same */
	char type[4];
	uint32_t handler;
} KeyDescr;

typedef union __attribute__((packed)) {
	uint8_t raw[6];
	struct __attribute__((packed)) {
		uint8_t major;
		uint8_t minor;
		uint8_t module;
		uint8_t unk;
		uint8_t patch1;
		uint8_t patch2;
	} p;
} FirmwareRevision;

// Thanks to FredWst for this struct
typedef struct __attribute__((packed)) {
	FirmwareRevision rev;
	uint8_t sbranch[8];
	uint8_t platform[8];
	uint16_t unk1;
	uint8_t adler32[4];
	uint32_t unk2;
} FirmwareInfo;

typedef struct {
	uint32_t pubnum;
	uint32_t privnum;
} KeyInfo;

typedef struct {
	bool KEY;
	bool REV;
	bool RVBF;
	bool RVUF;
	bool RBr;
	bool RPlt;
	bool LDKN;
} KeyPresence;

// Thanks for this to ionescu (https://www.youtube.com/watch?v=nSqpinjjgmg)
enum KeyAttribute {
	// Private variables cannot be written?
	ATTR_PRIVATE_WRITE  = 0x1,
	ATTR_PRIVATE_READ   = 0x2,
	ATTR_ATOMIC         = 0x4,
	ATTR_CONST          = 0x8,
	ATTR_FUNCTION       = 0x10,
	ATTR_UNK20          = 0x20,
	ATTR_WRITE          = 0x40,
	ATTR_READ           = 0x80,
};

// Thanks for these structures and defines to devnull

#define KERNEL_INDEX_SMC      2

#define SMC_CMD_READ_BYTES    5
#define SMC_CMD_WRITE_BYTES   6
#define SMC_CMD_READ_INDEX    8
#define SMC_CMD_READ_KEYINFO  9
#define SMC_CMD_READ_PLIMIT   11
#define SMC_CMD_READ_VERS     12

typedef struct {
	uint8_t major;
	uint8_t minor;
	uint8_t build;
	uint8_t reserved;
	uint16_t release;
} SMCKeyVersion;

typedef struct {
	uint16_t version;
	uint16_t length;
	uint32_t cpuPLimit;
	uint32_t gpuPLimit;
	uint32_t memPLimit;
} SMCKeyLimits;

typedef union {
	char name[4];
	uint32_t raw;
} SMCKeyName;

typedef union {
	char type[4];
	uint32_t raw;
} SMCKeyType;

typedef struct {
	uint32_t dataSize;
	SMCKeyType dataType;
	uint8_t dataAttributes;
} SMCKeyInfo;

typedef struct {
	SMCKeyName key;
	SMCKeyVersion vers;
	SMCKeyLimits pLimitData;
	SMCKeyInfo keyInfo;
	uint8_t result; // aka smcID, allows to invoke read/write to SMC at index
	uint8_t status;
	uint8_t data8;
	uint32_t data32;
	uint8_t bytes[32];
} SMCKey;

static void printMacosVersion(void) {
	FILE *product = popen("/usr/bin/sw_vers -productVersion", "r");

	if (!product) {
		puts("Unable to detect os version");
		return;
	}

	char osver[32];
	int major = 0, minor = 0, patch = 0;
	if (fgets(osver, sizeof(osver), product) != NULL &&
		sscanf(osver, "%d.%d.%d", &major, &minor, &patch)) {
		const char *codename = "macOS";
		if (major == 10 && minor < 8)
			codename = "Mac OS X";
		else if (major == 10 && minor < 12)
			codename = "OS X";
		printf("%s %d.%d.%d", codename, major, minor, patch);
	} else {
		printf("Unknown os version");
	}

	pclose(product);

	product = popen("/usr/bin/sw_vers -buildVersion", "r");

	if (product && fgets(osver, sizeof(osver), product) != NULL) {
		const char *ver = strtok(osver, "\n");
		printf(" (%s)\n", ver ? ver : "null");
	} else {
		printf(" (unknown)\n");
	}

	if (product)
		pclose(product);
}

static const char *getAttr(uint8_t attr) {
	static char attrbuf[1024];
	
	attrbuf[0] = '\0';
	if (attr & ATTR_PRIVATE_WRITE)
		strlcat(attrbuf, "ATTR_PRIVATE_WRITE|", sizeof(attrbuf));
	if (attr & ATTR_PRIVATE_READ)
		strlcat(attrbuf, "ATTR_PRIVATE_READ|", sizeof(attrbuf));
	if (attr & ATTR_ATOMIC)
		strlcat(attrbuf, "ATTR_ATOMIC|", sizeof(attrbuf));
	if (attr & ATTR_CONST)
		strlcat(attrbuf, "ATTR_CONST|", sizeof(attrbuf));
	if (attr & ATTR_FUNCTION)
		strlcat(attrbuf, "ATTR_FUNCTION|", sizeof(attrbuf));
	if (attr & ATTR_UNK20)
		strlcat(attrbuf, "ATTR_UNK20|", sizeof(attrbuf));
	if (attr & ATTR_WRITE)
		strlcat(attrbuf, "ATTR_WRITE|", sizeof(attrbuf));
	if (attr & ATTR_READ)
		strlcat(attrbuf, "ATTR_READ|", sizeof(attrbuf));
	
	size_t len = strlen(attrbuf);
	if (len > 0)
		attrbuf[len-1] = '\0';
	
	return attrbuf;
}

static uint8_t *convertUpdate(char *buf, size_t *sz, char revrecover[64]) {
	uint8_t *bin = NULL;
	size_t cursz = 0;
	// Current address
	size_t addr = 0;
	// By last written address
	size_t wrsz = 0;
	
	for (size_t i = 0, len = 0; i < *sz; i += len + 1) {
		char *line = &buf[i];
		char *endl = strchr(line, '\n');
		
		if (endl) {
			*endl = '\0';
			len = endl - line;
			
			for (size_t j = 0, numb = 0; j < len; j++) {
				if (j == 0) {
					if (line[j] == 'D' && line[j+1] == ':') {
						j += 2;
						char *end = NULL;
						addr = strtoul(&line[j], &end, 16);
						if (end) j += end - &line[j] - 1;
						else     break;
					} else if (line[j] != '+') {
						break;
					}
				} else if (numb == 0 && line[j] == ':') {
					j++;
					char *end = NULL;
					numb = strtoul(&line[j], &end, 10);
					if (end && numb > 0) j += end - &line[j];
					else break;
				} else if (numb > 0) {
					// Fill the gaps with 0xFF
					if (cursz < addr + numb) {
						size_t extra = numb > 0x1000 ? numb : 0x1000;
						uint8_t *newbuf = realloc(bin, addr + extra);
						if (newbuf) {
							bin = newbuf;
							cursz = addr + extra;
							memset(&bin[addr], 0xFF, extra);
						} else {
							fprintf(stderr, "Failed to allocate %zu bytes for smc conversion\n", addr + extra);
							free(bin);
							return NULL;
						}
					}
					
					if (!bin) {
						// silence blind clang
						return NULL;
					}
					
					// Update the actual file size
					if (wrsz < addr + numb)
						wrsz = addr + numb;
					
					// Convert the bytes
					for (size_t k = 0; k < numb; k++) {
						if (isxdigit(line[j]) && isxdigit(line[j+1])) {
							bin[addr+k] = (digittoint(line[j]) << 4) | digittoint(line[j+1]);
							//fprintf(stderr, "[0x%08lX] %02X\n", addr+k, bin[addr+k]);
						} else {
							fprintf(stderr, "Failed to parse hex string for smc conversion:\n%s\n", line);
							free(bin);
							return NULL;
						}
							
						j += 2;
					}
					
					addr += numb;
					break;
				}
			}
		} else {
			break;
		}
	}
	
	// Attempt to recover firmware info by first branch
	if (wrsz > sizeof(FirmwareInfo)) {
		FirmwareInfo *fwinfo = (FirmwareInfo *)&bin[wrsz - sizeof(FirmwareInfo)];
		uint32_t major = 0, minor = 0, flag = 0, patch = 0;
		if (sscanf(buf, "# Version: %x.%2x%1x%x", &major, &minor, &flag, &patch) > 0) {
			if (fwinfo->rev.raw[0] == 0xFF) {
				fwinfo->rev.p.major = major;
				fwinfo->rev.p.minor = minor;
				fwinfo->rev.p.module = flag;
				fwinfo->rev.p.patch1 = patch;
				fwinfo->rev.p.patch2 = 0;
				snprintf(revrecover, 64, " (recovered %x.%x%x%x)", major, minor, flag, patch);
			} else {
				snprintf(revrecover, 64, " (org %x.%x%x%x)", major, minor, flag, patch);
			}
		}
	}
	
	*sz = wrsz;
	return bin;
}

static io_connect_t smc_connect(void) {
	io_connect_t con = 0;
	CFMutableDictionaryRef dict = IOServiceMatching("AppleSMC");
	if (dict) {
		io_iterator_t iterator = 0;
		kern_return_t result = IOServiceGetMatchingServices(kIOMasterPortDefault, dict, &iterator);
		if (result == kIOReturnSuccess && IOIteratorIsValid(iterator)) {
			io_object_t device = IOIteratorNext(iterator);
			if (device) {
				result = IOServiceOpen(device, mach_task_self(), 0, &con);
				if (result != kIOReturnSuccess)
					fprintf(stderr, "Unable to open AppleSMC service %08X\n", result);
				IOObjectRelease(device);
			} else {
				fprintf(stderr, "Unable to locate AppleSMC device\n");
			}
			IOObjectRelease(iterator);
		} else {
			fprintf(stderr, "Unable to get AppleSMC service %08X\n", result);
		}
	} else {
		fprintf(stderr, "Unable to create AppleSMC matching dict\n");
	}
	
	return con;
}

static bool smc_read_name(io_connect_t con, uint32_t index, SMCKeyName *key) {
	SMCKey input = {};
	input.data8 = SMC_CMD_READ_INDEX;
	input.data32 = index;
	
	SMCKey output = {};
	size_t output_sz = sizeof(output);
	
	kern_return_t result = IOConnectCallStructMethod(con, KERNEL_INDEX_SMC, &input, sizeof(input), &output, &output_sz);
	if (result == kIOReturnSuccess) {
		*key = output.key;
		return true;
	}
	
	return false;
}

static uint8_t smc_read_key(io_connect_t con, SMCKeyName key, SMCKeyInfo *info, uint8_t data[32]) {
	SMCKey input = {};
	input.key = key;
	input.data8 = SMC_CMD_READ_KEYINFO;
	
	SMCKey output = {};
	size_t output_sz = sizeof(output);
	
	kern_return_t result = IOConnectCallStructMethod(con, KERNEL_INDEX_SMC, &input, sizeof(input), &output, &output_sz);
	if (result == kIOReturnSuccess) {
		if (info) *info = output.keyInfo;
		
		input.keyInfo.dataSize = output.keyInfo.dataSize;
		input.data8 = SMC_CMD_READ_BYTES;
		output_sz = sizeof(output);
		
		result = IOConnectCallStructMethod(con, KERNEL_INDEX_SMC, &input, sizeof(input), &output, &output_sz);
		if (result == kIOReturnSuccess) {
			if (output.result == 0 && data)
				memcpy(data, output.bytes, sizeof(output.bytes));
			return output.result;
		} else {
			fprintf(stderr, "Unable to read smc value %08X\n", result);
		}
	} else {
		fprintf(stderr, "Unable to read smc info %08X\n", result);
	}
	
	return 0xff;
}

static int smc_dump_keys(void) {
	io_connect_t con = smc_connect();
	if (con) {
		uint8_t data[32] = {};
		SMCKeyName key = {.raw = '#KEY'};
		
		SMCKeyName knownHidden[] = {
			{.raw = '____'},
			{.raw = 'OSK0'},
			{.raw = 'OSK1'},
			{.raw = 'MSSN'},
			{.raw = 'CRDP'},
			{.raw = 'FPOR'},
			{.raw = 'MOJO'},
			{.raw = 'MOJE'},
			{.raw = 'MOJD'},
			{.raw = 'MOJC'},
			{.raw = 'KPCT'},
			{.raw = 'KPPW'},
			{.raw = 'KPVR'},
			{.raw = 'KPST'},
			{.raw = 'zCRS'}
		};
		
		uint8_t result = smc_read_key(con, key, NULL, data);
		if (result == 0) {
			printMacosVersion();

			uint32_t numk = __builtin_bswap32(((uint32_t)data[0]) | ((uint32_t)data[1] << 8U) | ((uint32_t)data[2] << 16U) | ((uint32_t)data[3] << 24U));
			printf("Public keys (%u):\n", numk);

			bool has_const = false;
			bool allattr_null = true;
			for (uint32_t i = 0; i < numk; i++) {
				if (smc_read_name(con, i, &key)) {
					SMCKeyInfo info = {};
					result = smc_read_key(con, key, &info, data);
					printf("[%c%c%c%c] type [%c%c%c%c] %02X%02X%02X%02X len [%2u] attr [%02X] -> ",
						   key.name[3], key.name[2], key.name[1], key.name[0],
						   info.dataType.type[3], info.dataType.type[2], info.dataType.type[1], info.dataType.type[0] == '\0' ? '?' : info.dataType.type[0],
						   info.dataType.type[3], info.dataType.type[2], info.dataType.type[1], info.dataType.type[0],
						   info.dataSize, info.dataAttributes);

					if (result == 0)
						for (uint32_t j = 0; j < info.dataSize; j++)
							printf("%02X", data[j]);
					else
						printf("NOT READABLE, code %02X", result);

					printf("\n");

					if (info.dataAttributes)
						allattr_null = false;

					if (info.dataAttributes & ATTR_CONST)
						has_const = true;

					for (uint32_t j = 0; j < sizeof(knownHidden)/sizeof(knownHidden[0]); j++) {
						if (knownHidden[j].raw && knownHidden[j].raw == key.raw) {
							knownHidden[j].raw = 0;
							break;
						}
					}
				} else {
					fprintf(stderr, "Unable to read smc key name at %u index\n", i);
				}
			}
			
			if (allattr_null)
				printf("All key attributes are null, your SMC implementation is broken!\n");

			bool found_hidden = false;
			printf("\nHidden keys (?):\n");
			for (uint32_t i = 0; i < sizeof(knownHidden)/sizeof(knownHidden[0]); i++) {
				SMCKeyInfo info;
				if (knownHidden[i].raw) {
					result = smc_read_key(con, knownHidden[i], &info, data);
					if (result == 0 || result == 0x85 /* SmcNotReadable */) {
						printf("[%c%c%c%c] type [%c%c%c%c] %02X%02X%02X%02X len [%2u] attr [%02X] -> ",
							   knownHidden[i].name[3], knownHidden[i].name[2], knownHidden[i].name[1], knownHidden[i].name[0],
							   info.dataType.type[3], info.dataType.type[2], info.dataType.type[1], info.dataType.type[0] == '\0' ? '?' : info.dataType.type[0],
							   info.dataType.type[3], info.dataType.type[2], info.dataType.type[1], info.dataType.type[0],
							   info.dataSize, info.dataAttributes);
						
						if (result == 0)
							for (uint32_t j = 0; j < info.dataSize; j++)
								printf("%02X", data[j]);
						else
							printf("NOT READABLE");
						
						printf("\n");
						
						found_hidden = true;

						if (info.dataAttributes & ATTR_CONST)
							has_const = true;
					}
				}
			}
			
			if (!found_hidden)
				printf("No hidden keys, your SMC implementation is broken!\n");

			if (has_const)
				printf("Const attribute is exposed, your SMC implementation is broken!\n");


			return 0;
		} else {
			fprintf(stderr, "Unable to obtain smc key amount %02X\n", result);
		}
		
		IOServiceClose(con);
	}
	
	return -1;
}

struct PlatformKeyDescriptions {
	char key[4];
	uint8_t index;
	char description[256];
	uint8_t len;
	uint16_t pad1;
	char type[4];
	uint32_t pad2; /* added in 10.13.4 */
} __attribute__((packed));

struct PlatformStructLookup {
	char branch[16];
	struct PlatformKeyDescriptions *descr[2];
	uint32_t descrn[2];
	uint32_t pad[2];
};

static int smc_dump_lib_keys(const char *smcpath) {
	void *smc_lib = dlopen(smcpath, RTLD_LAZY);

	if (smc_lib) {
		struct PlatformStructLookup *lookup_arr = (struct PlatformStructLookup *)dlsym(smc_lib, "AccumulatorPlatformStructLookupArray");

		if (!lookup_arr) {
			// This library is normally in dyld shared cache, so we have to use DYLD_SHARED_REGION=avoid envvar.
			// As a more convenient workaround we currently compile with an overlapping segment.
			fprintf(stderr, "Unable to solve AccumulatorPlatformStructLookupArray symbol, trying to brute-force...\n");

			uint8_t *start = (uint8_t *)dlsym(smc_lib, "SMCReadKey");
			uint8_t *ptr = start;
			char first[16] = "m87";
			while (ptr && memcmp(ptr, first, sizeof(first)) != 0) {
				//printf("%08X: %02X\n", ptr-start, ptr[0]);
				ptr++;
			}

			lookup_arr = (struct PlatformStructLookup *)ptr;
		}

		if (lookup_arr) {
			bool stop = false;
			while (lookup_arr->branch[0] != '\0' && isascii(lookup_arr->branch[0]) && !stop) {
				printf("Dumping keys for %.4s...\n", lookup_arr->branch);

				for (uint32_t i = 0; i < 2 && !stop; i++) {
					printf(" Set %u has %u keys:\n", i, lookup_arr->descrn[i]);
					for (uint32_t j = 0; j < lookup_arr->descrn[i]; j++) {
						struct PlatformKeyDescriptions *key = &lookup_arr->descr[i][j];
						if (key->pad1 != 0 || key->pad2 != 0) {
							stop = true;
							break;
						}
						printf(" [%c%c%c%c] type [%c%c%c%c] %02X%02X%02X%02X len [%2u] idx [%3u]: %.256s\n",
							   key->key[3] == '\0' ? ' ' : key->key[3], key->key[2] == '\0' ? ' ' : key->key[2],
							   key->key[1] == '\0' ? ' ' : key->key[1], key->key[0] == '\0' ? ' ' : key->key[0],
							   key->type[3], key->type[2], key->type[1], key->type[0] == '\0' ? '?' : key->type[0],
							   key->type[3], key->type[2], key->type[1], key->type[0],
							   key->len, key->index, key->description);
					}
				}

				lookup_arr++;
			}

			dlclose(smc_lib);
			return 0;
		} else {
			fprintf(stderr, "Unable to locate lookup array in libSMC.dylib!\n");
			dlclose(smc_lib);
		}
	} else {
		fprintf(stderr, "Unable to open libSMC.dylib!\n");
	}

	return -1;
}

static bool key_type_valid[256] = {
	/* 00 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 10 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 20 */ 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0,
	/* 30 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
	/* 40 */ 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	/* 50 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	/* 60 */ 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	/* 70 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
	/* 80 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 90 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* A0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* B0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* C0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* D0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* E0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* F0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

int main(int argc, const char *argv[]) {
	if (argc < 2) {
		fprintf(stderr,
		"smcread 1.0\n\n"
		"Usage:\n"
		"smcread smc.bin\n"
		"smcread update.smc [smc.bin]\n"
		"smcread -s\n\n"
		"smcread -l [path to libSMC.dylib]\n\n"
		"Note:\n"
		"This utility tries to reconstruct certain key values.\n"
		"Be aware that some keys belong to different files:\n"
		" - RVUF is from flasher_update.smc\n"
		" - RVBF is from from flasher_base.smc\n"
		" - REV  is from Mac-XXX.smc\n");
		return -1;
	}
	
	if (!strcmp(argv[1], "-s"))
		return smc_dump_keys();

	if (!strcmp(argv[1], "-l"))
		return smc_dump_lib_keys(argc > 2 ? argv[2] : "/usr/lib/libSMC.dylib");
	
	FILE *fh = fopen(argv[1], "rb");
	if (!fh) {
		fprintf(stderr, "Unable to open %s for reading\n", argv[1]);
		return -1;
	}
	
	struct stat s;
	if (fstat(fileno(fh), &s)) {
		fprintf(stderr, "Unable to obtain %s size\n", argv[1]);
		fclose(fh);
		return -1;
	}
	
	size_t sz = (size_t)s.st_size;
	uint8_t *buf = calloc(1, sz+1);
	if (!buf) {
		fprintf(stderr, "Unable to allocate %zu bytes\n", sz);
		fclose(fh);
		return -1;
	}
	
	if (fseek(fh, 0, SEEK_SET) || fread(buf, sz, 1, fh) != 1) {
		fprintf(stderr, "Unable to read %s\n", argv[1]);
		fclose(fh);
		free(buf);
		return -1;
	}
	
	fclose(fh);
	
	char revrecover[64] = {};
	if (buf[0] == '#' && buf[1] == ' ' && buf[2] == 'V' && buf[3] == 'e') {
		uint8_t *bin = convertUpdate((char *)buf, &sz, revrecover);
		if (!bin) {
			fprintf(stderr, "Unable to parse smc update %s\n", argv[1]);
			free(buf);
			return -1;
		}
		
		free(buf);
		buf = bin;
		
		if (argc > 2) {
			int rmcode = remove(argv[2]);
			fh = fopen(argv[2], "wb");
			if (fh) {
				if (fseek(fh, 0, SEEK_SET) || fwrite(buf, sz, 1, fh) != 1)
					fprintf(stderr, "Unable to write %s", argv[2]);
				fclose(fh);
			} else {
				fprintf(stderr, "Unable to open %s for writing\n", argv[2]);
				if (rmcode)
					fprintf(stderr, "Make sure %s does not exist already\n", argv[2]);
			}
		}
	}
	
	size_t smcoff = 0;
	for (size_t i = sizeof(KeyInfo); smcoff == 0 && i + sizeof(KeyDescr) < sz; i++) {
		if (!memcmp(&buf[i], "#KEY", strlen("#KEY")))
			smcoff = i - sizeof(KeyInfo);
	}
	
	// Flasher updates (either update or base) have no #KEY, as well as private keys
	bool flasher = !smcoff;
	
	// We need to bruteforce a table for flasher
	if (flasher) {
		for (size_t i = sizeof(KeyInfo); smcoff == 0 && i + sizeof(KeyDescr)*3 < sz; i++) {
			KeyDescr *descr = (KeyDescr *)&buf[i];
			for (size_t j = 0; j < 3; j++) {
				bool valid = key_type_valid[descr->key[0]] && key_type_valid[descr->key[1]] && key_type_valid[descr->key[2]] && key_type_valid[descr->key[3]] &&
				             key_type_valid[descr->type[0]] && key_type_valid[descr->type[1]] && key_type_valid[descr->type[2]] &&
				             (key_type_valid[descr->type[3]] || descr->type[3] == '\0') && descr->zero == 0;
				if (!valid)
					break;
				else if (j == 2)
					smcoff = i - sizeof(KeyInfo);
			}
		}
		
		if (smcoff) {
			KeyInfo *info = (KeyInfo *)&buf[smcoff];
			info->privnum = 0;
			info->pubnum = 0;
			
			bool valid = true;
			KeyDescr *descr = (KeyDescr *)&buf[smcoff + sizeof(KeyInfo)];
			while (valid && (uint8_t *)&descr[1] < &buf[sz]) {
				valid = key_type_valid[descr->key[0]] && key_type_valid[descr->key[1]] && key_type_valid[descr->key[2]] && key_type_valid[descr->key[3]] &&
					    key_type_valid[descr->type[0]] && key_type_valid[descr->type[1]] && key_type_valid[descr->type[2]] &&
				        (key_type_valid[descr->type[3]] || descr->type[3] == '\0') && descr->zero == 0;
				if (valid) {
					info->pubnum++;
					descr++;
				}
			}
		}
	}
	
	if (!smcoff) {
		fprintf(stderr, "Unable to locate smc key table in %s\n", argv[1]);
		free(buf);
		return -1;
	}
	
	KeyInfo *info = (KeyInfo *)&buf[smcoff];
	
	// Older SMC FW is BE, detect this by assuming that at least 2 keys (OSK0, OSK1 are private)
	if (info->privnum != 0 && (info->privnum & 0xFFFF) == 0) {
		info->pubnum = __builtin_bswap32(info->pubnum);
		info->privnum = __builtin_bswap32(info->privnum);
	}
	
	size_t total = info->pubnum * sizeof(KeyDescr) + info->privnum * sizeof(KeyDescr);
	if (sz - smcoff < total) {
		fprintf(stderr, "Unable to parse smc key table in %s\n", argv[1]);
		free(buf);
		return -1;
	}
	
	smcoff += sizeof(KeyInfo);
	KeyPresence pres = {};
	
	printf("Public keys (%u):\n", info->pubnum);
	for (uint32_t i = 0; i < info->pubnum; i++, smcoff += sizeof(KeyDescr)) {
		KeyDescr *key = (KeyDescr *)&buf[smcoff];
		printf("[%.4s] type [%c%c%c%c] %02X%02X%02X%02X len [%2u] attr [%02X] handler [%08X] -> %s\n",
			   key->key, key->type[0], key->type[1], key->type[2], key->type[3] == '\0' ? '?' : key->type[3],
			   key->type[0], key->type[1], key->type[2], key->type[3],
			   key->len, key->attr, key->handler, getAttr(key->attr));
		
		if (key->key[0] == '#' && key->key[1] == 'K' && key->key[2] == 'E' && key->key[3] == 'Y')
			pres.KEY = true;
		else if (key->key[0] == 'R' && key->key[1] == 'E' && key->key[2] == 'V' && key->key[3] == ' ')
			pres.REV = true;
		else if (key->key[0] == 'R' && key->key[1] == 'V' && key->key[2] == 'B' && key->key[3] == 'F')
			pres.RVBF = true;
		else if (key->key[0] == 'R' && key->key[1] == 'V' && key->key[2] == 'U' && key->key[3] == 'F')
			pres.RVUF = true;
		else if (key->key[0] == 'R' && key->key[1] == 'B' && key->key[2] == 'r' && key->key[3] == ' ')
			pres.RBr = true;
		else if (key->key[0] == 'R' && key->key[1] == 'P' && key->key[2] == 'l' && key->key[3] == 't')
			pres.RPlt = true;
		else if (key->key[0] == 'L' && key->key[1] == 'D' && key->key[2] == 'K' && key->key[3] == 'N')
			pres.LDKN = true;
		
		//keyType[key->key[0]] = keyType[key->key[1]] = keyType[key->key[2]] = keyType[key->key[3]] = true;
		//keyType[key->type[0]] = keyType[key->type[1]] = keyType[key->type[2]] = keyType[key->type[3]] = true;
	}
	
	printf("\nHidden keys (%u):\n", info->privnum);
	for (uint32_t i = 0; i < info->privnum; i++, smcoff += sizeof(KeyDescr)) {
		KeyDescr *key = (KeyDescr *)&buf[smcoff];
		printf("[%.4s] type [%c%c%c%c] %02X%02X%02X%02X len [%2u] attr [%02X] handler [%08X] -> %s\n",
			   key->key, key->type[0], key->type[1], key->type[2], key->type[3] == '\0' ? '?' : key->type[3],
			   key->type[0], key->type[1], key->type[2], key->type[3],
			   key->len, key->attr, key->handler, getAttr(key->attr));
		
		//keyType[key->key[0]] = keyType[key->key[1]] = keyType[key->key[2]] = keyType[key->key[3]] = true;
		//keyType[key->type[0]] = keyType[key->type[1]] = keyType[key->type[2]] = keyType[key->type[3]] = true;
	}
	
	/*for (uint32_t i = 0; i < 256; i++) {
		printf("%d,", keyType[i]);
		if (i > 0 && ((i+1) % 16 == 0))
			printf("\n");
		else
			printf(" ");
	}
	
	for (uint32_t i = 0; i < 256; i++) {
		printf("%02X", i);
		if (i > 0 && ((i+1) % 16 == 0))
			printf("\n");
		else
			printf(" ");
	}*/
	
	if (sz > sizeof(FirmwareInfo)) {
		printf("\nKey values:\n");
		FirmwareInfo *fwinfo = (FirmwareInfo *)&buf[sz - sizeof(FirmwareInfo)];
		if (pres.KEY)
			printf("[#KEY] is %08X -> %u\n", __builtin_bswap32(info->pubnum), info->pubnum);
		// Base flasher revision (RVBF) and update flasher revision (RVUF) often equal REV but that is not a rule
		// RVUF should be taken from flasher_update.smc, RVBF from flasher_base.smc, REV from Mac-XXX.smc
		// patch is never two bytes fortunately
		if (pres.RVBF)
			printf("[RVBF] is %02X%02X%02X%02X%04X -> %x.%x%x%x%s\n", fwinfo->rev.p.major, fwinfo->rev.p.minor, fwinfo->rev.p.module, fwinfo->rev.p.unk,
				   fwinfo->rev.p.patch1 > 0 ? fwinfo->rev.p.patch1 : fwinfo->rev.p.patch2, fwinfo->rev.p.major, fwinfo->rev.p.minor, fwinfo->rev.p.module,
				   fwinfo->rev.p.patch1 > 0 ? fwinfo->rev.p.patch1 : fwinfo->rev.p.patch2, revrecover);
		if (pres.RVUF)
			printf("[RVUF] is %02X%02X%02X%02X%04X -> %x.%x%x%x%s\n", fwinfo->rev.p.major, fwinfo->rev.p.minor, fwinfo->rev.p.module, fwinfo->rev.p.unk,
				   fwinfo->rev.p.patch1 > 0 ? fwinfo->rev.p.patch1 : fwinfo->rev.p.patch2, fwinfo->rev.p.major, fwinfo->rev.p.minor, fwinfo->rev.p.module,
				   fwinfo->rev.p.patch1 > 0 ? fwinfo->rev.p.patch1 : fwinfo->rev.p.patch2, revrecover);
		if (pres.REV)
			printf("[REV ] is %02X%02X%02X%02X%04X -> %x.%x%x%x%s\n", fwinfo->rev.p.major, fwinfo->rev.p.minor, fwinfo->rev.p.module, fwinfo->rev.p.unk,
				   fwinfo->rev.p.patch1 > 0 ? fwinfo->rev.p.patch1 : fwinfo->rev.p.patch2, fwinfo->rev.p.major, fwinfo->rev.p.minor, fwinfo->rev.p.module,
				   fwinfo->rev.p.patch1 > 0 ? fwinfo->rev.p.patch1 : fwinfo->rev.p.patch2, revrecover);
		if (pres.LDKN)
			printf("[LDKN] is %02X -> %x\n", fwinfo->rev.raw[0], fwinfo->rev.p.major);
		if (pres.RBr && fwinfo->sbranch[0] != 0xFF) {
			printf("[RBr ] is %02X%02X%02X%02X%02X%02X%02X%02X -> %.8s\n", fwinfo->sbranch[0], fwinfo->sbranch[1], fwinfo->sbranch[2], fwinfo->sbranch[3],
				   fwinfo->sbranch[4], fwinfo->sbranch[5], fwinfo->sbranch[6], fwinfo->sbranch[7], fwinfo->sbranch);
		}
		if (pres.RPlt && fwinfo->platform[0] != 0xFF)
			printf("[RPlt] is %02X%02X%02X%02X%02X%02X%02X%02X -> %.8s\n", fwinfo->platform[0], fwinfo->platform[1], fwinfo->platform[2], fwinfo->platform[3],
				   fwinfo->platform[4], fwinfo->platform[5], fwinfo->platform[6], fwinfo->platform[7], fwinfo->platform);
		// Could actually add some region parsing here and produce valid CRC keys, but we won't be using them anyway
		if (*(uint32_t *)fwinfo->adler32 != 0xFFFFFFFF)
			printf("[CRC?] is %02X%02X%02X%02X\n", fwinfo->adler32[0], fwinfo->adler32[1], fwinfo->adler32[2], fwinfo->adler32[3]);
	}
	
	free(buf);
	return 0;
}
