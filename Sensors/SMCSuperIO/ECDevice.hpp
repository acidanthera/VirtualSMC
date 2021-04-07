//
//  ECDevice.hpp
//
//  Sensors implementation for EC SuperIO device
//
//

#ifndef _ECDEVICE_HPP
#define _ECDEVICE_HPP

#include "SuperIODevice.hpp"

namespace EC {
	// Intel PCH EC interface works over KBD ports. They are specified in H_EC (PNP0C09) device in DSDT,
	// but are de-facto hardcoded in all Intel boards.
	static constexpr uint8_t EC_D_PORT = 0x62;  // Data port
	static constexpr uint8_t EC_C_PORT = 0x66;  // Command & Status port

	// RAM area size over PMIO.
	static constexpr uint32_t EC_PMIO_SIZE = 0x100;

	// Status port bits read from status port. These are part of ACPI specification.
	static constexpr uint8_t EC_S_OVR_TMP = 0x80; // Current CPU temperature exceeds the threshold (non-standard)
	static constexpr uint8_t EC_S_SMI_EVT = 0x40; // SMI event is pending
	static constexpr uint8_t EC_S_SCI_EVT = 0x20; // SCI event is pending
	static constexpr uint8_t EC_S_BURST   = 0x10; // EC is in burst mode or normal mode
	static constexpr uint8_t EC_S_CMD     = 0x08; // Byte in data register is command/data
	static constexpr uint8_t EC_S_IGN     = 0x04; // Ignored
	static constexpr uint8_t EC_S_IBF     = 0x02; // Input buffer is full/empty
	static constexpr uint8_t EC_S_OBF     = 0x01; // Output buffer is full/empty

	// Reasonable I/O timeout for EC interaction.
	static constexpr uint32_t EC_TIME_OUT_US = 0x20000;
	static constexpr uint32_t EC_DELAY_US = 15;

	// Commands issued to EC interface. While these are standardised in ACPI, many more are non-standard.
	static constexpr uint8_t EC_C_READ_MEM = 0x80;     // Read the EC memory
	static constexpr uint8_t EC_C_WRITE_MEM = 0x81;    // Write the EC memory

	// Firmware-specific addresses in EC RAM. These are true for Intel NUCs ????.
	static constexpr uint8_t EC_CPU_FAN_RPM_H = 0x73;
	static constexpr uint8_t EC_CPU_FAN_RPM_L = 0x74;

	// Information about LGMR register on LPC device.
	static constexpr uint32_t PCI_DEVICE_NUMBER_PCH_LPC = 31;
	static constexpr uint32_t PCI_FUNCTION_NUMBER_PCH_LPC = 0;
	static constexpr uint32_t R_LPC_CFG_LGMR = 0x98;
	static constexpr uint32_t B_LPC_CFG_LGMR_MA = 0xFFFF0000;
	static constexpr uint32_t B_LPC_CFG_LGMR_LMRD_EN = 1;
	static constexpr uint32_t B_LPC_CFG_LGMR_SIZE = 0x10000;

	class ECDevice : public SuperIODevice {

	protected:
		/**
		 * Protocol support status.
		 */
		bool supportsPMIO {false};
		bool supportsMMIO {false};

		IOMemoryDescriptor *mmioDesc {nullptr};
		IOMemoryMap *mmioMap {nullptr};
		volatile uint8_t *mmioArea {nullptr};

		/**
		 *  Configure EC access over MMIO or PMIO.
		 */
		bool setupMMIO();
		bool setupPMIO();

		/**
		 *  Low-level PMIO access.
		 */
		bool sendECData(uint8_t data);
		bool receiveECData(uint8_t &data);
		bool sendECCommand(uint8_t command);

		/**
		 *  Dump PMIO or MMIO memory.
		 */
		void dumpMemory();

		/**
		 *  Initialize created device instance.
		 */
		bool initialize(SMCSuperIO* sio) {
			SuperIODevice::initialize(0, 0, sio);
			if (supportsMMIO && setupMMIO())
				return true;
			return supportsPMIO && setupPMIO();
		}
		
		/**
		 *  Subclasses to implement key setup
		 */
		virtual void setupVoltageKeys(VirtualSMCAPI::Plugin &vsmcPlugin) {}
		virtual void setupTemperatureKeys(VirtualSMCAPI::Plugin &vsmcPlugin) {}
		
		/**
		 *  Default read values
		 */
		virtual float tachometerRead(uint8_t index) { return 0; }
		virtual float voltageRead(uint8_t index) { return 0; }
		virtual float temperatureRead(uint8_t index) { return 0; }

	public:
		/**
		 *  Device access
		 */
		uint8_t readBytePMIO(uint8_t addr);
		void writeBytePMIO(uint8_t addr, uint8_t value);

		uint16_t readBigWordMMIO(uint32_t addr) {
			return (mmioArea[addr] << 8) | mmioArea[addr + 1];
		}
		uint16_t readLittleWordMMIO(uint32_t addr) {
			return (mmioArea[addr + 1] << 8) | mmioArea[addr];
		}
		uint16_t readBigWordPMIO(uint8_t addr) {
			return (readBytePMIO(addr) << 8) | readBytePMIO(addr + 1);
		}
		uint16_t readLittleWordPMIO(uint8_t addr) {
			return (readBytePMIO(addr + 1) << 8) | readBytePMIO(addr);
		}

		uint16_t readValue(uint32_t addr, bool isWord, bool isBig) {
			if (!isWord) {
				if (mmioArea != nullptr)
					return mmioArea[addr];
				return readBytePMIO(addr);
			}

			if (mmioArea != nullptr) {
				if (isBig)
					return readBigWordMMIO(addr);
				return readLittleWordMMIO(addr);
			}

			if (isBig)
				return readBigWordPMIO(addr);
			return readLittleWordPMIO(addr);
		}

		/**
		 *  Overrides
		 */
		void setupKeys(VirtualSMCAPI::Plugin &vsmcPlugin) override;

		/**
		 *  Extra non-standard keys for plugins.
		 */
		virtual void setupExtraKeys(VirtualSMCAPI::Plugin &vsmcPlugin) {
			setupVoltageKeys(vsmcPlugin);
			setupTemperatureKeys(vsmcPlugin);
		}

		/**
		 *  Ctor
		 */
		ECDevice() = default;

		/**
		 *  Device factory
		 */
		static SuperIODevice* detect(SMCSuperIO* sio, const char *name, IORegistryEntry *lpc);
	};
}

#endif // _ITEDEVICE_HPP
