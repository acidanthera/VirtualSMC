//
//  ECDevice.cpp
//
//  Sensors implementation for EC SuperIO device
//
//

#include "ECDevice.hpp"
#include "ECDeviceNUC.hpp"
#include "ECDeviceGeneric.hpp"
#include "ECDeviceDebug.hpp"
#include "SMCSuperIO.hpp"
#include "Devices.hpp"

namespace EC {

	bool ECDevice::sendECData(uint8_t data) {
		uint32_t i;

		// Await for space in the input buffer.
		auto status = ::inb(EC_C_PORT);
		for (i = 0; (status & EC_S_IBF) != 0 && i < EC_TIME_OUT_US; i += EC_DELAY_US) {
			IODelay(EC_DELAY_US);
			status = ::inb(EC_C_PORT);
		}
		if (i >= EC_TIME_OUT_US) return false;

		::outb(EC_D_PORT, data);
		return true;
	}

	bool ECDevice::receiveECData(uint8_t &data) {
		uint32_t i;

		// Await for data in the output buffer.
		auto status = ::inb(EC_C_PORT);
		for (i = 0; (status & EC_S_OBF) == 0 && i < EC_TIME_OUT_US; i += EC_DELAY_US) {
			IODelay(EC_DELAY_US);
			status = ::inb(EC_C_PORT);
		}
		if (i >= EC_TIME_OUT_US) return false;

		data = ::inb(EC_D_PORT);
		return true;
	}

	bool ECDevice::sendECCommand(uint8_t command) {
		uint32_t i;

		// Filter out remainings int the output buffer from the old commands.
		auto status = ::inb(EC_C_PORT);
		for (i = 0; (status & EC_S_OBF) != 0 && i < EC_TIME_OUT_US; i += EC_DELAY_US) {
			IODelay(EC_DELAY_US);
			::inb(EC_D_PORT);
			status = ::inb(EC_C_PORT);
		}
		if (i >= EC_TIME_OUT_US) return false;

		// Await for space in the input buffer.
		for (i = 0; (status & EC_S_IBF) != 0 && i < EC_TIME_OUT_US; i += EC_DELAY_US) {
			IODelay(EC_DELAY_US);
			status = ::inb(EC_C_PORT);
		}
		if (i >= EC_TIME_OUT_US) return false;

		::outb(EC_C_PORT, command);
		return true;
	}

	void ECDevice::dumpMemory() {
#ifdef DEBUG
		// Dump EC area for reference.
		if (mmioArea != nullptr) {
			for (uint32_t i = 0; i < 0x2000 /* B_LPC_CFG_LGMR_SIZE */; i+=16)
				DBGLOG("ssio", "%04X: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X", i,
					   mmioArea[i+0], mmioArea[i+1], mmioArea[i+2], mmioArea[i+3], mmioArea[i+4], mmioArea[i+5], mmioArea[i+6], mmioArea[i+7],
					   mmioArea[i+8], mmioArea[i+9], mmioArea[i+10], mmioArea[i+11], mmioArea[i+12], mmioArea[i+13], mmioArea[i+14], mmioArea[i+15]);
			return;
		}

		for (uint32_t i = 0; i < EC_PMIO_SIZE; i+=16)
			DBGLOG("ssio", "%04X: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X", i,
				   readBytePMIO(i+0), readBytePMIO(i+1), readBytePMIO(i+2), readBytePMIO(i+3), readBytePMIO(i+4), readBytePMIO(i+5), readBytePMIO(i+6), readBytePMIO(i+7),
				   readBytePMIO(i+8), readBytePMIO(i+9), readBytePMIO(i+10), readBytePMIO(i+11), readBytePMIO(i+12), readBytePMIO(i+13), readBytePMIO(i+14), readBytePMIO(i+15));
#endif
	}

	uint8_t ECDevice::readBytePMIO(uint8_t addr) {
		if (!sendECCommand(EC_C_READ_MEM)) return 0xFF;
		if (!sendECData(addr)) return 0xFF;
		uint8_t data = 0xFF;
		if (!receiveECData(data)) return 0xFF;
		return data;
	}

