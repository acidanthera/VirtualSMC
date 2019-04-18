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
#include <VirtualSMCSDK/kern_vsmcapi.hpp>

#define CALL_MEMBER_FUNC(obj, func)  ((obj).*(func))

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

class SMCSuperIO;

class SuperIODevice
{
private:
	const i386_ioport_t devicePort;
	const SuperIOModel deviceModel;
	const uint16_t deviceAddress;
	const SMCSuperIO* smcSuperIO;

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
	SuperIODevice(SuperIOModel deviceModel, uint16_t address, i386_ioport_t port, SMCSuperIO* sio)
		: deviceModel(deviceModel), deviceAddress(address), devicePort(port), smcSuperIO(sio)  { }
	SuperIODevice() = delete;
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

	/**
	 *  Misc
	 */
	static inline const char* getModelName(SuperIOModel model) {
		switch (model) {
			case IT8512F:       return "ITE IT8512F";
			case IT8705F:       return "ITE IT8705F";
			case IT8712F:       return "ITE IT8712F";
			case IT8716F:       return "ITE IT8716F";
			case IT8718F:       return "ITE IT8718F";
			case IT8720F:       return "ITE IT8720F";
			case IT8721F:       return "ITE IT8721F";
			case IT8726F:       return "ITE IT8726F";
			case IT8620E:       return "ITE IT8620E"; // monitoring device of IT8620E is compatible with IT8728F
			case IT8628E:       return "ITE IT8628E";
			case IT8728F:       return "ITE IT8728F";
			case IT8686E:       return "ITE IT8686E";
			case IT8752F:       return "ITE IT8752F";
			case IT8771E:       return "ITE IT8771E";
			case IT8772E:       return "ITE IT8772E";
			case IT8792E:       return "ITE IT8792E";
			case IT8688E:       return "ITE IT8688E";
			case IT8795E:       return "ITE IT8795E";
			case IT8665E:       return "ITE IT8665E";

			case W83627DHG:     return "Winbond W83627DHG";
			case W83627UHG:     return "Winbond W83627UHG";
			case W83627DHGP:    return "Winbond W83627DHGP";
			case W83627EHF:     return "Winbond W83627EHF";
			case W83627HF:      return "Winbond W83627HF";
			case W83627THF:     return "Winbond W83627THF";
			case W83627SF:      return "Winbond W83627SF";
			case W83637HF:      return "Winbond W83637HF";
			case W83667HG:      return "Winbond W83667HG";
			case W83667HGB:     return "Winbond W83667HGB";
			case W83687THF:     return "Winbond W83687THF";
			case W83697HF:      return "Winbond W83697HF";
			case W83697SF:      return "Winbond W83697SF";

			case F71858:        return "Fintek F71858";
			case F71862:        return "Fintek F71862";
			case F71868A:       return "Fintek F71868A";
			case F71869:        return "Fintek F71869";
			case F71869A:       return "Fintek F71869A";
			case F71882:        return "Fintek F71882";
			case F71889AD:      return "Fintek F71889AD";
			case F71889ED:      return "Fintek F71889ED";
			case F71889F:       return "Fintek F71889F";
			case F71808E:       return "Fintek F71808E";

			case NCT6771F:      return "Nuvoton NCT6771F";
			case NCT6776F:      return "Nuvoton NCT6776F";
			case NCT6779D:      return "Nuvoton NCT6779D";
			case NCT6791D:      return "Nuvoton NCT6791D";
			case NCT6792D:      return "Nuvoton NCT6792D";
			case NCT6793D:      return "Nuvoton NCT6793D";
			case NCT6795D:      return "Nuvoton NCT6795D";
			case NCT6796D:      return "Nuvoton NCT6796D";
			case NCT6797D:      return "Nuvoton NCT6797D";
			case NCT6798D:      return "Nuvoton NCT6798D";
			case NCT679BD:      return "Nuvoton NCT679BD";
			default:            return "Unknown";
		}
	}

public:
	/**
	 *  Initialize procedures run here. This is mostly for work with hardware.
	 *  FIXME: not in use so far. Consider to remove.
	 */
	virtual void initialize() { }

	/**
	 *  Power state handling.
	 *  @param state is a SMCSuperIO::PowerState value.
	 */
	virtual void powerStateChanged(unsigned long state) { }

	/**
	 *  Set up SMC keys.
	 */
	virtual void setupKeys(VirtualSMCAPI::Plugin &vsmcPlugin) = 0;

	/**
	 *  Invoked by timer event. Sync write ops with key accessors if necessary.
	 */
	virtual void update() = 0;

	/**
	 *  Accessors
	 */
	virtual uint16_t getTachometerValue(uint8_t index) = 0;
	virtual const char* getModelName() = 0;

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
