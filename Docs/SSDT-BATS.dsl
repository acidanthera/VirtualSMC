//
// Refer to Battery Supplement Information.md for details and formats
//

DefinitionBlock ("", "SSDT", 2, "ACDT", "BATS", 0x00000000)
{
    External (_SB_.PCI0.LPCB.EC, DeviceObj)
    External (_SB_.PCI0.LPCB.EC.BAT1, DeviceObj)
    External (_SB_.PCI0.LPCB.EC.B1T1, FieldUnitObj)
    External (_SB_.PCI0.LPCB.EC.B1T2, FieldUnitObj)
    External (_SB_.PCI0.LPCB.EC.BMIH, FieldUnitObj)
    External (_SB_.PCI0.LPCB.EC.BMIL, FieldUnitObj)
    External (_SB_.PCI0.LPCB.EC.BTM1, FieldUnitObj)
    External (_SB_.PCI0.LPCB.EC.BTM2, FieldUnitObj)
    External (_SB_.PCI0.LPCB.EC.DAVH, FieldUnitObj)
    External (_SB_.PCI0.LPCB.EC.DAVL, FieldUnitObj)
    External (_SB_.PCI0.LPCB.EC.FMVH, FieldUnitObj)
    External (_SB_.PCI0.LPCB.EC.FMVL, FieldUnitObj)
    External (_SB_.PCI0.LPCB.EC.FUSH, FieldUnitObj)
    External (_SB_.PCI0.LPCB.EC.FUSL, FieldUnitObj)
    External (_SB_.PCI0.LPCB.EC.HIDH, FieldUnitObj)
    External (_SB_.PCI0.LPCB.EC.HIDL, FieldUnitObj)
    External (\B1B2, MethodObj)

    Scope (\_SB.PCI0.LPCB.EC)
    {
        Scope (BAT1)
        {
            Method (CBSS, 0, Serialized)
            {
                Name (PKG1, Package (0x10)
                {
                    0x017F,
                    0xFFFFFFFF, 
                    0xFFFFFFFF, 
                    0xFFFFFFFF, 
                    0xFFFFFFFF, 
                    0xFFFFFFFF, 
                    0xFFFFFFFF, 
                    0xFFFFFFFF, 
                    0xFFFFFFFF, 
                    0xFFFFFFFF, 
                    0xFFFFFFFF, 
                    0xFFFFFFFF, 
                    0xFFFFFFFF, 
                    0xFFFFFFFF, 
                    0xFFFFFFFF, 
                    0xFFFFFFFF
                })
                If (ECAV)
                {
                    PKG1 [One] = B1B2 (B1T1, B1T2)
                    PKG1 [0x02] = B1B2 (FUSL, FUSH)
                    PKG1 [0x03] = B1B2 (BMIL, BMIH)
                    PKG1 [0x04] = B1B2 (FMVL, FMVH)
                    PKG1 [0x05] = B1B2 (HIDL, HIDH)
                    PKG1 [0x06] = B1B2 (DAVL, DAVH)
                    PKG1 [0x08] = B1B2 (BTM1, BTM2)
                }

                Return (PKG1) /* \_SB_.PCI0.LPCB.H_EC.BAT1.CBSS.PKG1 */
            }
        } // BAT1
    } // Scope (...)
}
//EOF
