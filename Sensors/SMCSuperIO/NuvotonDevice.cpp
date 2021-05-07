//
//  NuvotonDevice.cpp
//
//  Sensors implementation for Nuvoton SuperIO device
//
//  Based on https://github.com/kozlek/HWSensors/blob/master/SuperIOSensors/NCT677xSensors.cpp
//  @author joedm
//

#include "NuvotonDevice.hpp"
#include "SMCSuperIO.hpp"

namespace Nuvoton {
	
	uint8_t NuvotonDevice::readByte(uint16_t reg) {
		uint8_t bank = reg >> 8;
		uint8_t regi = reg & 0xFF;
		uint16_t address = getDeviceAddress();
		
		::outb(address + NUVOTON_ADDRESS_REGISTER_OFFSET, NUVOTON_BANK_SELECT_REGISTER);
		::outb(address + NUVOTON_DATA_REGISTER_OFFSET, bank);
		::outb(address + NUVOTON_ADDRESS_REGISTER_OFFSET, regi);
		
		return ::inb(address + NUVOTON_DATA_REGISTER_OFFSET);
	}

	uint8_t NuvotonDevice::readByte6683(uint16_t reg) {
		uint8_t bank = reg >> 8;
		uint8_t regi = reg & 0xFF;
		uint16_t address = getDeviceAddress();

		::outb(address + NUVOTON_6683_PAGE_REGISTER_OFFSET, 0xFF);
		::outb(address + NUVOTON_6683_PAGE_REGISTER_OFFSET, bank);
		::outb(address + NUVOTON_6683_INDEX_REGISTER_OFFSET, regi);

		return ::inb(address + NUVOTON_6683_DATA_REGISTER_OFFSET);
	}
	
	void NuvotonDevice::writeByte(uint16_t reg, uint8_t value) {
		uint8_t bank = reg >> 8;
		uint8_t regi = reg & 0xFF;
		uint16_t address = getDeviceAddress();
		
		::outb(address + NUVOTON_ADDRESS_REGISTER_OFFSET, NUVOTON_BANK_SELECT_REGISTER);
		::outb(address + NUVOTON_DATA_REGISTER_OFFSET, bank);
		::outb(address + NUVOTON_ADDRESS_REGISTER_OFFSET, regi);
		::outb(address + NUVOTON_DATA_REGISTER_OFFSET, value);
	}

	void NuvotonDevice::writeByte6683(uint16_t reg, uint8_t value) {
		uint8_t bank = reg >> 8;
		uint8_t regi = reg & 0xFF;
		uint16_t address = getDeviceAddress();

		::outb(address + NUVOTON_6683_PAGE_REGISTER_OFFSET, 0xFF);
		::outb(address + NUVOTON_6683_PAGE_REGISTER_OFFSET, bank);
		::outb(address + NUVOTON_6683_INDEX_REGISTER_OFFSET, regi);
		::outb(address + NUVOTON_6683_DATA_REGISTER_OFFSET, value);
	}
	
	void NuvotonDevice::setupKeys(VirtualSMCAPI::Plugin &vsmcPlugin) {
		VirtualSMCAPI::addKey(KeyFNum, vsmcPlugin.data,
			VirtualSMCAPI::valueWithUint8(getTachometerCount(), nullptr, SMC_KEY_ATTRIBUTE_CONST | SMC_KEY_ATTRIBUTE_READ));
		for (uint8_t index = 0; index < getTachometerCount(); ++index) {
			VirtualSMCAPI::addKey(KeyF0Ac(index), vsmcPlugin.data,
				VirtualSMCAPI::valueWithFp(0, SmcKeyTypeFpe2, new TachometerKey(getSmcSuperIO(), this, index)));
		}
	}
	
	uint16_t NuvotonDevice::tachometerRead(uint8_t index) {
		uint8_t high = readByte(NUVOTON_FAN_REGS[index]);
		uint8_t low = readByte(NUVOTON_FAN_REGS[index] + 1);
		return (high << 8) | low;
	}

	uint16_t NuvotonDevice::tachometerRead6776(uint8_t index) {
		uint8_t high = readByte(NUVOTON_FAN_6776_REGS[index]);
		uint8_t low = readByte(NUVOTON_FAN_6776_REGS[index] + 1);
		return (high << 8) | low;
	}

	uint16_t NuvotonDevice::tachometerRead6683(uint8_t index) {
		if (index >= NUVOTON_6683_FAN_NUMS) {
			return 0;
		}
		uint8_t high = readByte6683(NUVOTON_6683_FAN_REGS[index]);
		uint8_t low = readByte6683(NUVOTON_6683_FAN_REGS[index] + 1);
		return (high << 8) | low;
	}

	float NuvotonDevice::voltageRead(uint8_t index) {
		if (NUVOTON_VBAT_REG == NUVOTON_VOLTAGE_REGS[index]) {
			if (!(readByte(NUVOTON_VBAT_CONTROL_REG) & 1)) {
				return 0.0f;
			}
		}
		float value = readByte(NUVOTON_VOLTAGE_REGS[index]) * 0.008f;
		return value > 0 ? value : 0.0f;
	}

	float NuvotonDevice::voltageRead6775(uint8_t index) {
		if (NUVOTON_VBAT_6775_REG == NUVOTON_VOLTAGE_6775_REGS[index]) {
			if (!(readByte(NUVOTON_VBAT_CONTROL_REG) & 1)) {
				return 0.0f;
			}
		}
		float value = readByte(NUVOTON_VOLTAGE_6775_REGS[index]) * 0.008f;
		return value > 0 ? value : 0.0f;
	}

	float NuvotonDevice::voltageRead6683(uint8_t index) {
		if (nuvoton6683VoltageRegs[index] == 0) {
			return 0.0f;
		}
		float value = readByte6683(nuvoton6683VoltageRegs[index]) * 0.016f;
		return value > 0 ? value : 0.0f;
	}

	void NuvotonDevice::voltageMapping6683() {
		uint8_t value = 0;
		uint8_t index = 0;

		for (uint8_t i = 0; i < NUVOTON_6683_MON_NUMS; i++) {
			value = readByte6683(NUVOTON_6683_MON_CFG_OFFSET + i) & 0x7f;
			if (value >= NUVOTON_6683_MON_VOLTAGE_START) {
				index = value % NUVOTON_6683_MON_VOLTAGE_START;
				if (index >= NUVOTON_6683_VOLTAGE_NUMS) {
					continue;
				}
				nuvoton6683VoltageRegs[index] = NUVOTON_6683_MON_REGISTER_OFFSET + i * 2;
			}
		}
	}

	void NuvotonDevice::onPowerOn679xx() {
		i386_ioport_t port = getDevicePort();
		// disable the hardware monitor i/o space lock on NCT679xD chips
		enter(port);
		selectLogicalDevice(port, WinbondHardwareMonitorLDN);
		/* Activate logical device if needed */
		uint8_t options = listenPortByte(port, NUVOTON_REG_ENABLE);
		if (!(options & 0x01)) {
			writePortByte(port, NUVOTON_REG_ENABLE, options | 0x01);
		}
		options = listenPortByte(port, NUVOTON_HWMON_IO_SPACE_LOCK);
		// if the i/o space lock is enabled
		if (options & 0x10) {
			// disable the i/o space lock
			writePortByte(port, NUVOTON_HWMON_IO_SPACE_LOCK, options & ~0x10);
		}
		leave(port);
	}
} // namespace Nuvoton
