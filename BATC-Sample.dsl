// SSDT-BATC.dsl example with Patched Notifiers included
// An example from Lenovo ThinkPad T440S

DefinitionBlock ("", "SSDT", 2, "ACDT", "BATC", 0x00000000)
{
    External (_SB_.PCI0.LPC.EC, DeviceObj)
    External (_SB_.PCI0.LPC.EC.BAT0, DeviceObj)
    External (_SB_.PCI0.LPC.EC.BAT0._BIF, MethodObj)
    External (_SB_.PCI0.LPC.EC.BAT0._BIX, MethodObj)
    External (_SB_.PCI0.LPC.EC.BAT0._BST, MethodObj)
    External (_SB_.PCI0.LPC.EC.BAT0._HID, IntObj)
    External (_SB_.PCI0.LPC.EC.BAT0._STA, MethodObj)
    External (_SB_.PCI0.LPC.EC.BAT1, DeviceObj)
    External (_SB_.PCI0.LPC.EC.BAT1._BIF, MethodObj)
    External (_SB_.PCI0.LPC.EC.BAT1._BIX, MethodObj)
    External (_SB_.PCI0.LPC.EC.BAT1._BST, MethodObj)
    External (_SB_.PCI0.LPC.EC.BAT1._HID, IntObj)
    External (_SB_.PCI0.LPC.EC.BAT1._STA, MethodObj)
    External (CLPM, MethodObj)
    External (HB0A, MethodObj)
    External (HB1A, MethodObj)
    External (BT2T, FieldUnitObj)
    External (SLUL, FieldUnitObj)
    External (_SB.PCI0.LPC.EC.BAT1.SBLI, FieldUnitObj)
    External (_SB.PCI0.LPC.EC.BAT1.XB1S, FieldUnitObj)
    External (\_SB.PCI0.LPC.EC.BAT1.B1ST, FieldUnitObj)
    External (_SB.PCI0.LPC.EC.XQ22, MethodObj)
    External (_SB.PCI0.LPC.EC.XQ4A, MethodObj)
    External (_SB.PCI0.LPC.EC.XQ4B, MethodObj)
    External (_SB.PCI0.LPC.EC.XQ4D, MethodObj)
    External (_SB.PCI0.LPC.EC.XQ24, MethodObj)
    External (_SB.PCI0.LPC.EC.XQ25, MethodObj)
    External (_SB.PCI0.LPC.EC.XATW, MethodObj)

    Scope (\_SB.PCI0.LPC.EC)
    {
        Device (BATC)
        {
            Name (_HID, EisaId ("PNP0C0A"))
            Name (_UID, 0x02)

            Method (_INI)
            {
                If (_OSI ("Darwin"))
                {
                    // disable original battery objects by setting invalid _HID
                    ^^BAT0._HID = 0
                    ^^BAT1._HID = 0
                }
            }

            Method (_STA)
            {
                If (_OSI ("Darwin"))
                {
                    // call original _STA for BAT0 and BAT1
                    // result is bitwise OR between them
                    Return (^^BAT0._STA () | ^^BAT1._STA ())
                }
                Else
                {
                    Return (Zero)
                }
            }

            Method (_BIF)
            {
                // Local0 BAT0._BIF
                // Local1 BAT1._BIF
                // Local2 BAT0._STA
                // Local3 BAT1._STA
                // Local4/Local5 scratch

                // gather and validate data from BAT0
                Local0 = ^^BAT0._BIF ()
                Local2 = ^^BAT0._STA ()
                If (0x1f == Local2)
                {
                    // check for invalid design capacity
                    Local4 = DerefOf (Local0 [1])
                    If (!Local4 || Ones == Local4) { Local2 = 0; }
                    // check for invalid last full charge capacity
                    Local4 = DerefOf (Local0 [2])
                    If (!Local4 || Ones == Local4) { Local2 = 0; }
                    // check for invalid design voltage
                    Local4 = DerefOf (Local0 [4])
                    If (!Local4 || Ones == Local4) { Local2 = 0; }
                }
                // gather and validate data from BAT1
                Local1 = ^^BAT1._BIF ()
                Local3 = ^^BAT1._STA ()
                If (0x1f == Local3)
                {
                    // check for invalid design capacity
                    Local4 = DerefOf (Local1 [1])
                    If (!Local4 || Ones == Local4) { Local3 = 0; }
                    // check for invalid last full charge capacity
                    Local4 = DerefOf (Local1 [2])
                    If (!Local4 || Ones == Local4) { Local3 = 0; }
                    // check for invalid design voltage
                    Local4 = DerefOf (Local1 [4])
                    If (!Local4 || Ones == Local4) { Local3 = 0; }
                }
                // find primary and secondary battery
                If (0x1f != Local2 && 0x1f == Local3)
                {
                    // make primary use BAT1 data
                    Local0 = Local1 // BAT1._BIF result
                    Local2 = Local3 // BAT1._STA result
                    Local3 = 0  // no secondary battery
                }
                // combine batteries into Local0 result if possible
                If (0x1f == Local2 && 0x1f == Local3)
                {
                    // _BIF 0 Power Unit - leave BAT0 value
                    // _BIF 1 Design Capacity - add BAT0 and BAT1 values
                    Local4 = DerefOf (Local0 [1])
                    Local5 = DerefOf (Local1 [1])
                    If (0xffffffff != Local4 && 0xffffffff != Local5)
                    {
                        Local0 [1] = Local4 + Local5
                    }
                    // _BIF 2 Last Full Charge Capacity - add BAT0 and BAT1 values
                    Local4 = DerefOf (Local0 [2])
                    Local5 = DerefOf (Local1 [2])
                    If (0xffffffff != Local4 && 0xffffffff != Local5)
                    {
                        Local0 [2] = Local4 + Local5
                    }
                    // _BIF 3 Battery Technology - leave BAT0 value
                    // _BIF 4 Design Voltage - average between BAT0 and BAT1 values
                    Local4 = DerefOf (Local0 [4])
                    Local5 = DerefOf (Local1 [4])
                    If (0xffffffff != Local4 && 0xffffffff != Local5)
                    {
                        Local0 [4] = (Local4 + Local5) / 2
                    }
                    // _BIF 5 Design Capacity of Warning - add BAT0 and BAT1 values
                    Local0 [5] = DerefOf (Local0 [5]) + DerefOf (Local1 [5])
                    // _BIF 6 Design Capacity of Low - add BAT0 and BAT1 values
                    Local0 [6] = DerefOf (Local0 [6]) + DerefOf (Local1 [6])
                    // _BIF 7 Battery Capacity Granularity 1 - add BAT0 and BAT1 values
                    Local4 = DerefOf (Local0 [7])
                    Local5 = DerefOf (Local1 [7])
                    If (0xffffffff != Local4 && 0xffffffff != Local5)
                    {
                        Local0 [7] = Local4 + Local5
                    }
                    // _BIF 8 Battery Capacity Granularity 2 - add BAT0 and BAT1 values
                    Local4 = DerefOf (Local0 [8])
                    Local5 = DerefOf (Local1 [8])
                    If (0xffffffff != Local4 && 0xffffffff != Local5)
                    {
                        Local0 [8] = Local4 + Local5
                    }
                    // _BIF 9 Model Number - concatenate BAT0 and BAT1 values
                    Local0 [0x09] = Concatenate (Concatenate (DerefOf (Local0 [0x09]), " / "), DerefOf (Local1 [0x09]))
                    // _BIF a Serial Number - concatenate BAT0 and BAT1 values
                    Local0 [0x0a] = Concatenate (Concatenate (DerefOf (Local0 [0x0a]), " / "), DerefOf (Local1 [0x0a]))
                    // _BIF b Battery Type - concatenate BAT0 and BAT1 values
                    Local0 [0x0b] = Concatenate (Concatenate (DerefOf (Local0 [0x0b]), " / "), DerefOf (Local1 [0x0b]))
                    // _BIF c OEM Information - concatenate BAT0 and BAT1 values
                    Local0 [0x0c] = Concatenate (Concatenate (DerefOf (Local0 [0x0c]), " / "), DerefOf (Local1 [0x0c]))
                }

                Return (Local0)
            } // _BIF

            Method (_BIX)
            {
                // Local0 BAT0._BIX
                // Local1 BAT1._BIX
                // Local2 BAT0._STA
                // Local3 BAT1._STA
                // Local4/Local5 scratch

                // gather and validate data from BAT0
                Local0 = ^^BAT0._BIX ()
                Local2 = ^^BAT0._STA ()
                If (0x1f == Local2)
                {
                    // check for invalid design capacity
                    Local4 = DerefOf (Local0 [2])
                    If (!Local4 || Ones == Local4) { Local2 = 0; }
                    // check for invalid last full charge capacity
                    Local4 = DerefOf (Local0 [3])
                    If (!Local4 || Ones == Local4) { Local2 = 0; }
                    // check for invalid design voltage
                    Local4 = DerefOf (Local0 [5])
                    If (!Local4 || Ones == Local4) { Local2 = 0; }
                }
                // gather and validate data from BAT1
                Local1 = ^^BAT1._BIX ()
                Local3 = ^^BAT1._STA ()
                If (0x1f == Local3)
                {
                    // check for invalid design capacity
                    Local4 = DerefOf (Local1 [2])
                    If (!Local4 || Ones == Local4) { Local3 = 0; }
                    // check for invalid last full charge capacity
                    Local4 = DerefOf (Local1 [3])
                    If (!Local4 || Ones == Local4) { Local3 = 0; }
                    // check for invalid design voltage
                    Local4 = DerefOf (Local1 [5])
                    If (!Local4 || Ones == Local4) { Local3 = 0; }
                }
                // find primary and secondary battery
                If (0x1f != Local2 && 0x1f == Local3)
                {
                    // make primary use BAT1 data
                    Local0 = Local1 // BAT1._BIX result
                    Local2 = Local3 // BAT1._STA result
                    Local3 = 0  // no secondary battery
                }
                // combine batteries into Local0 result if possible
                If (0x1f == Local2 && 0x1f == Local3)
                {
                    // _BIX 0 Revision - leave BAT0 value
                    // _BIX 1 Power Unit - leave BAT0 value
                    // _BIX 2 Design Capacity - add BAT0 and BAT1 values
                    Local4 = DerefOf (Local0 [2])
                    Local5 = DerefOf (Local1 [2])
                    If (0xffffffff != Local4 && 0xffffffff != Local5)
                    {
                        Local0 [2] = Local4 + Local5
                    }
                    // _BIX 3 Last Full Charge Capacity - add BAT0 and BAT1 values
                    Local4 = DerefOf (Local0 [3])
                    Local5 = DerefOf (Local1 [3])
                    If (0xffffffff != Local4 && 0xffffffff != Local5)
                    {
                        Local0 [3] = Local4 + Local5
                    }
                    // _BIX 4 Battery Technology - leave BAT0 value
                    // _BIX 5 Design Voltage - average between BAT0 and BAT1 values
                    Local4 = DerefOf (Local0 [5])
                    Local5 = DerefOf (Local1 [5])
                    If (0xffffffff != Local4 && 0xffffffff != Local5)
                    {
                        Local0 [5] = (Local4 + Local5) / 2
                    }
                    // _BIX 6 Design Capacity of Warning - add BAT0 and BAT1 values
                    Local0 [6] = DerefOf (Local0 [6]) + DerefOf (Local1 [6])
                    // _BIX 7 Design Capacity of Low - add BAT0 and BAT1 values
                    Local0 [7] = DerefOf (Local0 [7]) + DerefOf (Local1 [7])
                    // _BIX 8 Cycle Count - average between BAT0 and BAT1 values
                    Local4 = DerefOf (Local0 [8])
                    Local5 = DerefOf (Local1 [8])
                    If (0xffffffff != Local4 && 0xffffffff != Local5)
                    {
                        Local0 [8] = (Local4 + Local5) / 2
                    }
                    // _BIX 9 Measurement Accuracy - average between BAT0 and BAT1 values
                    Local0 [9] = (DerefOf (Local0 [9]) + DerefOf (Local1 [9])) / 2
                    // _BIX 0xa Max Sampling Time - average between BAT0 and BAT1 values
                    Local4 = DerefOf (Local0 [0xa])
                    Local5 = DerefOf (Local1 [0xa])
                    If (0xffffffff != Local4 && 0xffffffff != Local5)
                    {
                        Local0 [0xa] = (Local4 + Local5) / 2
                    }
                    // _BIX 0xb Min Sampling Time - average between BAT0 and BAT1 values
                    Local4 = DerefOf (Local0 [0xb])
                    Local5 = DerefOf (Local1 [0xb])
                    If (0xffffffff != Local4 && 0xffffffff != Local5)
                    {
                        Local0 [0xb] = (Local4 + Local5) / 2
                    }
                    // _BIX 0xc Max Averaging Interval - average between BAT0 and BAT1 values
                    Local0 [0xc] = (DerefOf (Local0 [0xc]) + DerefOf (Local1 [0xc])) / 2
                    // _BIX 0xd Min Averaging Interval - average between BAT0 and BAT1 values
                    Local0 [0xd] = (DerefOf (Local0 [0xd]) + DerefOf (Local1 [0xd])) / 2
                    // _BIX 0xe Battery Capacity Granularity 1 - add BAT0 and BAT1 values
                    Local4 = DerefOf (Local0 [0xe])
                    Local5 = DerefOf (Local1 [0xe])
                    If (0xffffffff != Local4 && 0xffffffff != Local5)
                    {
                        Local0 [0xe] = Local4 + Local5
                    }
                    // _BIX 0xf Battery Capacity Granularity 2 - add BAT0 and BAT1 values
                    Local4 = DerefOf (Local0 [0xf])
                    Local5 = DerefOf (Local1 [0xf])
                    If (0xffffffff != Local4 && 0xffffffff != Local5)
                    {
                        Local0 [0xf] = Local4 + Local5
                    }
                    // _BIX 10 Model Number - concatenate BAT0 and BAT1 values
                    Local0 [0x10] = Concatenate (Concatenate (DerefOf (Local0 [0x10]), " / "), DerefOf (Local1 [0x10]))
                    // _BIX 11 Serial Number - concatenate BAT0 and BAT1 values
                    Local0 [0x11] = Concatenate (Concatenate (DerefOf (Local0 [0x11]), " / "), DerefOf (Local1 [0x11]))
                    // _BIX 12 Battery Type - concatenate BAT0 and BAT1 values
                    Local0 [0x12] = Concatenate (Concatenate (DerefOf (Local0 [0x12]), " / "), DerefOf (Local1 [0x12]))
                    // _BIX 13 OEM Information - concatenate BAT0 and BAT1 values
                    Local0 [0x13] = Concatenate (Concatenate (DerefOf (Local0 [0x13]), " / "), DerefOf (Local1 [0x13]))
                    // _BIX 14 Battery Swapping Capability - leave BAT0 value for now
                }
                Return (Local0)
            } // _BIX

            Method (_BST)
            {
                // Local0 BAT0._BST
                // Local1 BAT1._BST
                // Local2 BAT0._STA
                // Local3 BAT1._STA
                // Local4/Local5 scratch

                // gather battery data from BAT0
                Local0 = ^^BAT0._BST ()
                Local2 = ^^BAT0._STA ()
                If (0x1f == Local2)
                {
                    // check for invalid remaining capacity
                    Local4 = DerefOf (Local0 [2])
                    If (!Local4 || Ones == Local4) { Local2 = 0; }
                }
                // gather battery data from BAT1
                Local1 = ^^BAT1._BST ()
                Local3 = ^^BAT1._STA ()
                If (0x1f == Local3)
                {
                    // check for invalid remaining capacity
                    Local4 = DerefOf (Local1 [2])
                    If (!Local4 || Ones == Local4) { Local3 = 0; }
                }
                // find primary and secondary battery
                If (0x1f != Local2 && 0x1f == Local3)
                {
                    // make primary use BAT1 data
                    Local0 = Local1 // BAT1._BST result
                    Local2 = Local3 // BAT1._STA result
                    Local3 = 0  // no secondary battery
                }
                // combine batteries into Local0 result if possible
                If (0x1f == Local2 && 0x1f == Local3)
                {
                    // _BST 0 - Battery State - if one battery is charging, then charging, else discharging
                    Local4 = DerefOf (Local0 [0])
                    Local5 = DerefOf (Local1 [0])
                    If (Local4 != Local5)
                    {
                        If (Local4 == 2 || Local5 == 2)
                        {
                            // 2 = charging
                            Local0 [0] = 2
                        }
                        ElseIf (Local4 == 1 || Local5 == 1)
                        {
                            // 1 = discharging
                            Local0 [0] = 1
                        }
                        ElseIf (Local4 == 3 || Local5 == 3)
                        {
                            Local0 [0] = 3
                        }
                        ElseIf (Local4 == 4 || Local5 == 4)
                        {
                            // critical
                            Local0 [0] = 4
                        }
                        ElseIf (Local4 == 5 || Local5 == 5)
                        {
                            // critical and discharging
                            Local0 [0] = 5
                        }
                        // if none of the above, just leave as BAT0 is
                    }

                    // _BST 1 - Battery Present Rate - add BAT0 and BAT1 values
                    Local0 [1] = DerefOf (Local0 [1]) + DerefOf (Local1 [1])
                    // _BST 2 - Battery Remaining Capacity - add BAT0 and BAT1 values
                    Local0 [2] = DerefOf (Local0 [2]) + DerefOf (Local1 [2])
                    // _BST 3 - Battery Present Voltage - average between BAT0 and BAT1 values
                    Local0 [3] = (DerefOf (Local0 [3]) + DerefOf (Local1 [3])) / 2
                }
                Return (Local0)
            } // _BST
        } // BATC
        // Patched Notifier Methods
        Method (_Q22, 0, NotSerialized)
        {
            If (_OSI ("Darwin"))
            {
                CLPM ()
                If (HB0A)
                {
                    Notify (BATC, 0x80)
                }

                If (HB1A)
                {
                    Notify (BATC, 0x80)
                }
            }
            Else
            {
                \_SB.PCI0.LPC.EC.XQ22 ()
            }
        }
        Method (_Q4A, 0, NotSerialized)
        {
            If (_OSI ("Darwin"))
            {
                CLPM ()
                Notify (BATC, 0x81)
            }
            Else
            {
                \_SB.PCI0.LPC.EC.XQ4A ()
            }
        }
        Method (_Q4B, 0, NotSerialized)
        {
            If (_OSI ("Darwin"))
            {
                CLPM ()
                Notify (BATC, 0x80)
            }
            Else
            {
                \_SB.PCI0.LPC.EC.XQ4B ()
            }
        }
        Method (_Q4D, 0, NotSerialized)
        {
            If (_OSI ("Darwin"))
            {
                CLPM ()
                If (\BT2T)
                {
                    If (LEqual (^BAT1.SBLI, 0x01))
                    {
                        Sleep (0x0A)
                        If (LAnd (HB1A, LEqual (SLUL, 0x00)))
                        {
                            Store (0x01, ^BAT1.XB1S)
                            Notify (\_SB.PCI0.LPC.EC.BATC, 0x01)
                        }
                    }
                    ElseIf (LEqual (SLUL, 0x01))
                    {
                        Store (0x00, ^BAT1.XB1S)
                        Notify (\_SB.PCI0.LPC.EC.BATC, 0x03)
                    }
                }

                If (And (^BAT1.B1ST, ^BAT1.XB1S))
                {
                    Notify (BATC, 0x80)
                }
            }
            Else
            {
                \_SB.PCI0.LPC.EC.XQ4D ()
            }
        }
        Method (_Q24, 0, NotSerialized)
        {
            If (_OSI ("Darwin"))
            {
                CLPM ()
                Notify (BATC, 0x80)
            }
            Else
            {
                \_SB.PCI0.LPC.EC.XQ24 ()
            }
        }
        Method (_Q25, 0, NotSerialized)
        {
            If (_OSI ("Darwin"))
            {
                If (And (^BAT1.B1ST, ^BAT1.XB1S))
                {
                    CLPM ()
                    Notify (BATC, 0x80)
                }
            }
            Else
            {
                \_SB.PCI0.LPC.EC.XQ25 ()
            }
        }
        Method (BATW, 1, NotSerialized)
        {
            If (_OSI ("Darwin"))
            {
                If (\BT2T)
                {
                    Store (\_SB.PCI0.LPC.EC.BAT1.XB1S, Local0)
                    If (LAnd (HB1A, LNot (SLUL)))
                    {
                        Store (0x01, Local1)
                    }
                    Else
                    {
                        Store (0x00, Local1)
                    }

                    If (XOr (Local0, Local1))
                    {
                        Store (Local1, \_SB.PCI0.LPC.EC.BAT1.XB1S)
                        Notify (\_SB.PCI0.LPC.EC.BATC, 0x01)
                    }
                }
            }
            Else
            {
                \_SB.PCI0.LPC.EC.XATW ()
            }
        }
    } // Scope
}
