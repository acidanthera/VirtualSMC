//
//  SuperIODevice.hpp
//
//  SuperIO Chip data
//
//  Based on https://github.com/kozlek/HWSensors/blob/master/SuperIOSensors/SuperIODevice.h
//  @author joedm
//

#ifndef _SUPERIODEVICE_HPP
#define _SUPERIODEVICE_HPP

#include <IOKit/IOService.h>
#include <architecture/i386/pio.h>
#include "SMCSuperIO.hpp"

#define EC_ENDPOINT 0xFF

class SuperIODevice
{
private:
	i386_ioport_t devicePort {0};
	uint16_t deviceAddress {0};
	SMCSuperIO* smcSuperIO {nullptr};

protected:
	/**
	 *  Entering ports
	 */
	static constexpr uint8_t SuperIOPort2E = 0x2E;
	static constexpr uint8_t SuperIOPort4E = 0x4E;

	/**
	 *  Logical device number
	 */
	static constexpr uint8_t WinbondHardwareMonitorLDN = 0x0B;
	static constexpr uint8_t F71858HardwareMonitorLDN = 0x02;
	static constexpr uint8_t FintekITEHardwareMonitorLDN = 0x04;

	/**
	 *  Registers
	 */
	static constexpr uint8_t SuperIOConfigControlRegister  = 0x02;
	static constexpr uint8_t SuperIOChipIDRegister         = 0x20;
	static constexpr uint8_t SuperIOBaseAddressRegister    = 0x60;
	static constexpr uint8_t SuperIOBaseAltAddressRegister = 0x62;
	static constexpr uint8_t SuperIODeviceSelectRegister   = 0x07;
	static constexpr uint8_t SuperIODeviceActivateRegister = 0x30;

	/**
	 *  Key name index mapping
	 */
	static constexpr size_t MaxIndexCount = sizeof("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ") - 1;
	static constexpr const char *KeyIndexes = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	/**
	 *  Supported keys
	 */
	static constexpr SMC_KEY KeyFNum = SMC_MAKE_IDENTIFIER('F','N','u','m');
	static constexpr SMC_KEY KeyF0Ac(size_t i) { return SMC_MAKE_IDENTIFIER('F', KeyIndexes[i],'A', 'c'); }

	/**
	 *  Constructor
	 */
	SuperIODevice() = default;

	/**
	 *  Hardware access methods
	 */
	static inline uint8_t listenPortByte(i386_ioport_t port, uint8_t reg) {
		::outb(port, reg);
		return ::inb(port + 1);
	}

	static inline uint16_t listenPortWord(i386_ioport_t port, uint8_t reg) {
		return ((listenPortByte(port, reg) << 8) | listenPortByte(port, reg + 1));
	}

	static inline void writePortByte(i386_ioport_t port, uint8_t reg, uint8_t value) {
		::outb(port, reg);
		::outb(port + 1, value);
	}

	static inline void activateLogicalDevice(i386_ioport_t port) {
		uint8_t options = listenPortByte(port, SuperIODeviceActivateRegister);
		DBGLOG("ssio", "logical device activation status %d", options);
		if ((options & 1U) == 0) {
			writePortByte(port, SuperIODeviceActivateRegister, options | 1U);
			IOSleep(10);
			DBGLOG("ssio", "activated logical device, new status %d", listenPortByte(port, SuperIODeviceActivateRegister));
		}
	}

	static inline void selectLogicalDevice(i386_ioport_t port, uint8_t reg) {
		::outb(port, SuperIODeviceSelectRegister);
		::outb(port + 1, reg);
	}
	
protected:
	/**
	 * This method implementation is always-generated.
	 */
	virtual uint8_t getTachometerCount() = 0;

	virtual const char* getTachometerName(uint8_t index) = 0;

	virtual void setTachometerValue(uint8_t index, uint16_t value) = 0;

	virtual void updateTachometers() {
		for (uint8_t index = 0; index < getTachometerCount(); ++index) {
			uint16_t value = updateTachometer(index);
			setTachometerValue(index, value);
		}
	}
	/**
	 * This method implementation is always-generated.
	 */
	virtual uint16_t updateTachometer(uint8_t index) = 0;
	/**
	 * This method implementation is always-generated.
	 */
	virtual uint8_t getVoltageCount() = 0;

	virtual const char* getVoltageName(uint8_t index) = 0;

	virtual void setVoltageValue(uint8_t index, float value) = 0;

	virtual void updateVoltages() {
		for (uint8_t index = 0; index < getVoltageCount(); ++index) {
			float value = updateVoltage(index);
			setVoltageValue(index, value);
		}
	}
	/**
	 * This method implementation is always-generated.
	 */
	virtual float updateVoltage(uint8_t index) = 0;

	/**
	 * Do something on power state change to PowerStateOn.
	 */
	virtual void onPowerOn() {}
	
public:
	/**
	 *  Initialize created device instance.
	 */
	virtual void initialize(uint16_t address, i386_ioport_t port, SMCSuperIO* sio) {
		deviceAddress = address;
		devicePort = port;
		smcSuperIO = sio;
	}
	
	/**
	 *  Power state handling.
	 *  @param state is a SMCSuperIO::PowerState value.
	 */
	void powerStateChanged(unsigned long state) {
		if (state == SMCSuperIO::PowerStateOn) {
			onPowerOn();
		}
	}

	/**
	 *  Set up SMC keys.
	 */
	virtual void setupKeys(VirtualSMCAPI::Plugin &vsmcPlugin) = 0;

	/**
	 *  Invoked by timer event. Sync write ops with key accessors if necessary.
	 */
	void update();
	
	void updateIORegistry();
	
	/**
	 *  Accessors
	 */
	virtual uint16_t getTachometerValue(uint8_t index) = 0;
	virtual float getVoltageValue(uint8_t index) = 0;
	virtual const char* getModelName() = 0;
	virtual uint8_t getLdn() { return EC_ENDPOINT; };

	/**
	 *  Getters
	 */
	uint16_t getDeviceAddress() { return deviceAddress; }
	i386_ioport_t getDevicePort() { return devicePort; }
	const SMCSuperIO* getSmcSuperIO() { return smcSuperIO; }

	/**
	 *  Destructor
	 */
	virtual ~SuperIODevice() = default;
};

/**
 * Generic keys
 */
class TachometerKey : public VirtualSMCValue {
protected:
	const SMCSuperIO *sio;
	uint8_t index;
	SuperIODevice *device;
	SMC_RESULT readAccess() override;
public:
	TachometerKey(const SMCSuperIO *sio, SuperIODevice *device, uint8_t index) : sio(sio), index(index), device(device) {}
};

#endif // _SUPERIODEVICE_HPP
