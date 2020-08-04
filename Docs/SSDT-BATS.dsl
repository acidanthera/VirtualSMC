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
    External (_SB_.PCI0.LPCB.H_EC.BCL1, FieldUnitObj)
    External (_SB_.PCI0.LPCB.H_EC.BCL2, FieldUnitObj)
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
            Name (PKG1, Package (0x10)
            {
                // config
                0x00001F7F,
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
                // Temperature (0x8), AppleSmartBattery format
                0xFFFFFFFF, 
                // TimeToFull (0x9), minutes (0xFF)
                0xFFFFFFFF, 
                // TimeToEmpty (0xa), minutes (0)
                0xFFFFFFFF, 
                // ChargeLevel (0xb), percentage
                0xFFFFFFFF, 
                // AverageRate (0xc), signed 16-bit integer
                0xFFFFFFFF, 
                0xFFFFFFFF, 
                0xFFFFFFFF, 
                0xFFFFFFFF
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
                PKG1 [0x08] = B1B2 (BTM1, BTM2)
                PKG1 [0x09] = B1B2 (BCL1, BCL2)
                PKG1 [0x0A] = B1B2 (BCW1, BCW2)
                PKG1 [0x0B] = B1B2 (BPR1, BPR2)
                PKG1 [0x0C] = B1B2 (BAR1, BAR2)
            }

            Return (PKG1)
        } // CBIS
    } // BAT1
}
//EOF
