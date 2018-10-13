//
//  NuvotonDevice.hpp
//
//  Sensors implementation for Nuvoton SuperIO device
//
//  Based on https://github.com/kozlek/HWSensors/blob/master/SuperIOSensors/NCT677xSensors.cpp
//  @author joedm
//

#ifndef _NUVOTONDEVICE_HPP
#define _NUVOTONDEVICE_HPP

#include "SuperIODevice.hpp"
#include "WinbondFamilyDevice.hpp"

namespace Nuvoton {
	
	class Device : public WindbondFamilyDevice {
	private:
		static constexpr uint8_t NUVOTON_MAX_TACHOMETER_COUNT		= 6;
		static constexpr uint8_t NUVOTON_ADDRESS_REGISTER_OFFSET     = 0x05;
		static constexpr uint8_t NUVOTON_DATA_REGISTER_OFFSET        = 0x06;
		static constexpr uint8_t NUVOTON_BANK_SELECT_REGISTER        = 0x4E;
		static constexpr uint8_t NUVOTON_REG_ENABLE                  = 0x30;
		static constexpr uint8_t NUVOTON_HWMON_IO_SPACE_LOCK         = 0x28;
		static constexpr uint16_t NUVOTON_VENDOR_ID                  = 0x5CA3;
		
		// Hardware Monitor Registers
		static constexpr uint16_t NUVOTON_VENDOR_ID_HIGH_REGISTER    = 0x804F;
		static constexpr uint16_t NUVOTON_VENDOR_ID_LOW_REGISTER     = 0x004F;
		static constexpr uint16_t NUVOTON_VOLTAGE_VBAT_REG           = 0x0551;
		
		static constexpr uint16_t NUVOTON_TEMPERATURE_REG[]          = { 0x027, 0x073, 0x075, 0x077, 0x150, 0x250, 0x62B, 0x62C, 0x62D };
		static constexpr uint16_t NUVOTON_TEMPERATURE_REG_NEW[]      = { 0x027, 0x073, 0x075, 0x077, 0x079, 0x07B, 0x150 };
		static constexpr uint16_t NUVOTON_TEMPERATURE_SEL_REG[]      = {	0x100, 0x200, 0x300, 0x800, 0x900, 0xa00 };
		
		static constexpr uint16_t NUVOTON_VOLTAGE_REG[]              = { 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x550, 0x551 };
		static constexpr float  NUVOTON_VOLTAGE_SCALE[]            = { 8,    8,    16,   16,   8,    8,    8,    16,    16 };
		
		static constexpr uint16_t NUVOTON_VOLTAGE_REG_NEW[]          = { 0x480, 0x481, 0x482, 0x483, 0x484, 0x485, 0x486, 0x487, 0x488, 0x489, 0x48A, 0x48B, 0x48C, 0x48D, 0x48E };
		
		static constexpr uint16_t NUVOTON_FAN_RPM_REG[]              = { 0x656, 0x658, 0x65A, 0x65C, 0x65E, 0x660 };
		static constexpr uint16_t NUVOTON_FAN_STOP_REG[]             = {	0x105, 0x205, 0x305, 0x805, 0x905, 0xa05 };
		
		static constexpr uint16_t NUVOTON_FAN_PWM_MODE_REG[]         = { 0x04,  0,     0,     0,     0,     0 };
		static constexpr uint16_t NUVOTON_PWM_MODE_MASK[]            = { 0x01,  0,     0,     0,     0,     0 };
		
		static constexpr uint16_t NUVOTON_FAN_PWM_MODE_OLD_REG[]     = { 0x04,  0x04,  0x12,  0,     0,     0 };
		static constexpr uint16_t NUVOTON_PWM_MODE_MASK_OLD[]        = { 0x01,  0x02,  0x01,  0,     0,     0 };
		
		static constexpr uint16_t NUVOTON_FAN_PWM_OUT_REG[]          = { 0x001, 0x003, 0x011, 0x013, 0x015, 0x017 };
		static constexpr uint16_t NUVOTON_FAN_PWM_COMMAND_REG[]      = { 0x109, 0x209, 0x309, 0x809, 0x909, 0xA09 };
		static constexpr uint16_t NUVOTON_FAN_CONTROL_MODE_REG[]     = { 0x102, 0x202, 0x302, 0x802, 0x902, 0xA02 };
		
