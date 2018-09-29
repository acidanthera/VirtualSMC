//
//  SuperIODevice.h
//
//  SuperIO Chip data
//
//  Based on https://github.com/kozlek/HWSensors/blob/master/SuperIOSensors/SuperIODevice.h
//  @Author joedm.
//

#ifndef _SUPERIONUB_H
#define _SUPERIONUB_H

#include <IOKit/IOService.h>
#include <architecture/i386/pio.h>

#define kSuperIOHWMAddress  "monitor-address"
#define kSuperIOControlPort "control-port"
#define kSuperIOModelValue  "model"

#define kSuperIOModelName   "model-name"
#define kSuperIOVendorName  "vendor-name"

#define kSuperIODeviceID  "device-id"

// Entering ports
const UInt8 kSuperIOPorts[]               = {0x2e, 0x4e};

// Registers
const UInt8 kSuperIOConfigControlRegister = 0x02;
const UInt8 kSuperIOChipIDRegister        = 0x20;
const UInt8 kSuperIOBaseAddressRegister   = 0x60;
const UInt8 kSuperIODeviceSelectRegister  = 0x07;

// Logical device number
const UInt8 kWinbondHardwareMonitorLDN    = 0x0B;
const UInt8 kF71858HardwareMonitorLDN     = 0x02;
const UInt8 kFintekITEHardwareMonitorLDN  = 0x04;

enum SuperIOModel
{
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
    
    // Winbond
    W83627DHG	= 0xA020,
	W83627UHG	= 0xA230,
    W83627DHGP	= 0xB070,
    W83627EHF	= 0x8800,    
    W83627HF	= 0x5200,
	W83627THF	= 0x8280,
	W83627SF	= 0x5950,
	W83637HF	= 0x7080,
    W83667HG	= 0xA510,
    W83667HGB	= 0xB350,
    W83687THF	= 0x8541,
	W83697HF	= 0x6010,
	W83697SF	= 0x6810,
    
    // Fintek
    F71858		= 0x0507,
    F71862		= 0x0601,
    F71868A     = 0x1106,
    F71869		= 0x0814,
    F71869A     = 0x1007,
    F71882		= 0x0541,
    F71889AD    = 0x1005,
    F71889ED	= 0x0909,
    F71889F		= 0x0723,
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
};

inline UInt8 superio_listen_port_byte(i386_ioport_t port, UInt8 reg)
{
	outb(port, reg);
	return inb(port + 1);
}

inline UInt16 superio_listen_port_word(i386_ioport_t port, UInt8 reg)
{
	return ((superio_listen_port_byte(port, reg) << 8) | superio_listen_port_byte(port, reg + 1));
}

inline void superio_write_port_byte(i386_ioport_t port, UInt8 reg, UInt8 value)
{
	outb(port, reg);
	outb(port + 1, value);
}

inline void superio_select_logical_device(i386_ioport_t port, UInt8 reg)
{
	outb(port, kSuperIODeviceSelectRegister);
	outb(port + 1, reg);
}

inline void ite_family_enter(i386_ioport_t port)
{
    outb(port, 0x87);
	outb(port, 0x01);
	outb(port, 0x55);
	outb(port, 0x55);
}

inline void ite_family_exit(i386_ioport_t port)
{
    outb(port, kSuperIOConfigControlRegister);
	outb(port + 1, 0x02);
}

inline void winbond_family_enter(i386_ioport_t port)
{
    outb(port, 0x87);
	outb(port, 0x87);
}

inline void winbond_family_exit(i386_ioport_t port)
{
    outb(port, 0xAA);
}

inline const char* superio_get_model_name(UInt16 model)
{
    switch (model) {
        case IT8512F:       return "IT8512F";
        case IT8705F:       return "IT8705F";
        case IT8712F:       return "IT8712F";
        case IT8716F:       return "IT8716F";
        case IT8718F:       return "IT8718F";
        case IT8720F:       return "IT8720F";
        case IT8721F:       return "IT8721F";
        case IT8726F:       return "IT8726F";
        case IT8620E:       return "IT8620E"; // monitoring device of IT8620E is compatible with IT8728F
        case IT8628E:       return "IT8628E";
        case IT8728F:       return "IT8728F";
        case IT8686E:       return "IT8686E";
        case IT8752F:       return "IT8752F";
        case IT8771E:       return "IT8771E";
        case IT8772E:       return "IT8772E";
        case IT8792E:       return "IT8792E";
            
        case W83627DHG:     return "W83627DHG";
        case W83627UHG:     return "W83627UHG";
        case W83627DHGP:    return "W83627DHGP";
        case W83627EHF:     return "W83627EHF";
        case W83627HF:      return "W83627HF";
        case W83627THF:     return "W83627THF";
        case W83627SF:      return "W83627SF";
        case W83637HF:      return "W83637HF";
        case W83667HG:      return "W83667HG";
        case W83667HGB:     return "W83667HGB";
        case W83687THF:     return "W83687THF";
        case W83697HF:      return "W83697HF";
        case W83697SF:      return "W83697SF";
            
        case F71858:        return "F71858";
        case F71862:        return "F71862";
        case F71868A:       return "F71868A";
        case F71869:        return "F71869";
        case F71869A:       return "F71869A";
        case F71882:        return "F71882";
        case F71889AD:      return "F71889AD";
        case F71889ED:      return "F71889ED";
        case F71889F:       return "F71889F";
        case F71808E:       return "F71808E";
            
        case NCT6771F:      return "NCT6771F";
        case NCT6776F:      return "NCT6776F";
        case NCT6779D:      return "NCT6779D";
        case NCT6791D:      return "NCT6791D";
        case NCT6792D:      return "NCT6792D";
        case NCT6793D:      return "NCT6793D";
        case NCT6795D:      return "NCT6795D";
        case NCT6796D:      return "NCT6796D";
    }
    
    return "unknown";
}

struct SuperIODevice
{
    UInt16              id;
    UInt16              model;
    UInt8               ldn;
    const char*         vendor;
    UInt16              address;
    i386_ioport_t       port;
	
	bool                detect(bool isGigabyte = false);
	
private:
    bool                detectWinbondFamilyChip();
    bool                detectITEFamilyChip();
};

#endif
