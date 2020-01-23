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
#include "Devices.hpp"

namespace ITE {

	uint16_t ITEDevice::tachometerRead8bit(uint8_t index) {
		if (index > 2) {
			// devices using this reading routine cannot have more than 3 fans
			return 0;
		}
		uint16_t value = readByte(ITE_FAN_TACHOMETER_REG[index]);
		// bits 0-2 - FAN1, bits 3-5 - FAN2, bit 6 - FAN3, bit 7 - reserved
		uint8_t divisorByte = readByte(ITE_FAN_TACHOMETER_DIVISOR_REGISTER) & 0x7F;
		uint8_t divisorShift = (divisorByte >> (3 * index)) & 0x7;
		if (index == 2) {
			// bit 6: set -> divisor = 8, unset -> divisor = 2
			divisorShift = divisorShift == 0 ? 1 : 3;
		}
		uint8_t divisor = 1 << divisorShift;
		return value > 0 && value < 0xff ? 1.35e6f / (value * divisor) : 0;
	}

	uint16_t ITEDevice::tachometerRead(uint8_t index) {
		uint16_t value = readByte(ITE_FAN_TACHOMETER_REG[index]);
		value |= readByte(ITE_FAN_TACHOMETER_EXT_REG[index]) << 8;
		return value > 0x3f && value < 0xffff ? (1.35e6f + value) / (value * 2) : 0;
	}
	
	float ITEDevice::voltageRead(uint8_t index) {
		uint8_t v = readByte(ITE_VOLTAGE_REG[index]);
		return static_cast<float>(v) * 0.012f;
	}

	float ITEDevice::voltageReadOld(uint8_t index) {
		uint8_t v = readByte(ITE_VOLTAGE_REG[index]);
		return static_cast<float>(v) * 0.016f;
	}

	uint8_t ITEDevice::readByte(uint8_t reg) {
		uint16_t address = getDeviceAddress();
		::outb(address + ITE_ADDRESS_REGISTER_OFFSET, reg);
		return ::inb(address + ITE_DATA_REGISTER_OFFSET);
	}
	
	void ITEDevice::writeByte(uint8_t reg, uint8_t value) {
		uint16_t address = getDeviceAddress();
		::outb(address + ITE_ADDRESS_REGISTER_OFFSET, reg);
		::outb(address + ITE_DATA_REGISTER_OFFSET, value);
	}
	
	void ITEDevice::setupKeys(VirtualSMCAPI::Plugin &vsmcPlugin) {
		VirtualSMCAPI::addKey(KeyFNum, vsmcPlugin.data,
			VirtualSMCAPI::valueWithUint8(getTachometerCount(), nullptr, SMC_KEY_ATTRIBUTE_CONST | SMC_KEY_ATTRIBUTE_READ));
		for (uint8_t index = 0; index < getTachometerCount(); ++index) {
			VirtualSMCAPI::addKey(KeyF0Ac(index), vsmcPlugin.data,
				VirtualSMCAPI::valueWithFp(0, SmcKeyTypeFpe2, new TachometerKey(getSmcSuperIO(), this, index)));
		}
	}

	/**
	 *  Device factory
	 */
	SuperIODevice* ITEDevice::detect(SMCSuperIO* sio) {
		// IT87XX can enter only on port 0x2E
		uint8_t port = SuperIOPort2E;
		enter(port);
		uint16_t id = listenPortWord(port, SuperIOChipIDRegister);
		DBGLOG("ssio", "probing device on 0x%4X, id=0x%4X", port, id);
		SuperIODevice *detectedDevice = createDevice(id);
		if (detectedDevice) {
			DBGLOG("ssio", "detected %s, starting address sanity checks", detectedDevice->getModelName());
			selectLogicalDevice(port, detectedDevice->getLdn());
			IOSleep(10);
			uint16_t address = listenPortWord(port, SuperIOBaseAddressRegister);
			IOSleep(10);
			uint16_t verifyAddress = listenPortWord(port, SuperIOBaseAddressRegister);
			IOSleep(10);
			
			if (address != verifyAddress || address < 0x100 || (address & 0xF007) != 0) {
				DBGLOG("ssio", "address verify check error: address = 0x%4X, verifyAddress = 0x%4X", address, verifyAddress);
				delete detectedDevice;
				return nullptr;
			} else {
				detectedDevice->initialize(address, port, sio);
			}
		}
		leave(port);
		return detectedDevice;
	}
	
} // namespace ITE
