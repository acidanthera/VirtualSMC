#  Battery Information Supplement

Many laptops have more information available in EC, such as manufacture date, battery temperature.
In this quick guide we will explain on how to implement add support for those data under macOS.

**Note:** You need to complete patching process for battery reporting first. Some EC fields maybe not used
in `_BIF`, `_BIX` and `_BST`, but you still need to split them for proper reading.

## Requirements

- [MaciASL](https://github.com/acidanthera/MaciASL)
- Dump of native DSDT
- [SSDT-BATS](https://github.com/acidanthera/VirtualSMC/blob/master/Docs/SSDT-BATS.dsl)

## Implementation
Required method (`CBIS`, `CBSS`) for supplement information is under the same active battery device for `_BST` etc.,
and you can just append them to your battery's patched SSDT.

The methods wil return a package consisting of following information, currently all fields are 16-bit integer.

### Static Information `CBIS`

- Config (0x0)

This number is used to indicate enabled information below by setting corresponding bit to 1.

Set calculator to programmer mode can help you a lot.

- ManufactureDate (0x1)

This data is in AppleSmartBattery format, while some battery manufacturer supplied same format in EC.

The number can be calculated using follow formula:

```
day + month << 5 + (year - 1980) << 9
```

- PackLotCode (0x2)

This field seems to be empty on some Macbooks.

- PCBLotCode (0x3)

Same above.

- FirmwareVersion (0x4)

Fill it with corresponding EC fields, displayed as hex.

- HardwareVersion (0x5)

Same above.

- BatteryVersion (0x6)

Same above.

### Dynamic Status `CBSS`

While index for dynamic status in config starts from 0x10, index in SSDT package still starts from 0.

- Temperature (0x10)

This data is in AppleSmartBattery format, while some battery manufacturer supplied same format in EC.

The number can be calculated using follow formula:

```
celcius * 10 + 2731
```

Current issues (won't solve):

When battery's fully charged or not charging, temperature won't be updated since no ACPI polling happens.

- TimeToFull (0x11)

If there's no such field in EC, ignore it and leave the calculation to SMCBatteryManger. Same for following fields.

Valid integer in minutes when charging, otherwise 0xFF.

- TimeToEmpty (0x12)

Valid integer in minutes when discharging, otherwise 0.

- ChargeLevel (0x13)

0 - 100 for percentage.

- AverageRate (0x14)

Valid *signed* integer in mA. Double check if you have valid value since this bit will disable quickPoll.

- ChargingCurrent (0x15)

Valid integer in mA.

- ChargingVoltage (0x16)

Valid integer in mV.

## How to dump your EC fields
In windows, you can use [nbfc](https://github.com/hirschmann/nbfc/wiki/Probe-the-EC%27s-registers)
or [RWEverything](http://rweverything.com).
On macOS, you may try append a method to some known _QXX button and
[Enable ACPI Debugging](https://pikeralpha.wordpress.com/2013/12/23/enabling-acpi-debugging/).
Or here is another example using [OS-X-ACPI-Debug](https://github.com/RehabMan/OS-X-ACPI-Debug). 

```
Method (DBG0, 1, Serialized)
{
	Local0 = Zero
	While ((Local0 < 0xFF))
	{
		\RMDT.P2 (ToHexString (Local0), RECB (Local0, 0x80))
		Sleep (0x10)
		Local0 += 0x10
	}
}
```
Or there's an experimental driver [(YogaSMC)](https://github.com/zhen-zen/YogaSMC) to read EC fields.

## Special on IdeaPad laptops
This note is developed on EC layout of the Lenovo Yoga 900 series (same from 900 to C940), with battery-related
EC region starting at 0x60 (`B1ST`). For other layouts, see your `GSBI` method and check naming or index for hints. 
You may also use those information to construct a `_BIX` method instead of `_BIF` and fix bogus data.

```
Name (RETB, Buffer (0x53){})
RETB [Zero] = B1DC /* Design Capacity, _BIF[1] / _BIX[2] */
RETB [One] = (B1DC >> 0x08)
RETB [0x02] = B1FC /* Last Full Charge, _BIF[2] / _BIX[3] */
RETB [0x03] = (B1FC >> 0x08)
RETB [0x04] = B1RC /* Remaining Capacity, _BST[2] */
RETB [0x05] = (B1RC >> 0x08)
RETB [0x06] = BDCW /* Time to Empty, aka ATTE */
RETB [0x07] = (BDCW >> 0x08)
RETB [0x08] = BDCL /* Time to Full, aka ATTF */
RETB [0x09] = (BDCL >> 0x08)
RETB [0x0A] = B1VT /* Present Voltage, _BST[3] */
RETB [0x0B] = (B1VT >> 0x08)
RETB [0x0C] = B1CR /* Present Charging Rate, _BST[1] */
RETB [0x0D] = (B1CR >> 0x08)
RETB [0x0E] = B1TM /* Battery Temperature, aka BTMP */
RETB [0x0F] = (B1TM >> 0x08)
RETB [0x10] = B1DT /* Battery 1 Manufacture Date */
RETB [0x11] = (B1DT >> 0x08)
RETB [0x12] = B2DT /* Battery 2 Manufacture Date */
RETB [0x13] = (B2DT >> 0x08)
RETB [0x14] = B1DV /* Design Voltage, _BIF[4] / _BIX[5] */
RETB [0x15] = (B1DV >> 0x08)
RETB [0x16] = B1CH /* Battery Type, _BIX[0x12] */
RETB [0x17] = (B1CH >> 0x08)
RETB [0x18] = (B1CH >> 0x10)
RETB [0x19] = (B1CH >> 0x18)

RETB [0x20] = B1DN /* Model Number, _BIX[0x10] */
RETB [0x21] = (B1DN >> 0x08)
RETB [0x22] = (B1DN >> 0x10)
RETB [0x23] = (B1DN >> 0x18)
RETB [0x24] = (B1DN >> 0x20)
RETB [0x25] = (B1DN >> 0x28)
RETB [0x26] = (B1DN >> 0x30)
RETB [0x27] = (B1DN >> 0x38)
RETB [0x28] = B1MA /* OEM Information, _BIX[0x13] */
RETB [0x29] = (B1MA >> 0x08)
RETB [0x2A] = (B1MA >> 0x10)
RETB [0x2B] = (B1MA >> 0x18)
RETB [0x2C] = (B1MA >> 0x20)
RETB [0x2D] = (B1MA >> 0x28)
RETB [0x2E] = (B1MA >> 0x30)
RETB [0x2F] = (B1MA >> 0x38)

RETB [0x34] = BARL /* Bar coding number, little confidence */
RETB [0x35] = (BARL >> 0x08)
RETB [0x36] = (BARL >> 0x10)
RETB [0x37] = (BARL >> 0x18)
RETB [0x38] = (BARL >> 0x20)
RETB [0x39] = (BARL >> 0x28)
RETB [0x3A] = (BARL >> 0x30)
RETB [0x3B] = (BARL >> 0x38)
RETB [0x3C] = BARH /* Bar coding number, little confidence */
RETB [0x3D] = (BARH >> 0x08)
RETB [0x3E] = (BARH >> 0x10)
RETB [0x3F] = (BARH >> 0x18)
RETB [0x40] = (BARH >> 0x20)
RETB [0x41] = (BARH >> 0x28)
RETB [0x42] = (BARH >> 0x30)
RETB [0x43] = (BARH >> 0x38)

RETB [0x4B] = BMIL /* BatMaker */
RETB [0x4C] = BMIH /* BatMaker */
RETB [0x4D] = HIDL /* Hardware ID */
RETB [0x4E] = HIDH /* Hardware ID */
RETB [0x4F] = FMVL /* Firmware Version */
RETB [0x50] = FMVH /* Firmware Version */
RETB [0x51] = DAVL /* Part of Firmware Version, little confidence */
RETB [0x52] = DAVH /* Part of Firmware Version, little confidence */
```

And other fields for those in interest

```
Offset (0x68), 
BAPR,   16, // Percent / Charge Level
Offset (0x6E), 
B1CC,   16, // ChargingCurrent
B1CV,   16, // ChargingVoltage
Offset (0x7E), 
B1AR,   16, // Average Rate
Offset (0xA4), 
B1CY,   16, // _BIX[8] - Cycle Count
Offset (0xC0), 
BARN,   184, // _BIX[0x11] - Serial Number - "XXXXXSSSYYY"
```
