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
protected:

	// Reasonable max values.
	static constexpr uint8_t MAX_TACHOMETER_COUNT = 10;
	static constexpr uint8_t MAX_VOLTAGE_COUNT = 10;
	static constexpr uint8_t MAX_TEMPERATURE_COUNT = 10;

	i386_ioport_t devicePort {0};
	uint16_t deviceAddress {0};
	SMCSuperIO* smcSuperIO {nullptr};

	/**
	 *  Tachometers
	 */
	_Atomic(uint16_t) tachometers[MAX_TACHOMETER_COUNT] = { };
	/**
	 *	Target RPMs
	 */
	_Atomic(uint16_t) targets[MAX_TACHOMETER_COUNT] = { };
	/**
	 *	Max RPMs
	 */
	_Atomic(uint16_t) maxRpm[MAX_TACHOMETER_COUNT] = { };
	/**
	 *	Min RPMs
	 */
	_Atomic(uint16_t) minRpm[MAX_TACHOMETER_COUNT] = { };
	/**
	 * Manual contol
	 */
	_Atomic(uint8_t) manual[MAX_TACHOMETER_COUNT] = { };
	/**
	 *  Voltages
	 */
	_Atomic(float) voltages[MAX_VOLTAGE_COUNT] = { };

	/**
	 *  Temperatures
	 */
	_Atomic(float) temperatures[MAX_TEMPERATURE_COUNT] = { };

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
	static constexpr SMC_KEY KeyF0ID(size_t i) { return SMC_MAKE_IDENTIFIER('F', KeyIndexes[i],'I','D'); }
	static constexpr SMC_KEY KeyF0Ac(size_t i) { return SMC_MAKE_IDENTIFIER('F', KeyIndexes[i],'A', 'c'); }
	static constexpr SMC_KEY KeyF0Mn(size_t i) { return SMC_MAKE_IDENTIFIER('F', KeyIndexes[i],'M','n'); }
	static constexpr SMC_KEY KeyF0Mx(size_t i) { return SMC_MAKE_IDENTIFIER('F', KeyIndexes[i],'M','x'); }
	static constexpr SMC_KEY KeyF0Md(size_t i) { return SMC_MAKE_IDENTIFIER('F', KeyIndexes[i],'M','d'); }
	static constexpr SMC_KEY KeyF0Tg(size_t i) { return SMC_MAKE_IDENTIFIER('F', KeyIndexes[i],'T','g'); }
	static constexpr SMC_KEY KeyVM0R(size_t i) { return SMC_MAKE_IDENTIFIER('V','M',KeyIndexes[i],'R'); }
	static constexpr SMC_KEY KeyVD0R(size_t i) { return SMC_MAKE_IDENTIFIER('V','D',KeyIndexes[i],'R'); }
	static constexpr SMC_KEY KeyV50R(size_t i) { return SMC_MAKE_IDENTIFIER('V','5',KeyIndexes[i],'R'); }
	static constexpr SMC_KEY KeyVR3R = SMC_MAKE_IDENTIFIER('V','5','3','R');
	static constexpr SMC_KEY KeyTC0P(size_t i) { return SMC_MAKE_IDENTIFIER('T','C',KeyIndexes[i],'P'); }
	static constexpr SMC_KEY KeyTP0P(size_t i) { return SMC_MAKE_IDENTIFIER('T','P',KeyIndexes[i],'P'); }
	static constexpr SMC_KEY KeyTM0P(size_t i) { return SMC_MAKE_IDENTIFIER('T','M',KeyIndexes[i],'P'); }
	static constexpr SMC_KEY KeyTG0P(size_t i) { return SMC_MAKE_IDENTIFIER('T','G',KeyIndexes[i],'P'); }
	static constexpr SMC_KEY KeyTH0P(size_t i) { return SMC_MAKE_IDENTIFIER('T','H',KeyIndexes[i],'P'); }
	static constexpr SMC_KEY KeyTA0P(size_t i) { return SMC_MAKE_IDENTIFIER('T','A',KeyIndexes[i],'P'); }
	static constexpr SMC_KEY KeyTV0P(size_t i) { return SMC_MAKE_IDENTIFIER('T','V',KeyIndexes[i],'P'); }
	static constexpr SMC_KEY KeyInvalid = SMC_MAKE_IDENTIFIER('X','X','X','X');

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
	virtual uint16_t updateTachometer(uint8_t index) = 0;

	virtual void setTachometerValue(uint8_t index, uint16_t value) {
		if (index < getTachometerCount() && index < MAX_TACHOMETER_COUNT) {
			atomic_store_explicit(&tachometers[index], value, memory_order_relaxed);
		}
	}

	virtual void updateTachometers() {
		for (uint8_t index = 0; index < getTachometerCount(); ++index) {
			uint16_t value = updateTachometer(index);
			setTachometerValue(index, value);
		}
	}

	/**
	 * This method implementation is always-generated.
	 */
	virtual uint8_t getVoltageCount() = 0;
	virtual const char* getVoltageName(uint8_t index) = 0;
	virtual float updateVoltage(uint8_t index) = 0;
	
	virtual void setVoltageValue(uint8_t index, float value) {
		if (index < getVoltageCount() && index < MAX_VOLTAGE_COUNT) {
			atomic_store_explicit(&voltages[index], value, memory_order_relaxed);
		}
	}
	virtual void updateVoltages() {
		for (uint8_t index = 0; index < getVoltageCount(); ++index) {
			float value = updateVoltage(index);
			setVoltageValue(index, value);
		}
	}

	/**
	 * This method is optional.
	 */
	virtual uint8_t getTemperatureCount() { return 0; }
	virtual const char* getTemperatureName(uint8_t index) { return ""; }
	virtual float updateTemperature(uint8_t index) { return 0.0; }

	virtual void setTemperatureValue(uint8_t index, float value) {
		if (index < getTemperatureCount() && index < MAX_TEMPERATURE_COUNT) {
			atomic_store_explicit(&temperatures[index], value, memory_order_relaxed);
		}
	}
	virtual void updateTemperatures() {
		for (uint8_t index = 0; index < getTemperatureCount(); ++index) {
			float value = updateTemperature(index);
			setTemperatureValue(index, value);
		}
	}

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
	
	virtual void updateTargets() {};

	/**
	 *  Accessors
	 */
	uint16_t getTachometerValue(uint8_t index) {
		if (index < getTachometerCount() && index < MAX_TACHOMETER_COUNT) {
			return atomic_load_explicit(&tachometers[index], memory_order_relaxed);
		}
		return 0;
	}

	uint16_t getTargetValue(uint8_t index) {
		if (index < getTachometerCount() && index < MAX_TACHOMETER_COUNT) {
			return atomic_load_explicit(&targets[index], memory_order_relaxed);
		}
		return 0;
	}

	uint8_t getManualValue(uint8_t index) {
		if (index < getTachometerCount() && index < MAX_TACHOMETER_COUNT) {
			return atomic_load_explicit(&manual[index], memory_order_relaxed);
		}
		return 0;
	}

	float getVoltageValue(uint8_t index) {
		if (index < getVoltageCount() && index < MAX_VOLTAGE_COUNT) {
			return atomic_load_explicit(&voltages[index], memory_order_relaxed);
		}
		return 0.0f;
	}

	float getTemperatureValue(uint8_t index) {
		if (index < getTemperatureCount() && index < MAX_TEMPERATURE_COUNT) {
			return atomic_load_explicit(&temperatures[index], memory_order_relaxed);
		}
		return 0.0f;
	}
	
	uint16_t getMaxValue(uint8_t index) {
		if (index < getTachometerCount() && index < MAX_TACHOMETER_COUNT) {
			return atomic_load_explicit(&maxRpm[index], memory_order_relaxed);
		}
		return 0;
	}
	
	uint16_t getMinValue(uint8_t index) {
		if (index < getTachometerCount() && index < MAX_TACHOMETER_COUNT) {
			return atomic_load_explicit(&minRpm[index], memory_order_relaxed);
		}
		return 0;
	}

	virtual const char* getModelName() = 0;
	virtual uint8_t getLdn() { return EC_ENDPOINT; };

	/**
	 *  Getters
	 */
	uint16_t getDeviceAddress() { return deviceAddress; }
	i386_ioport_t getDevicePort() { return devicePort; }
	const SMCSuperIO* getSmcSuperIO() { return smcSuperIO; }

	virtual void setTargetValue(uint8_t index, uint16_t value) {
		if (index < getTachometerCount() && index < MAX_TACHOMETER_COUNT) {
			atomic_store_explicit(&targets[index], value, memory_order_relaxed);
		}
	}

	virtual void setManualValue(uint8_t index, uint8_t value) {
		if (index < getTachometerCount() && index < MAX_TACHOMETER_COUNT) {
			atomic_store_explicit(&manual[index], value, memory_order_relaxed);
		}
	}
	
	virtual void setMaxValue(uint8_t index, uint16_t value) {
		if (index < getTachometerCount() && index < MAX_TACHOMETER_COUNT) {
			atomic_store_explicit(&maxRpm[index], value, memory_order_relaxed);
		}
	}

	virtual void setMinValue(uint8_t index, uint16_t value) {
		if (index < getTachometerCount() && index < MAX_TACHOMETER_COUNT) {
			atomic_store_explicit(&minRpm[index], value, memory_order_relaxed);
		}
	}
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