	void ECDevice::writeBytePMIO(uint8_t addr, uint8_t value) {
		if (!sendECCommand(EC_C_WRITE_MEM)) return;
		if (!sendECData(addr)) return;
		if (!sendECData(value)) return;
	}
	
	void ECDevice::setupKeys(VirtualSMCAPI::Plugin &vsmcPlugin) {
		VirtualSMCAPI::addKey(KeyFNum, vsmcPlugin.data,
			VirtualSMCAPI::valueWithUint8(getTachometerCount(), nullptr, SMC_KEY_ATTRIBUTE_CONST | SMC_KEY_ATTRIBUTE_READ));
		for (uint8_t index = 0; index < getTachometerCount(); ++index) {
			VirtualSMCAPI::addKey(KeyF0Ac(index), vsmcPlugin.data,
				VirtualSMCAPI::valueWithFp(0, SmcKeyTypeFpe2, new TachometerKey(getSmcSuperIO(), this, index)));
		}

		setupExtraKeys(vsmcPlugin);

		qsort(const_cast<VirtualSMCKeyValue *>(vsmcPlugin.data.data()), vsmcPlugin.data.size(), sizeof(VirtualSMCKeyValue), VirtualSMCKeyValue::compare);
	}

	bool ECDevice::setupMMIO() {
		auto lgmr = WIOKit::readPCIConfigValue(smcSuperIO->getParentEntry(gIOServicePlane), R_LPC_CFG_LGMR);
		DBGLOG("ssio", "EC LGMR is 0x%X", lgmr);
		if ((lgmr & B_LPC_CFG_LGMR_LMRD_EN) == 0) {
			SYSLOG("ssio", "EC LGMR address space is not enabled (0x%X)", lgmr);
			return false;
		}

		if ((lgmr & B_LPC_CFG_LGMR_MA) == 0) {
			SYSLOG("ssio", "EC LGMR address space is not available (0x%X)", lgmr);
			return false;
		}

		lgmr &= B_LPC_CFG_LGMR_MA; /* often 0xFE0B0000 or 0xFE410000 */

		mmioDesc = IOMemoryDescriptor::withPhysicalAddress(lgmr, B_LPC_CFG_LGMR_SIZE, kIODirectionIn);
		if (mmioDesc == nullptr) {
			SYSLOG("ssio", "cannot create descriptor over EC space");
			return false;
		}

		mmioDesc->prepare();
		mmioMap = mmioDesc->map();
		if (mmioMap == nullptr) {
			SYSLOG("ssio", "cannot map EC descriptor space");
			OSSafeReleaseNULL(mmioDesc);
			return false;
		}

		mmioArea = reinterpret_cast<volatile uint8_t *>(mmioMap->getVirtualAddress());
		return true;
	}

	bool ECDevice::setupPMIO() {
		// TODO: Allow port selection?
		return true;
	}

	/**
	 *  Device factory
	 */
	SuperIODevice* ECDevice::detect(SMCSuperIO* sio, const char *name, IORegistryEntry *lpc) {
		if (name[0] == '\0') {
			DBGLOG("ssio", "please inject ec-device property into LPC with the name");
			return nullptr;
		}

		DBGLOG("ssio", "ECDevice probing device %s", name);
		ECDevice *detectedDevice = static_cast<ECDevice *>(createDeviceEC(name));

		if (!detectedDevice) {
			detectedDevice = ECDeviceGeneric::detect(sio, name, lpc);
		}

#ifdef DEBUG
		bool debug = false;
		if (!detectedDevice) {
			detectedDevice = ECDeviceDebug::detect(sio, name);
			debug = true;
		}
#endif
		if (detectedDevice) {
			if (!detectedDevice->initialize(sio)) {
				SYSLOG("ssio", "cannot initialise EC either in MMIO or in PMIO mode");
				delete detectedDevice;
				return nullptr;
			}

#ifdef DEBUG
			if (debug) detectedDevice->dumpMemory();
#endif
		}
		return detectedDevice;
	}
	
} // namespace ITE
