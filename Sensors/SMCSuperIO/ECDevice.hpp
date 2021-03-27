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
	// Reasonable max values.
	static constexpr uint8_t EC_MAX_TACHOMETER_COUNT = 5;
	static constexpr uint8_t EC_MAX_VOLTAGE_COUNT = 9;
	static constexpr uint8_t EC_MAX_TEMPERATURE_COUNT = 9;

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
		/**
		 *  Tachometers
		 */
		_Atomic(uint16_t) tachometers[EC_MAX_TACHOMETER_COUNT] = { };
		/**
		 *  Voltages
		 */
		_Atomic(float) voltages[EC_MAX_VOLTAGE_COUNT] = { };

		/**
		 *  Temperatures
		 */
		_Atomic(float) temperatures[EC_MAX_TEMPERATURE_COUNT] = { };
	protected:
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
		bool initialize(bool mmio, SMCSuperIO* sio) {
			SuperIODevice::initialize(0, 0, sio);
			if (mmio) return setupMMIO();
			return setupPMIO();
		}

		/**
		 *  Implementations for tachometer reading.
		 */
		void setTachometerValue(uint8_t index, uint16_t value) override {
			if (index < getTachometerCount() && index < EC_MAX_TACHOMETER_COUNT) {
				atomic_store_explicit(&tachometers[index], value, memory_order_relaxed);
			}
		}
		uint16_t getTachometerValue(uint8_t index) override {
			if (index < getTachometerCount() && index < EC_MAX_TACHOMETER_COUNT) {
				return atomic_load_explicit(&tachometers[index], memory_order_relaxed);
			}
			return 0;
		}
		/**
		 * Reads voltage data. Invoked from update() only.
		 */
		void setVoltageValue(uint8_t index, float value) override {
			if (index < getVoltageCount() && index < EC_MAX_VOLTAGE_COUNT) {
				atomic_store_explicit(&voltages[index], value, memory_order_relaxed);
			}
		}
		float getVoltageValue(uint8_t index) override {
			if (index < getVoltageCount() && index < EC_MAX_VOLTAGE_COUNT) {
				return atomic_load_explicit(&voltages[index], memory_order_relaxed);
			}
			return 0.0f;
		}

	public:
		/**
		 *  Device access
		 */
		uint8_t readBytePMIO(uint8_t addr);
		void writeBytePMIO(uint8_t addr, uint8_t value);

		/**
		 *  Overrides
		 */
		void setupKeys(VirtualSMCAPI::Plugin &vsmcPlugin) override;

		/**
		 *  Ctor
		 */
		ECDevice() = default;

		/**
		 *  Device factory
		 */
		static SuperIODevice* detect(SMCSuperIO* sio);
	};
}

#endif // _ITEDEVICE_HPP