class MinKey : public VirtualSMCValue {
protected:
	const SMCSuperIO *sio;
	uint8_t index;
	SuperIODevice *device;
	SMC_RESULT readAccess() override;
public:
	MinKey(const SMCSuperIO *sio, SuperIODevice *device, uint8_t index) : sio(sio), index(index), device(device) {}
};

class MaxKey : public VirtualSMCValue {
protected:
	const SMCSuperIO *sio;
	uint8_t index;
	SuperIODevice *device;
	SMC_RESULT readAccess() override;
public:
	MaxKey(const SMCSuperIO *sio, SuperIODevice *device, uint8_t index) : sio(sio), index(index), device(device) {}
};

class ManualKey : public VirtualSMCValue {
protected:
	const SMCSuperIO *sio;
	uint8_t index;
	SuperIODevice *device;
	SMC_RESULT readAccess() override;
	SMC_RESULT update(const SMC_DATA *src) override;
public:
	ManualKey(const SMCSuperIO *sio, SuperIODevice *device, uint8_t index) : sio(sio), index(index), device(device) {}
};

class TargetKey : public VirtualSMCValue {
protected:
	const SMCSuperIO *sio;
	uint8_t index;
	SuperIODevice *device;
	SMC_RESULT readAccess() override;
	SMC_RESULT update(const SMC_DATA *src) override;
public:
	TargetKey(const SMCSuperIO *sio, SuperIODevice *device, uint8_t index) : sio(sio), index(index), device(device) {}
};

class VoltageKey : public VirtualSMCValue {
protected:
	const SMCSuperIO *sio;
	uint8_t index;
	SuperIODevice *device;
	SMC_RESULT readAccess() override;
public:
	VoltageKey(const SMCSuperIO *sio, SuperIODevice *device, uint8_t index) : sio(sio), index(index), device(device) {}
};

class TemperatureKey : public VirtualSMCValue {
protected:
	const SMCSuperIO *sio;
	uint8_t index;
	SuperIODevice *device;
	SMC_RESULT readAccess() override;
public:
	TemperatureKey(const SMCSuperIO *sio, SuperIODevice *device, uint8_t index) : sio(sio), index(index), device(device) {}
};

#endif // _SUPERIODEVICE_HPP
