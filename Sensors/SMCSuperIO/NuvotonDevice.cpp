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

		// The EC page/index window is shared with the timer thread, so every
		// access must be serialised. Recursive: the fan-config handshake holds
		// it implicitly through these calls without deadlocking.
		if (ecLock) IORecursiveLockLock(ecLock);

		::outb(address + NUVOTON_6683_PAGE_REGISTER_OFFSET, 0xFF);
		::outb(address + NUVOTON_6683_PAGE_REGISTER_OFFSET, bank);
		::outb(address + NUVOTON_6683_INDEX_REGISTER_OFFSET, regi);

		uint8_t value = ::inb(address + NUVOTON_6683_DATA_REGISTER_OFFSET);

		if (ecLock) IORecursiveLockUnlock(ecLock);
		return value;
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

		if (ecLock) IORecursiveLockLock(ecLock);

		::outb(address + NUVOTON_6683_PAGE_REGISTER_OFFSET, 0xFF);
		::outb(address + NUVOTON_6683_PAGE_REGISTER_OFFSET, bank);
		::outb(address + NUVOTON_6683_INDEX_REGISTER_OFFSET, regi);
		::outb(address + NUVOTON_6683_DATA_REGISTER_OFFSET, value);

		if (ecLock) IORecursiveLockUnlock(ecLock);
	}
	
	void NuvotonDevice::setupKeys(VirtualSMCAPI::Plugin &vsmcPlugin) {
		VirtualSMCAPI::addKey(KeyFNum, vsmcPlugin.data,
			VirtualSMCAPI::valueWithUint8(getTachometerCount(), nullptr, SMC_KEY_ATTRIBUTE_CONST | SMC_KEY_ATTRIBUTE_READ));
		for (uint8_t index = 0; index < getTachometerCount(); ++index) {
			VirtualSMCAPI::addKey(KeyF0Ac(index), vsmcPlugin.data,
				VirtualSMCAPI::valueWithFp(0, SmcKeyTypeFpe2, new TachometerKey(getSmcSuperIO(), this, index)));
			// Nuvoton chips do not expose firmware minimum/maximum RPM registers,
			// so the F0Mn/F0Mx range is derived from the observed readings in
			// updateTachometers(). Without these keys monitoring software falls
			// back to its own placeholder limits (commonly shown as 65536).
			VirtualSMCAPI::addKey(KeyF0Mn(index), vsmcPlugin.data,
				VirtualSMCAPI::valueWithFp(0, SmcKeyTypeFpe2, new MinKey(getSmcSuperIO(), this, index)));
			VirtualSMCAPI::addKey(KeyF0Mx(index), vsmcPlugin.data,
				VirtualSMCAPI::valueWithFp(0, SmcKeyTypeFpe2, new MaxKey(getSmcSuperIO(), this, index)));

			// Manual fan control (NCT6687D-R only): F0Md enables manual mode,
			// F0Tg carries the requested RPM. Both are handled by updateTargets().
			if (fanControlSupported() && index < NUVOTON_6687_FAN_NUMS &&
			    NUVOTON_6687_FAN_CTRL[index].dutyWrite != 0) {
				VirtualSMCAPI::addKey(KeyF0Md(index), vsmcPlugin.data,
					VirtualSMCAPI::valueWithUint8(0, new ManualKey(getSmcSuperIO(), this, index),
						SMC_KEY_ATTRIBUTE_WRITE | SMC_KEY_ATTRIBUTE_READ));
				VirtualSMCAPI::addKey(KeyF0Tg(index), vsmcPlugin.data,
					VirtualSMCAPI::valueWithFp(0, SmcKeyTypeFpe2, new TargetKey(getSmcSuperIO(), this, index),
						SMC_KEY_ATTRIBUTE_WRITE | SMC_KEY_ATTRIBUTE_READ));
			}
		}

		setupTemperatureKeys(vsmcPlugin);

		qsort(const_cast<VirtualSMCKeyValue *>(vsmcPlugin.data.data()), vsmcPlugin.data.size(), sizeof(VirtualSMCKeyValue), VirtualSMCKeyValue::compare);
	}

	void NuvotonDevice::initialize(uint16_t address, i386_ioport_t port, SMCSuperIO* sio) {
		SuperIODevice::initialize(address, port, sio);
		if (!ecLock) {
			ecLock = IORecursiveLockAlloc();
		}
		// Enter manual fan control only on the chips that expose it, and only
		// once the EC customer id matches the D-R family value.
		if (fanControlSupported()) {
			uint16_t customerId = (readByte6683(NUVOTON_6687_REG_CUSTOMER_ID) << 8) |
				readByte6683(NUVOTON_6687_REG_CUSTOMER_ID + 1);
			if (customerId != NUVOTON_6687_CUSTOMER_ID_MSI4) {
				SYSLOG("ssio", "NCT6687D-R fan control disabled: customer id 0x%04X != 0x%04X",
					customerId, NUVOTON_6687_CUSTOMER_ID_MSI4);
				fanControlEnabled = false;
			} else {
				SYSLOG("ssio", "NCT6687D-R fan control enabled (customer id 0x%04X)", customerId);
				fanControlEnabled = true;
			}
		}
	}

	bool NuvotonDevice::setFanDuty(uint8_t index, uint8_t value) {
		if (!fanControlEnabled || index >= getTachometerCount() || index >= NUVOTON_6687_FAN_NUMS)
			return false;

		const Nuvoton6687Fan &fan = NUVOTON_6687_FAN_CTRL[index];
		if (fan.dutyWrite == 0)
			return false;

		// Hold the EC lock for the whole handshake: it is a multi-register
		// sequence that must not be interleaved with the timer's reads.
		if (ecLock) IORecursiveLockLock(ecLock);

		// Save the original EC state before the very first override of a header.
		if (!hasSaved[index]) {
			savedModeBit[index] = static_cast<uint8_t>(readByte6683(fan.modeReg) & (1U << fan.modeBit));
			savedDuty[index] = readByte6683(fan.dutyWrite);
			hasSaved[index] = true;
		}

		bool ok = false;
		for (int attempt = 0; attempt < 3 && !ok; ++attempt) {
			if (!startFanConfig())
				break;
			// Read-modify-write: never clear someone else's manual-mode bits.
			uint8_t mode = readByte6683(fan.modeReg);
			writeByte6683(fan.modeReg, static_cast<uint8_t>(mode | (1U << fan.modeBit)));
			writeByte6683(fan.dutyWrite, value);
			ok = finishFanConfig();
		}

		if (ecLock) IORecursiveLockUnlock(ecLock);
		return ok;
	}

	bool NuvotonDevice::restoreFanControl(uint8_t index) {
		if (!fanControlEnabled || index >= getTachometerCount() || index >= NUVOTON_6687_FAN_NUMS)
			return false;
		if (!hasSaved[index])
			return true;

		const Nuvoton6687Fan &fan = NUVOTON_6687_FAN_CTRL[index];

		if (ecLock) IORecursiveLockLock(ecLock);

		bool ok = false;
		for (int attempt = 0; attempt < 3 && !ok; ++attempt) {
			if (!startFanConfig())
				break;
			uint8_t mode = readByte6683(fan.modeReg);
			uint8_t mask = static_cast<uint8_t>(1U << fan.modeBit);
			mode = static_cast<uint8_t>((mode & ~mask) | (savedModeBit[index] & mask));
			writeByte6683(fan.modeReg, mode);
			writeByte6683(fan.dutyWrite, savedDuty[index]);
			ok = finishFanConfig();
		}

		if (ok)
			hasSaved[index] = false;

		if (ecLock) IORecursiveLockUnlock(ecLock);
		return ok;
	}

	bool NuvotonDevice::startFanConfig() {
		uint8_t sts = readByte6683(NUVOTON_6687_REG_ENGINE_STS);
		if ((sts & NUVOTON_6687_CFG_LOCK) == 0 && (sts & NUVOTON_6687_CFG_PHASE) != 0)
			return true;

		// Wait (<=1s) until the EC finishes any pending configuration phase.
		for (int i = 0; i < 1000; ++i) {
			sts = readByte6683(NUVOTON_6687_REG_ENGINE_STS);
			if ((sts & NUVOTON_6687_CFG_PHASE) == 0 &&
			    (readByte6683(NUVOTON_6687_REG_CFG_CTRL) & NUVOTON_6687_CFG_REQ) == 0)
				break;
			IOSleep(1);
		}

		uint8_t ctrl = readByte6683(NUVOTON_6687_REG_CFG_CTRL);
		writeByte6683(NUVOTON_6687_REG_CFG_CTRL, static_cast<uint8_t>(ctrl | NUVOTON_6687_CFG_REQ));
		IOSleep(10);

		for (int i = 0; i < 1000; ++i) {
			sts = readByte6683(NUVOTON_6687_REG_ENGINE_STS);
			if ((sts & NUVOTON_6687_CFG_LOCK) == 0 && (sts & NUVOTON_6687_CFG_PHASE) != 0)
				return true;
			IOSleep(1);
		}

		SYSLOG("ssio", "NCT6687D-R: failed to enter fan config phase");
		return false;
	}

	bool NuvotonDevice::finishFanConfig() {
		uint8_t sts = readByte6683(NUVOTON_6687_REG_ENGINE_STS);
		if ((sts & NUVOTON_6687_CFG_LOCK) != 0 || (sts & NUVOTON_6687_CFG_PHASE) == 0)
			return false;

		uint8_t ctrl = readByte6683(NUVOTON_6687_REG_CFG_CTRL);
		writeByte6683(NUVOTON_6687_REG_CFG_CTRL, static_cast<uint8_t>(ctrl | NUVOTON_6687_CFG_DONE));
		IOSleep(10);

		for (int i = 0; i < 1000; ++i) {
			if (readByte6683(NUVOTON_6687_REG_ENGINE_STS) & NUVOTON_6687_CFG_CHECK_DONE)
				break;
			IOSleep(1);
		}

		sts = readByte6683(NUVOTON_6687_REG_ENGINE_STS);
		if (sts & NUVOTON_6687_CFG_INVALID) {
			SYSLOG("ssio", "NCT6687D-R: EC rejected the fan configuration");
			return false;
		}
		return true;
	}

	void NuvotonDevice::updateTargets() {
		if (!fanControlEnabled)
			return;

		for (uint8_t index = 0; index < getTachometerCount() && index < NUVOTON_6687_FAN_NUMS; ++index) {
			if (!NUVOTON_6687_FAN_CTRL[index].dutyWrite)
				continue;

			uint8_t manual = getManualValue(index);
			if (!manual) {
				// Manual control released: hand the header back to the EC.
				if (hasSaved[index])
					restoreFanControl(index);
				continue;
			}

			// F0Tg carries the requested RPM. The EC fan curve is not readable
			// here, so map the target linearly onto the observed min..max range
			// the same way the ITE path derives its curve. Until the range has
			// been learned from live readings (or without a positive target),
			// leave the header to the EC rather than risk stopping the fan.
			uint16_t target = getTargetValue(index);
			uint16_t lo = getMinValue(index);
			uint16_t hi = getMaxValue(index);
			if (hi <= lo || target == 0) {
				DBGLOG("ssio", "NCT6687D-R: fan %u has no usable target/range (%u..%u, target %u), leaving to EC",
					index, lo, hi, target);
				continue;
			}

			uint8_t duty;
			if (target <= lo) {
				duty = 0;
			} else if (target >= hi) {
				duty = 255;
			} else {
				duty = static_cast<uint8_t>(static_cast<uint32_t>(target - lo) * 255U / (hi - lo));
			}

			if (!setFanDuty(index, duty))
				SYSLOG("ssio", "NCT6687D-R: failed to set fan %u duty %u", index, duty);
		}
	}

	void NuvotonDevice::updateTachometers() {
		SuperIODevice::updateTachometers();

		// Track the running minimum and maximum RPM advertised through F0Mn/F0Mx.
		// A stopped or disconnected fan reads 0, and Fpe2 can only represent
		// unsigned 14.2 fixed point values up to 16383, so discard any reading
		// that is not a plausible, representable fan speed.
		for (uint8_t index = 0; index < getTachometerCount(); ++index) {
			uint16_t value = getTachometerValue(index);
			if (value == 0 || value >= (1U << 14)) {
				continue;
			}
			uint16_t minValue = getMinValue(index);
			if (minValue == 0 || value < minValue) {
				setMinValue(index, value);
			}
			if (value > getMaxValue(index)) {
				setMaxValue(index, value);
			}
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

	uint16_t NuvotonDevice::tachometerRead6687(uint8_t index) {
		if (index >= NUVOTON_6687_FAN_NUMS) {
			return 0;
		}
		uint8_t high = readByte6683(NUVOTON_6687_FAN_REGS[index]);
		uint8_t low = readByte6683(NUVOTON_6687_FAN_REGS[index] + 1);
		return (high << 8) | low;
	}

	float NuvotonDevice::temperatureRead6687(uint8_t index) {
		if (index >= NUVOTON_6687_TEMP_NUMS) {
			return 0.0f;
		}
		uint16_t reg = NUVOTON_6687_TEMP_BASE + index * 2;
		// Signed integer degrees in the low register, half-degree bit 7 of the next one.
		int8_t value = static_cast<int8_t>(readByte6683(reg));
		uint8_t half = (readByte6683(reg + 1) >> 7) & 0x1;
		return static_cast<float>(value) + (half ? 0.5f : 0.0f);
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