		/* DC or PWM output fan configuration */
		static constexpr uint16_t NUVOTON_NCT6775_REG_PWM_MODE[]     = { 0x04,  0x04,  0x12,  0,     0,     0 };
		static constexpr uint16_t NUVOTON_NCT6775_PWM_MODE_MASK[]    = { 0x01,  0x02,  0x01,  0,     0,     0 };
		static constexpr uint16_t NUVOTON_NCT6776_REG_PWM_MODE[]     = { 0x04,  0,     0,     0,     0,     0 };
		static constexpr uint16_t NUVOTON_NCT6776_PWM_MODE_MASK[]    = { 0x01,  0,     0,     0,     0,     0 };
		
		static constexpr uint16_t NUVOTON_NCT6775_REG_PWM[]          = { 0x109, 0x209, 0x309, 0x809, 0x909, 0xa09 };
		static constexpr uint16_t NUVOTON_NCT6775_REG_PWM_READ[]     = {	0x01,  0x03,  0x11,  0x13,  0x15,  0xa09 };
		
		static constexpr uint16_t NUVOTON_NCT6775_REG_FAN_MODE[]     = {	0x102, 0x202, 0x302, 0x802, 0x902, 0xa02 };
		
		static constexpr uint16_t NUVOTON_NCT6775_REG_TEMP_SEL[]     = {	0x100, 0x200, 0x300, 0x800, 0x900, 0xa00 };
		
		static constexpr int32_t RPM_THRESHOLD1 = (int32_t)(1.35e6 / 0xFFFF);
		static constexpr int32_t RPM_THRESHOLD2 = (int32_t)(1.35e6 / 0x1FFF);

		/**
		 *  Initialize
		 */
		using InitializeFunc = void (Device::*)();
		void initialize679xx();

		/**
		 *  A stub. Does nothing.
		 */
		void stub() { }

		/**
		 *  Tachometer
		 */
		using TachometerUpdateFunc = uint16_t (Device::*)(uint8_t);
		uint16_t tachometers[NUVOTON_MAX_TACHOMETER_COUNT] = { 0 };

		/**
		 * Reads tachometers data. Invoked from update() only.
		 */
		void updateTachometers();
		
		/**
		 *  Implementation for tachometer reading. Invoked via descriptor only.
		 */
		uint16_t tachometerReadDefault(uint8_t);
		
		/**
		 *  Struct for describing supported devices
		 */
		struct DeviceDescriptor {
			SuperIOModel ID;
			/* Maximum tachometer sensors this Nuvoton device has */
			uint8_t tachometerCount;
			/* Tachometer RPM threshold: values less than this are invalid */
			uint32_t tachometerMinRPM;
			/* Base register for tachometers reading */
			uint16_t tachometerRpmBaseRegister;
			/* Function for tachometer reading */
			TachometerUpdateFunc updateTachometer;
			/* Init proc */
			InitializeFunc initialize;
		};
		
		/**
		 *  The descriptor instance for this device
		 */
		const DeviceDescriptor& deviceDescriptor;
		
	public:
		/**
		 * Reads a byte from device's register
		 */
		uint8_t readByte(uint16_t reg);
		
		/**
		 * Writes a byte into device's register
		 */
		void writeByte(uint16_t reg, uint8_t value);
		
		/**
		 *  Overrides
		 */
		virtual const char* getVendor() override { return "Nuvoton"; }
		virtual void setupKeys(VirtualSMCAPI::Plugin &vsmcPlugin) override;
		virtual void update() override;
		virtual void powerStateChanged(unsigned long state) override;
		virtual uint16_t getTachometerValue(uint8_t index) override { return tachometers[index]; }

		/**
		 *  Ctors
		 */
		Device(const DeviceDescriptor &desc, uint16_t address, i386_ioport_t port, SMCSuperIO* sio)
			: WindbondFamilyDevice(desc.ID, address, port, sio), deviceDescriptor(desc) {}
		Device() = delete;
		
		/**
		 *  Supported devices
		 */
		static const DeviceDescriptor _NCT6771F;
		static const DeviceDescriptor _NCT6776F;
		static const DeviceDescriptor _NCT6779D;
		static const DeviceDescriptor _NCT6791D;
		static const DeviceDescriptor _NCT6792D;
		static const DeviceDescriptor _NCT6793D;
		static const DeviceDescriptor _NCT6795D;
		static const DeviceDescriptor _NCT6796D;
		
		/**
		 *  Device factory
		 */
		static SuperIODevice* detect(SMCSuperIO* sio);
		
		/**
		 *  Device factory helper
		 */
		static const DeviceDescriptor* detectModel(uint16_t id, uint8_t &ldn);
	};
}

#endif // _NUVOTONDEVICE_HPP
