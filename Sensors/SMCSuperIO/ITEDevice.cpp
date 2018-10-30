//
//  ITEDevice.cpp
//
//  Sensors implementation for ITE SuperIO device
//
//  Based on https://github.com/kozlek/HWSensors/blob/master/SuperIOSensors/IT87xxSensors.cpp
//  @author joedm.
//

#include "ITEDevice.hpp"
#include "SMCSuperIO.hpp"

namespace ITE {

	uint16_t Device::tachometerRead(uint8_t index) {
		uint16_t value = readByte(ITE_FAN_TACHOMETER_REG[index]);
		int divisor = 2;
		if (index < 2) {
			divisor = 1 << ((readByte(ITE_FAN_TACHOMETER_DIVISOR_REGISTER) >> (3 * index)) & 0x7);
		}
		return value > 0 && value < 0xff ? 1.35e6f / (value * divisor) : 0;
	}

	uint16_t Device::tachometerRead16(uint8_t index) {
		uint16_t value = readByte(ITE_FAN_TACHOMETER_REG[index]);
		value |= readByte(ITE_FAN_TACHOMETER_EXT_REG[index]) << 8;
		return value > 0x3f && value < 0xffff ? (1.35e6f + value) / (value * 2) : 0;
	}

	uint8_t Device::readByte(uint8_t reg) {
		uint16_t address = getDeviceAddress();
		::outb(address + ITE_ADDRESS_REGISTER_OFFSET, reg);
		return ::inb(address + ITE_DATA_REGISTER_OFFSET);
	}
	
	void Device::writeByte(uint8_t reg, uint8_t value) {
		uint16_t address = getDeviceAddress();
		::outb(address + ITE_ADDRESS_REGISTER_OFFSET, reg);
		::outb(address + ITE_DATA_REGISTER_OFFSET, value);
	}
	
	void Device::setupKeys(VirtualSMCAPI::Plugin &vsmcPlugin) {
		VirtualSMCAPI::addKey(KeyFNum, vsmcPlugin.data,
			VirtualSMCAPI::valueWithUint8(deviceDescriptor.tachometerCount, nullptr, SMC_KEY_ATTRIBUTE_CONST | SMC_KEY_ATTRIBUTE_READ));
		for (uint8_t index = 0; index < deviceDescriptor.tachometerCount; ++index) {
			VirtualSMCAPI::addKey(KeyF0Ac(index), vsmcPlugin.data,
				VirtualSMCAPI::valueWithFp(0, SmcKeyTypeFpe2, new TachometerKey(getSmcSuperIO(), this, index)));
		}
	}

	void Device::update() {
		IOSimpleLockLock(getSmcSuperIO()->counterLock);
		updateTachometers();
		IOSimpleLockUnlock(getSmcSuperIO()->counterLock);
	}
	
	void Device::updateTachometers() {
		for (uint8_t index = 0; index < deviceDescriptor.tachometerCount; ++index) {
			tachometers[index] = CALL_MEMBER_FUNC(*this, deviceDescriptor.updateTachometer)(index);
		}
	}
	
	/**
	 *  Supported devices
	 */
	const Device::DeviceDescriptor Device::_IT8512F = { IT8512F, 5, &Device::tachometerRead };
	const Device::DeviceDescriptor Device::_IT8705F = { IT8705F, 3, &Device::tachometerRead };
	const Device::DeviceDescriptor Device::_IT8712F = { IT8712F, 5, &Device::tachometerRead };
	const Device::DeviceDescriptor Device::_IT8716F = { IT8716F, 5, &Device::tachometerRead16 };
	const Device::DeviceDescriptor Device::_IT8718F = { IT8718F, 5, &Device::tachometerRead16 };
	const Device::DeviceDescriptor Device::_IT8720F = { IT8720F, 5, &Device::tachometerRead16 };
	const Device::DeviceDescriptor Device::_IT8721F = { IT8721F, 5, &Device::tachometerRead16 };
	const Device::DeviceDescriptor Device::_IT8726F = { IT8726F, 5, &Device::tachometerRead16 };
	const Device::DeviceDescriptor Device::_IT8620E = { IT8620E, 5, &Device::tachometerRead16 };
	const Device::DeviceDescriptor Device::_IT8628E = { IT8628E, 5, &Device::tachometerRead16 };
	const Device::DeviceDescriptor Device::_IT8686E = { IT8686E, 5, &Device::tachometerRead16 };
	const Device::DeviceDescriptor Device::_IT8728F = { IT8728F, 5, &Device::tachometerRead16 };
	const Device::DeviceDescriptor Device::_IT8752F = { IT8752F, 5, &Device::tachometerRead16 };
	const Device::DeviceDescriptor Device::_IT8771E = { IT8771E, 5, &Device::tachometerRead16 };
	const Device::DeviceDescriptor Device::_IT8772E = { IT8772E, 5, &Device::tachometerRead16 };
	const Device::DeviceDescriptor Device::_IT8792E = { IT8792E, 5, &Device::tachometerRead16 };
	
	/**
	 *  Device factory
	 */
	SuperIODevice* Device::detect(SMCSuperIO* sio) {
		// IT87XX can enter only on port 0x2E
		uint8_t port = SuperIOPort2E;
		enter(port);
		uint16_t id = listenPortWord(port, SuperIOChipIDRegister);
		DBGLOG("ssio", "probing device on 0x%4X, id=0x%4X", port, id);
		uint8_t ldn = FintekITEHardwareMonitorLDN;
		SuperIODevice *detectedDevice = nullptr;
		const DeviceDescriptor *desc = nullptr;

		switch (id) {
			case IT8512F:
				desc = &_IT8512F;
				break;
			case IT8705F:
				desc = &_IT8705F;
				break;
			case IT8712F:
				desc = &_IT8712F;
				break;
			case IT8716F:
				desc = &_IT8716F;
				break;
			case IT8718F:
				desc = &_IT8718F;
				break;
			case IT8720F:
				desc = &_IT8720F;
				break;
			case IT8721F:
				desc = &_IT8721F;
				break;
			case IT8726F:
				desc = &_IT8726F;
				break;
			case IT8620E:
				desc = &_IT8620E;
				break;
			case IT8628E:
				desc = &_IT8628E;
				break;
			case IT8686E:
				desc = &_IT8686E;
				break;
			case IT8728F:
				desc = &_IT8728F;
				break;
			case IT8752F:
				desc = &_IT8752F;
				break;
			case IT8771E:
				desc = &_IT8771E;
				break;
			case IT8772E:
				desc = &_IT8772E;
				break;
			case IT8792E:
				desc = &_IT8792E;
				break;
		}
		if (desc) {
			DBGLOG("ssio", "detected %s, starting address sanity checks", SuperIODevice::getModelName(desc->ID));
			selectLogicalDevice(port, ldn);
			IOSleep(10);
			uint16_t address = listenPortWord(port, SuperIOBaseAddressRegister);
			IOSleep(10);
			uint16_t verifyAddress = listenPortWord(port, SuperIOBaseAddressRegister);
			IOSleep(10);
			
			if (address != verifyAddress || address < 0x100 || (address & 0xF007) != 0) {
				DBGLOG("ssio", "address verify check error: address = 0x%4X, verifyAddress = 0x%4X", address, verifyAddress);
			} else {
				detectedDevice = new Device(*desc, address, port, sio);
			}
		}
		leave(port);
		return detectedDevice;
	}
	
} // namespace ITE
