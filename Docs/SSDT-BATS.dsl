//
// Refer to Battery Supplement Information.md for details and formats
//

DefinitionBlock ("", "SSDT", 2, "ACDT", "BATS", 0x00000000)
{
    // Match your battery device (PNP0C0A) path
    External (_SB_.PCI0.LPCB.H_EC.BAT1, DeviceObj)
    External (_SB_.PCI0.LPCB.H_EC.ECAV, IntObj)

    External (_SB_.PCI0.LPCB.H_EC.B1T1, FieldUnitObj)
    External (_SB_.PCI0.LPCB.H_EC.B1T2, FieldUnitObj)
    External (_SB_.PCI0.LPCB.H_EC.BAR1, FieldUnitObj)
    External (_SB_.PCI0.LPCB.H_EC.BAR2, FieldUnitObj)
    External (_SB_.PCI0.LPCB.H_EC.BCC1, FieldUnitObj)
    External (_SB_.PCI0.LPCB.H_EC.BCC2, FieldUnitObj)
    External (_SB_.PCI0.LPCB.H_EC.BCL1, FieldUnitObj)
    External (_SB_.PCI0.LPCB.H_EC.BCL2, FieldUnitObj)
    External (_SB_.PCI0.LPCB.H_EC.BCV1, FieldUnitObj)
    External (_SB_.PCI0.LPCB.H_EC.BCV2, FieldUnitObj)
    External (_SB_.PCI0.LPCB.H_EC.BCW1, FieldUnitObj)
    External (_SB_.PCI0.LPCB.H_EC.BCW2, FieldUnitObj)
    External (_SB_.PCI0.LPCB.H_EC.BMIH, FieldUnitObj)
    External (_SB_.PCI0.LPCB.H_EC.BMIL, FieldUnitObj)
    External (_SB_.PCI0.LPCB.H_EC.BPR1, FieldUnitObj)
    External (_SB_.PCI0.LPCB.H_EC.BPR2, FieldUnitObj)
    External (_SB_.PCI0.LPCB.H_EC.BTM1, FieldUnitObj)
    External (_SB_.PCI0.LPCB.H_EC.BTM2, FieldUnitObj)
    External (_SB_.PCI0.LPCB.H_EC.DAVH, FieldUnitObj)
    External (_SB_.PCI0.LPCB.H_EC.DAVL, FieldUnitObj)
    External (_SB_.PCI0.LPCB.H_EC.FMVH, FieldUnitObj)
    External (_SB_.PCI0.LPCB.H_EC.FMVL, FieldUnitObj)
    External (_SB_.PCI0.LPCB.H_EC.FUSH, FieldUnitObj)
    External (_SB_.PCI0.LPCB.H_EC.FUSL, FieldUnitObj)
    External (_SB_.PCI0.LPCB.H_EC.HIDH, FieldUnitObj)
    External (_SB_.PCI0.LPCB.H_EC.HIDL, FieldUnitObj)

    External (\B1B2, MethodObj)

    Scope (\_SB.PCI0.LPCB.H_EC.BAT1)
    {
        Method (CBIS, 0, Serialized)
        {
            Name (PKG1, Package (0x08)
            {
                // config, double check if you have valid AverageRate before
                // fliping that bit to 0x007F007F since it will disable quickPoll
                0x006F007F,
                // ManufactureDate (0x1), AppleSmartBattery format
                0xFFFFFFFF, 
                // PackLotCode (0x2)
                0xFFFFFFFF, 
                // PCBLotCode (0x3)
                0xFFFFFFFF, 
                // FirmwareVersion (0x4)
                0xFFFFFFFF, 
                // HardwareVersion (0x5)
                0xFFFFFFFF, 
                // BatteryVersion (0x6)
                0xFFFFFFFF, 
                0xFFFFFFFF, 
            })
            // Check your _BST method for similiar condition of EC accessibility
            If (ECAV)
            {
                PKG1 [One] = B1B2 (B1T1, B1T2)
                PKG1 [0x02] = B1B2 (FUSL, FUSH)
                PKG1 [0x03] = B1B2 (BMIL, BMIH)
                PKG1 [0x04] = B1B2 (FMVL, FMVH)
                PKG1 [0x05] = B1B2 (HIDL, HIDH)
                PKG1 [0x06] = B1B2 (DAVL, DAVH)
            }

            Return (PKG1)
        } // CBIS

        Method (CBSS, 0, Serialized)
        {
            Name (PKG1, Package (0x08)
            {
                // Temperature (0x10), AppleSmartBattery format
                0xFFFFFFFF, 
                // TimeToFull (0x11), minutes (0xFF)
                0xFFFFFFFF, 
                // TimeToEmpty (0x12), minutes (0)
                0xFFFFFFFF, 
                // ChargeLevel (0x13), percentage
                0xFFFFFFFF, 
                // AverageRate (0x14), mA (signed)
                0xFFFFFFFF, 
                // ChargingCurrent (0x15), mA
                0xFFFFFFFF, 
                // ChargingVoltage (0x16), mV
                0xFFFFFFFF, 
                0xFFFFFFFF
            })
            // Check your _BST method for similiar condition of EC accessibility
            If (ECAV)
            {
                PKG1 [Zero] = B1B2 (BTM1, BTM2)
                PKG1 [One] = B1B2 (BCL1, BCL2)
                PKG1 [0x02] = B1B2 (BCW1, BCW2)
                PKG1 [0x03] = B1B2 (BPR1, BPR2)
                PKG1 [0x04] = B1B2 (BAR1, BAR2)
                PKG1 [0x05] = B1B2 (BCC1, BCC2)
                PKG1 [0x06] = B1B2 (BCV1, BCV2)
            }

            Return (PKG1)
        } // CBSS
    } // BAT1
}
//EOF
