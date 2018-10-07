//
//  NuvotonDevice.h
//
//  Sensors implementation for Nuvoton SuperIO device
//
//  Based on https://github.com/kozlek/HWSensors/blob/master/SuperIOSensors/NCT677xSensors.cpp
//  @Author joedm.
//

#ifndef _NUVOTONDEVICE_H
#define _NUVOTONDEVICE_H

#include "SuperIODevice.hpp"

class NuvotonGenericDevice : public SuperIODevice {
protected:
	static constexpr UInt8 NUVOTON_MAX_TACHOMETER_COUNT		= 6;
	static constexpr UInt8 NUVOTON_ADDRESS_REGISTER_OFFSET     = 0x05;
	static constexpr UInt8 NUVOTON_DATA_REGISTER_OFFSET        = 0x06;
	static constexpr UInt8 NUVOTON_BANK_SELECT_REGISTER        = 0x4E;
	static constexpr UInt8 NUVOTON_REG_ENABLE                  = 0x30;
	static constexpr UInt8 NUVOTON_HWMON_IO_SPACE_LOCK         = 0x28;
	static constexpr UInt16 NUVOTON_VENDOR_ID                  = 0x5CA3;
	
	// Hardware Monitor Registers
	static constexpr UInt16 NUVOTON_VENDOR_ID_HIGH_REGISTER    = 0x804F;
	static constexpr UInt16 NUVOTON_VENDOR_ID_LOW_REGISTER     = 0x004F;
	static constexpr UInt16 NUVOTON_VOLTAGE_VBAT_REG           = 0x0551;
	
	static constexpr UInt16 NUVOTON_TEMPERATURE_REG[]          = { 0x027, 0x073, 0x075, 0x077, 0x150, 0x250, 0x62B, 0x62C, 0x62D };
	static constexpr UInt16 NUVOTON_TEMPERATURE_REG_NEW[]      = { 0x027, 0x073, 0x075, 0x077, 0x079, 0x07B, 0x150 };
	static constexpr UInt16 NUVOTON_TEMPERATURE_SEL_REG[]      = {	0x100, 0x200, 0x300, 0x800, 0x900, 0xa00 };
	
	static constexpr UInt16 NUVOTON_VOLTAGE_REG[]              = { 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x550, 0x551 };
	static constexpr float  NUVOTON_VOLTAGE_SCALE[]            = { 8,    8,    16,   16,   8,    8,    8,    16,    16 };
	
	static constexpr UInt16 NUVOTON_VOLTAGE_REG_NEW[]          = { 0x480, 0x481, 0x482, 0x483, 0x484, 0x485, 0x486, 0x487, 0x488, 0x489, 0x48A, 0x48B, 0x48C, 0x48D, 0x48E };
	
	static constexpr UInt16 NUVOTON_FAN_RPM_REG[]              = { 0x656, 0x658, 0x65A, 0x65C, 0x65E, 0x660 };
	static constexpr UInt16 NUVOTON_FAN_STOP_REG[]             = {	0x105, 0x205, 0x305, 0x805, 0x905, 0xa05 };
	
	static constexpr UInt16 NUVOTON_FAN_PWM_MODE_REG[]         = { 0x04,  0,     0,     0,     0,     0 };
	static constexpr UInt16 NUVOTON_PWM_MODE_MASK[]            = { 0x01,  0,     0,     0,     0,     0 };
	
	static constexpr UInt16 NUVOTON_FAN_PWM_MODE_OLD_REG[]     = { 0x04,  0x04,  0x12,  0,     0,     0 };
	static constexpr UInt16 NUVOTON_PWM_MODE_MASK_OLD[]        = { 0x01,  0x02,  0x01,  0,     0,     0 };
	
	static constexpr UInt16 NUVOTON_FAN_PWM_OUT_REG[]          = { 0x001, 0x003, 0x011, 0x013, 0x015, 0x017 };
	static constexpr UInt16 NUVOTON_FAN_PWM_COMMAND_REG[]      = { 0x109, 0x209, 0x309, 0x809, 0x909, 0xA09 };
	static constexpr UInt16 NUVOTON_FAN_CONTROL_MODE_REG[]     = { 0x102, 0x202, 0x302, 0x802, 0x902, 0xA02 };
	
