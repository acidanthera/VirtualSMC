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

enum SuperIOModel
{
	sioUnknown = -1,

	// ITE
	IT8512F     = 0x8512,
	IT8705F     = 0x8705,
	IT8712F     = 0x8712,
	IT8716F     = 0x8716,
	IT8718F     = 0x8718,
	IT8720F     = 0x8720,
	IT8721F     = 0x8721,
	IT8726F     = 0x8726,
	IT8620E     = 0x8620,
	IT8628E     = 0x8628,
	IT8686E     = 0x8686,
	IT8728F     = 0x8728,
	IT8752F     = 0x8752,
	IT8771E     = 0x8771,
	IT8772E     = 0x8772,
	IT8792E     = 0x8792,
	IT8688E     = 0x8688,
	IT8795E     = 0x8795,
	IT8665E     = 0x8665,
	IT8613E     = 0x8613,

	// Winbond
	W83627DHG   = 0xA020,
	W83627UHG   = 0xA230,
	W83627DHGP  = 0xB070,
	W83627EHF   = 0x8800,
	W83627HF    = 0x5200,
	W83627THF   = 0x8280,
	W83627SF    = 0x5950,
	W83637HF    = 0x7080,
	W83667HG    = 0xA510,
	W83667HGB   = 0xB350,
	W83687THF   = 0x8541,
	W83697HF    = 0x6010,
	W83697SF    = 0x6810,

	// Fintek
	F71858      = 0x0507,
	F71862      = 0x0601,
	F71868A     = 0x1106,
	F71869      = 0x0814,
	F71869A     = 0x1007,
	F71882      = 0x0541,
	F71889AD    = 0x1005,
	F71889ED    = 0x0909,
	F71889F     = 0x0723,
	F71808E     = 0x0901,

	// Nuvoton
	NCT6771F    = 0xB470,
	NCT6776F    = 0xC330,
	NCT6779D    = 0xC560,
	NCT6791D    = 0xC803,
	NCT6792D    = 0xC911,
	NCT6793D    = 0xD121,
	NCT6795D    = 0xD352,
	NCT6796D    = 0xD423,
	NCT6797D    = 0xD451,
	NCT6798D    = 0xD428,
	NCT679BD    = 0xD42B,
};

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
	static constexpr uint8_t SuperIOConfigControlRegister = 0x02;
	static constexpr uint8_t SuperIOChipIDRegister        = 0x20;
	static constexpr uint8_t SuperIOBaseAddressRegister   = 0x60;
	static constexpr uint8_t SuperIODeviceSelectRegister  = 0x07;

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
	 *  Constructor / Destructor
	 */
	SuperIODevice() = default;
	virtual ~SuperIODevice() = default;

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
	virtual uint8_t getLdn() = 0;

	/**
	 *  Getters
	 */
	uint16_t getDeviceAddress() { return deviceAddress; }
	i386_ioport_t getDevicePort() { return devicePort; }
	const SMCSuperIO* getSmcSuperIO() { return smcSuperIO; }
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
