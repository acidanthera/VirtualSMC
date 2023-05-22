#  Dual Battery Support

macOS has buggy dual battery support and we won't have correct battery status without doing some changes.
Many laptops do have dual batteries, one common example are Lenovo ThinkPad Laptops.
In this quick guide we will explain on how to implement dual battery support under macOS.

**Note:** this quick guide only covers the dual battery implementation part and not the patching process for battery reporting.

## Requirements

- [MaciASL](https://github.com/acidanthera/MaciASL)
- Dump of native DSDT
- Plist Editor (use your prefered one)
- [SSDT-BATC](https://github.com/acidanthera/VirtualSMC/blob/master/Docs/SSDT-BATC.dsl)
- [BATC-Sample.plist](https://github.com/acidanthera/VirtualSMC/blob/master/Docs/BATC-Sample.plist) as a sample for ACPI patches

## Implementation

As an example for this quick guide we will use the Lenovo ThinkPad T440S laptop.
In order to have correct battery status, in a dual battery scenario, we need to use SSDT-BATC which combines two batteries into a single one and aside from the SSDT itself, we also need to rename the notifiers of BAT0 & BAT1 to BATC

Notifiers can be in short and long versions, some examples are:

```
Notify (BAT0, 0x80)
Notify (BAT1, 0x81)
Notify (\_SB.PCI0.LPC.EC.BAT0, 0x01)
Notify (\_SB.PCI0.LPCB.EC.BAT1, 0x03)
```

In order for our SSDT-BATC to work, we should change these notifiers to BATC, in our example:

```
Notify (BATC, 0x80)
Notfiy (BATC, 0x81)
Notify (\_SB.PCI0.LPC.EC.BATC, 0x01)
Notify (\_SB.PCI0.LPCB.EC.BATC, 0x03)
```

The appropriate way to change these notifiers would be to open DSDT and search for notifiers that have `BAT0` or `BAT1` (all of them should be changed or else the Battery Reporting will fail/ will be incorrect)
But instead of just binpatch changing, the appropriate way would be to look on which method is that notifier executed from and instead of binpatch changing the notifier itself, we instead rename the method where the notifier is executed and provide the patched Method into a new SSDT or inside SSDT-BATC.

Open your native DSDT with MaciASL and start searching for `notify (BAT` and `notify (\_SB.PCI0.LPC.EC.BAT` 
(Keep in mind that the path can be different from laptop models, some have LPC, some LPCB), in our example:

```
Notify (BAT0, 0x80)
Notify (BAT1, 0x80)
Notify (BAT0, 0x81)
Notify (\_SB.PCI0.LPC.EC.BAT1, 0x01)
```

Now let's find out on which Method(s) each Notifier(s) is included/executed, here i listed all the methods containing the notifiers that need patching, in our example:

```
                   Method (_Q22, 0, NotSerialized)  // _Qxx: EC Query, xx=0x00-0xFF
                   {
                       CLPM ()
                       If (HB0A)
                       {
                           Notify (BAT0, 0x80) // Status Change
                       }

                       If (HB1A)
                       {
                           Notify (BAT1, 0x80) // Status Change
                       }
                   }

                   Method (_Q4A, 0, NotSerialized)  // _Qxx: EC Query, xx=0x00-0xFF
                   {
                       CLPM ()
                       Notify (BAT0, 0x81) // Information Change
                   }

                   Method (_Q4B, 0, NotSerialized)  // _Qxx: EC Query, xx=0x00-0xFF
                   {
                       CLPM ()
                       Notify (BAT0, 0x80) // Status Change
                   }

                   Method (_Q4D, 0, NotSerialized)  // _Qxx: EC Query, xx=0x00-0xFF
                   {
                       CLPM ()
                       If (\BT2T)
                       {
                           If ((^BAT1.SBLI == 0x01))
                           {
                               Sleep (0x0A)
                               If ((HB1A && (SLUL == 0x00)))
                               {
                                   ^BAT1.XB1S = 0x01
                                   Notify (\_SB.PCI0.LPC.EC.BAT1, 0x01) // Device Check
                               }
                           }
                           ElseIf ((SLUL == 0x01))
                           {
                               ^BAT1.XB1S = 0x00
                               Notify (\_SB.PCI0.LPC.EC.BAT1, 0x03) // Eject Request
                           }
                       }

                       If ((^BAT1.B1ST & ^BAT1.XB1S))
                       {
                           Notify (BAT1, 0x80) // Status Change
                       }
                   }

                   Method (_Q24, 0, NotSerialized)  // _Qxx: EC Query, xx=0x00-0xFF
                   {
                       CLPM ()
                       Notify (BAT0, 0x80) // Status Change
                   }

                   Method (_Q25, 0, NotSerialized)  // _Qxx: EC Query, xx=0x00-0xFF
                   {
                       If ((^BAT1.B1ST & ^BAT1.XB1S))
                       {
                           CLPM ()
                           Notify (BAT1, 0x80) // Status Change
                       }
                   }

                   Method (BATW, 1, NotSerialized)
                   {
                       If (\BT2T)
                       {
                           Local0 = \_SB.PCI0.LPC.EC.BAT1.XB1S
                           If ((HB1A && !SLUL))
                           {
                               Local1 = 0x01
                           }
                           Else
                           {
                               Local1 = 0x00
                           }

                           If ((Local0 ^ Local1))
                           {
                               \_SB.PCI0.LPC.EC.BAT1.XB1S = Local1
                               Notify (\_SB.PCI0.LPC.EC.BAT1, 0x01) // Device Check
                           }
                       }
                   }
```

As you can see here, the Methods that we need to patch are:

```
Method (_Q22)
Method (_Q4A)
Method (_Q4B)
Method (_Q4D)
Method (_Q24)
Method (_Q25)
Method (BATW)
```

So first step we need to do is to rename the native Method and place the patched one into a separate SSDT, example:
`Method (_Q22)` can be renamed to `XQ22` from **config.plist > ACPI > Patch >**

**Find: 5F513232 00**
(5F513232 00 is _Q22 translated into hex code)

**Replace: 58513232 00**
(58513232 00 is XQ22 translated into hex code)

Do the renames for all the found methods that will need to be patched like in the example below:
(keep in mind to check if code with similar name doesn't exist in ACPI/DSDT, if yes, use different name)

```
Method (_Q22) to Method XQ22
Method (_Q4A) to Method XQ4A
Method (_Q4B) to Method XQ4B
Method (_Q4D) to Method XQ4D
Method (_Q24) to Method XQ24
Method (_Q25) to Method XQ25
Method (BATW) to Method XATW
```

(keep in mind to check if code with similar name doesn't exist in ACPI/DSDT, if yes, use different name)

Next we copy the original `Method (_Q22)` from DSDT and paste it into a new SSDT:

```
Method (_Q22, 0, NotSerialized)  // _Qxx: EC Query, xx=0x00-0xFF
{
    CLPM ()
    If (HB0A)
    {
        Notify (BAT0, 0x80) // Status Change
    }
    If (HB1A)
    {
        Notify (BAT1, 0x80) // Status Change
    }
}
```

Now we change Notifiers from `BAT0` and `BAT1` to `BATC`:

```
Method (_Q22, 0, NotSerialized)  // _Qxx: EC Query, xx=0x00-0xFF
{
    CLPM ()
    If (HB0A)
    {
        Notify (BATC, 0x80) // Status Change
    }
    If (HB1A)
    {
        Notify (BATC, 0x80) // Status Change
    }
}
```

Next we add the `If (_OSI ("Darwin"))`.
This makes sure that the patched code into our SSDT works only if the operating system is macOS ("Darwin").
So now the  patched code will work on macOS but won't in other Operating Systems like Windows or Linux, etc.
For this we add the `Else` into the code and call the original method that we renamed to Method XQ22.
By this, if macOS, the patched SSDT code will work, if it's something else like Windows or Linux it will load the original method that we renamed to XQ22 in our case, like shown below:

```
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
```

When doing such edits you must always press compile on MaciASL to see if the compiler shows any errors.
Keep in mind that you will need to add External Declarations for each Methods and objects that are included as part of the code into the new SSDT, example like in our case with `Method (_Q22)`:

```
External (_SB.PCI0.LPC.EC.XQ22, MethodObj)
External (CLPM, MethodObj)
External (HB0A, MethodObj)
External (HB1A, MethodObj)
```

Repeat the same process for all `Method(s)`

Here is the completed example SSDT:

```
// Patched Notifiers SSDT for dual battery support
// An example from Lenovo ThinkPad T440S

DefinitionBlock ("", "SSDT", 2, "ACDT", "DBAT", 0x00000000)
{
    External (_SB_.PCI0.LPC.EC, DeviceObj)
    External (_SB.PCI0.LPC.EC.BATC, DeviceObj)
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
    }
}
```

Once you are done and the compiler returns no errors, save the SSDT with patched `Method(s)` and `SSDT-BATC` as `.aml` file and place it on your:
**EFI > OC > ACPI >** folder and make sure to add/list it under **config.plist > ACPI > Add** section

Now you can reboot and you should have correct Battery Status and reporting under macOS.

Refer to the sample [BATC-Sample.plist](https://github.com/acidanthera/VirtualSMC/blob/master/Docs/BATC-Sample.plist) for ACPI renames so you can adapt patches according to your ACPI.