	/* DC or PWM output fan configuration */
	static constexpr UInt16 NUVOTON_NCT6775_REG_PWM_MODE[]     = { 0x04,  0x04,  0x12,  0,     0,     0 };
	static constexpr UInt16 NUVOTON_NCT6775_PWM_MODE_MASK[]    = { 0x01,  0x02,  0x01,  0,     0,     0 };
	static constexpr UInt16 NUVOTON_NCT6776_REG_PWM_MODE[]     = { 0x04,  0,     0,     0,     0,     0 };
	static constexpr UInt16 NUVOTON_NCT6776_PWM_MODE_MASK[]    = { 0x01,  0,     0,     0,     0,     0 };
	
	static constexpr UInt16 NUVOTON_NCT6775_REG_PWM[]          = { 0x109, 0x209, 0x309, 0x809, 0x909, 0xa09 };
	static constexpr UInt16 NUVOTON_NCT6775_REG_PWM_READ[]     = {	0x01,  0x03,  0x11,  0x13,  0x15,  0xa09 };
	
	static constexpr UInt16 NUVOTON_NCT6775_REG_FAN_MODE[]     = {	0x102, 0x202, 0x302, 0x802, 0x902, 0xa02 };
	
	static constexpr UInt16 NUVOTON_NCT6775_REG_TEMP_SEL[]     = {	0x100, 0x200, 0x300, 0x800, 0x900, 0xa00 };

	/**
	 * Reads a byte from device's register
	 */
	UInt8 readByte(UInt16 reg);

	/**
	 * Writes a byte into device's register
	 */
	void writeByte(UInt16 reg, UInt8 value);
	
	/* Maximum tachometer sensors Nuvoton ever supported */
	UInt8 tachometerMaxCount { NUVOTON_MAX_TACHOMETER_COUNT };

	/* Tachometer RPM threshold: values less than this are invalid */
	UInt8 tachometerMinRPM;

	/* Tachometers data */
	UInt32 tachometers[NUVOTON_MAX_TACHOMETER_COUNT];
	
	/* Base register for tachometers reading */
	UInt16 tachometerRpmBaseRegister;

	/* Constructor is protected */
	NuvotonGenericDevice(UInt16 deviceID) : SuperIODevice(deviceID) {}
	
	/**
	 * Reads tachometers data. Invoked from update() only.
	 */
	virtual void updateTachometers();
public:
	virtual const char* getVendor() override { return "Nuvoton"; }
	virtual void setupKeys(VirtualSMCAPI::Plugin &vsmcPlugin) override;
	virtual void update() override;
	
	UInt32 getTachometerValue(UInt8 index) { return tachometers[index]; }
};

class NuvotonLegacyDevice : public NuvotonGenericDevice {
protected:
	NuvotonLegacyDevice(UInt16 deviceID) : NuvotonGenericDevice(deviceID)
	{
		tachometerRpmBaseRegister = 0x656;
	}
};

class NuvotonNCT6771FDevice : public NuvotonLegacyDevice {
public:
	NuvotonNCT6771FDevice() : NuvotonLegacyDevice(NCT6771F)
	{
		tachometerMaxCount = 3;
		tachometerMinRPM = (UInt32)(1.35e6 / 0xFFFF);
	}
};

class NuvotonNCT6776FDevice : public NuvotonLegacyDevice {
public:
	NuvotonNCT6776FDevice() : NuvotonLegacyDevice(NCT6776F)
	{
		tachometerMaxCount = 3;
		tachometerMinRPM = (UInt32)(1.35e6 / 0x1FFF);
	}
};

class NuvotonNCT6779DDevice : public NuvotonGenericDevice {
public:
	NuvotonNCT6779DDevice() : NuvotonGenericDevice(NCT6779D)
	{
		tachometerMaxCount = 5;
		tachometerRpmBaseRegister = 0x4C0;
		tachometerMinRPM = (UInt32)(1.35e6 / 0x1FFF);
	}
};

class NuvotonNCT679xxDevice : public NuvotonGenericDevice {
public:
	NuvotonNCT679xxDevice(UInt16 deviceID) : NuvotonGenericDevice(deviceID)
	{
		tachometerMaxCount = 6;
		tachometerRpmBaseRegister = 0x4C0;
		tachometerMinRPM = (UInt32)(1.35e6 / 0x1FFF);
	}
};

// TODO: make this generic?
class TachometerKey : public VirtualSMCValue {
protected:
	SMCSuperIO *sio;
	size_t index;
	NuvotonGenericDevice *device;
	SMC_RESULT readAccess() override;
public:
	TachometerKey(SMCSuperIO *sio, NuvotonGenericDevice *device, size_t index) : sio(sio), index(index), device(device) {}
};

#endif // _NUVOTONDEVICE_H
