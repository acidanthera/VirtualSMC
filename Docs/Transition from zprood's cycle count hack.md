
RehabMan's ACPIBatteryManager supported a so-called @zprood's `_BIF` extension technique for providing cycle count. This technique was used to add cycle count into `_BIF` if it existed in some EC registers, but wasn't visible because the battery device only implemented `_BIF` method, but not ACPI 4.0 `_BIX`. Extending the `_BIF` return package doesn't conform to the ACPI specification and should be avoided. It's not supported by SMCBatteryManager. Instead, a proper `_BIX` implementation should be provided. Here I'll show an example of such transformation for @kecinzer's HP laptop, based on his SSDT.

**The original code with comments (I'll show everything for `BAT0` device, it's the same for `BAT1`):**
```c
Name (NBTI, Package(0x02)
{
    Package(0x0F)
    {
        0x01,
        0xFFFFFFFF,
        0xFFFFFFFF,
        0x01,
        0xFFFFFFFF,
        0x00,
        0x00,
        0x64,
        0x64,
        "Primary",
        "100000",
        "LIon",
        "Hewlett-Packard",
        Zero, // extension for cycle count
        Zero, // extension for temperature
    },
    Package (0x0F) { /* the same package for the second battery is not shown */ }
})

Method (BTIF, 1, Serialized) // it's _SB.BTIF
{
    Local0 = ^PCI0.LPCB.EC.BTIF (Arg0)
    If ((Local0 == 0xFF))
    {
        Return (Package (0x0D)
        {
            Zero, 
            0xFFFFFFFF, 
            0xFFFFFFFF, 
            One, 
            0xFFFFFFFF, 
            Zero, 
            Zero, 
            Zero, 
            Zero, 
            "", 
            "", 
            "", 
            Zero
        })
    }
    Else
    {
        Return (DerefOf (NBTI [Arg0]))
    }
}

Device (BAT0)
{
    /* other methods are not shown */
    Method (_BIF, 0, NotSerialized)  // _BIF: Battery Information
    {
        Return (BTIF (Zero))
    }
}
```

```c
Method (BTIF, 1, Serialized) // it's EC.BTIF
{
    ShiftLeft (One, Arg0, Local7)
    BTDR (One)
    If (LEqual (BSTA (Local7), 0x0F))
    {
        Return (0xFF)
    }

    Acquire (BTMX, 0xFFFF)
    Store (NGBF, Local0)
    Release (BTMX)
    If (LEqual (And (Local0, Local7), Zero))
    {
        Return (Zero)
    }

    Store (NDBS, Index (NBST, Arg0))
    Acquire (BTMX, 0xFFFF)
    Or (NGBT, Local7, NGBT)
    Release (BTMX)
    Acquire (ECMX, 0xFFFF)
    If (ECRG)
    {
        Store (Arg0, BSEL) // Battery index for dual-battery laptops

        // Battery Full Capacity (last full charge capacity)
        // It's either HP's mistake or intentional lie –
        // they are putting last full charge capacity
        // in both Last Full Charge Capacity and
        // Design Capacity field.
        // I'll correct it below.
        Store (B1B2 (BFC0, BFC1), Local0)
        Store (Local0, Index (DerefOf (Index (NBTI, Arg0)), One)) // Design Capacity
        Store (Local0, Index (DerefOf (Index (NBTI, Arg0)), 0x02)) // Last Full Charge Capacity
        Store (B1B2 (BDV0, BDV1), Index (DerefOf (Index (NBTI, Arg0)), 0x04)) // Design Voltage
        Multiply (B1B2 (BFC0, BFC1), NLB1, Local0)
        Divide (Local0, 0x64, /*Local3*/, Local4)
        Store (Local4, Index (DerefOf (Index (NBTI, Arg0)), 0x05)) // Design Capacity of Warning
        Multiply (B1B2 (BFC0, BFC1), NLO2, Local0)
        Divide (Local0, 0x64, /*Local3*/, Local4) 
        Store (Local4, Index (DerefOf (Index (NBTI, Arg0)), 0x06)) // Design Capacity of Low
        Store (B1B2 (BSN0, BSN1), Local0)
        Store (B1B2 (BDA0, BDA1), Local1)
        // Battery Cycle Count – this is zprood's extension, 
        // look that index is 0x0D, which is not allowed for _BIF
        Store (B1B2 (BCC0, BCC1), Index (DerefOf (Index (NBTI, Arg0)), 0x0D))
        // RehabMan's extension, temperature should not be provided here
        Acquire (\_SB.PCI0.LPCB.EC.ECMX, 0xFFFF)
        Store (5, \_SB.PCI0.LPCB.EC.CRZN)
        Store (\_SB.PCI0.LPCB.EC.TEMP, Local2)
        Release (\_SB.PCI0.LPCB.EC.ECMX)
        Add (Multiply (Local2, 10), 2732, Local2) // Celsius to .1K
        Store (Local2, Index (DerefOf (Index (NBTI, Arg0)), 0x0E))
    }

    Release (ECMX)
    Store (GBSS (Local0, Local1), Local2)
    Store (Local2, Index (DerefOf (Index (NBTI, Arg0)), 0x0A))
    Acquire (BTMX, 0xFFFF)
    And (NGBF, Not (Local7), NGBF)
    Release (BTMX)
    Return (Zero)
}
```

**First let's remove the extra fields from `_BIF` (and fix HP bug):**
```c
Name (NBTI, Package(0x02)
{
    Package(0x0D) // change package length back to 0x0D
    {
        0x01,
        0xFFFFFFFF,
        0xFFFFFFFF,
        0x01,
        0xFFFFFFFF,
        0x00,
        0x00,
        0x64,
        0x64,
        "Primary",
        "100000",
        "LIon",
        "Hewlett-Packard",
        // extra fields are removed
    },
    Package (0x0F) { /* the same package for the second battery is not shown */ }
})
```

```c
Method (BTIF, 1, Serialized) // it's EC.BTIF
{
    ShiftLeft (One, Arg0, Local7)
    BTDR (One)
    If (LEqual (BSTA (Local7), 0x0F))
    {
        Return (0xFF)
    }

    Acquire (BTMX, 0xFFFF)
    Store (NGBF, Local0)
    Release (BTMX)
    If (LEqual (And (Local0, Local7), Zero))
    {
        Return (Zero)
    }

    Store (NDBS, Index (NBST, Arg0))
    Acquire (BTMX, 0xFFFF)
    Or (NGBT, Local7, NGBT)
    Release (BTMX)
    Acquire (ECMX, 0xFFFF)
    If (ECRG)
    {
        Store (Arg0, BSEL)

        Store (B1B2 (BDC0, BDC1), Local0) // Fix for HP's bug – put Designed Capacity here
        Store (Local0, Index (DerefOf (Index (NBTI, Arg0)), One)) // Design Capacity
        Store (B1B2 (BFC0, BFC1), Local0)
        Store (Local0, Index (DerefOf (Index (NBTI, Arg0)), 0x02)) // Last Full Charge Capacity
        Store (B1B2 (BDV0, BDV1), Index (DerefOf (Index (NBTI, Arg0)), 0x04)) // Design Voltage
        Multiply (B1B2 (BFC0, BFC1), NLB1, Local0)
        Divide (Local0, 0x64, /*Local3*/, Local4)
        Store (Local4, Index (DerefOf (Index (NBTI, Arg0)), 0x05)) // Design Capacity of Warning
        Multiply (B1B2 (BFC0, BFC1), NLO2, Local0)
        Divide (Local0, 0x64, /*Local3*/, Local4) 
        Store (Local4, Index (DerefOf (Index (NBTI, Arg0)), 0x06)) // Design Capacity of Low
        Store (B1B2 (BSN0, BSN1), Local0)
        Store (B1B2 (BDA0, BDA1), Local1)
    }

    Release (ECMX)
    Store (GBSS (Local0, Local1), Local2)
    Store (Local2, Index (DerefOf (Index (NBTI, Arg0)), 0x0A))
    Acquire (BTMX, 0xFFFF)
    And (NGBF, Not (Local7), NGBF)
    Release (BTMX)
    Return (Zero)
}
```

**Then implement `_BIX` method:**
```c
Scope(\_SB.BAT0)
{
    Method (_BIX, 0, NotSerialized)
    {
        Return (BTIX (Zero))
    }
}
```

```c
Scope (\_SB)
{
    Method (BTIX, 1, Serialized)
    {
        Local0 = ^PCI0.LPCB.EC.BTIX (Arg0)
        If ((Local0 == 0xFF))
        {
            Return (Package (0x15)
            {
                One, // Add Revision field with value 1
                Zero,  // Power Unit
                0xFFFFFFFF, // Design Capacity
                0xFFFFFFFF, // Last Full Charge Capacity
                One, // Battery Technology
                0xFFFFFFFF, // Design Voltage
                Zero, // Design Capacity of Warning
                Zero, // Design Capacity of Low
                0xFFFFFFFF, // Add Cycle Count field
                100000, // Add Measurement Accuracy field with 100.000% value
                0xFFFFFFFF, // Add Max Sampling Time field
                0xFFFFFFFF, // Add Min Sampling Time field
                0xFFFFFFFF, // Add Max Averaging Interval field
                0xFFFFFFFF, // Add Min Averaging Interval field
                0xFFFFFFFF, // Battery Capacity Granularity 1
                0xFFFFFFFF, // Battery Capacity Granularity 2
                "", // Model Number
                "", // Serial Number
                "", // Battery Type
                Zero, // OEM Information
                One // Battery Swapping Capability – added in Revision 1: Zero means Non-swappable, One – Cold-swappable, 0x10 – Hot-swappable
            })
        }
        Else
        {
            Return (DerefOf (NBIX [Arg0]))
        }
    }

    Name (NBIX, Package(0x02)
    {
        Package(0x15)
        {
            0x01, // Add Revision field with value 1
            0x01,  // Power Unit
            0xFFFFFFFF, // Design Capacity
            0xFFFFFFFF, // Last Full Charge Capacity
            0x01, // Battery Technology
            0xFFFFFFFF, // Design Voltage
            0x00, // Design Capacity of Warning
            0x00, // Design Capacity of Low
            0x00, // Add Cycle Count field
            100000, // Add Measurement Accuracy field with 100.000% value
            0xFFFFFFFF, // Add Max Sampling Time field
            0xFFFFFFFF, // Add Min Sampling Time field
            0xFFFFFFFF, // Add Min Averaging Interval field
            0xFFFFFFFF, // Add Min Averaging Interval field
            0x64, // Battery Capacity Granularity 1
            0x64, // Battery Capacity Granularity 2
            "Primary", // Model Number
            "100000", // Serial Number
            "LIon", // Battery Type
            "Hewlett-Packard", // OEM Information
            One // Battery Swapping Capability – added in Revision 1: Zero means Non-swappable, One – Cold-swappable, 0x10 – Hot-swappable
        },
        /* the package for the second battery is not shown */
    })
}
```

`BTIX` is basically a copy of `BTIF` with changed field offsets according to `_BIX` definition in ACPI specification and added cycle count:
```c
Scope (\_SB.PCI0.LPCB.EC)
{
    Method (BTIX, 1, Serialized)
    {
        ShiftLeft (One, Arg0, Local7)
        BTDR (One)
        If (LEqual (BSTA (Local7), 0x0F))
        {
            Return (0xFF)
        }

        Acquire (BTMX, 0xFFFF)
        Store (NGBF, Local0)
        Release (BTMX)
        If (LEqual (And (Local0, Local7), Zero))
        {
            Return (Zero)
        }

        Store (NDBS, Index (NBST, Arg0))
        Acquire (BTMX, 0xFFFF)
        Or (NGBT, Local7, NGBT)
        Release (BTMX)
        Acquire (ECMX, 0xFFFF)
        If (ECRG)
        {
            Store (Arg0, BSEL)
            Store (B1B2 (BDC0, BDC1), Local0)
            Store (Local0, Index (DerefOf (Index (NBIX, Arg0)), 0x02)) // +1 because of Revision field
            Store (B1B2 (BFC0, BFC1), Local0)
            Store (Local0, Index (DerefOf (Index (NBIX, Arg0)), 0x03)) // +1
            Store (B1B2 (BDV0, BDV1), Index (DerefOf (Index (NBIX, Arg0)), 0x05)) // +1
            Multiply (B1B2 (BFC0, BFC1), NLB1, Local0)
            Divide (Local0, 0x64, /*Local3*/, Local4)
            Store (Local4, Index (DerefOf (Index (NBIX, Arg0)), 0x06)) // +1
            Multiply (B1B2 (BFC0, BFC1), NLO2, Local0)
            Divide (Local0, 0x64, /*Local3*/, Local4)
            Store (Local4, Index (DerefOf (Index (NBIX, Arg0)), 0x07)) // +1
            Store (B1B2 (BSN0, BSN1), Local0)
            Store (B1B2 (BDA0, BDA1), Local1)
            Store (B1B2 (BCC0, BCC1), Index (DerefOf (Index (NBIX, Arg0)), 0x08)) // Cycle Count – new field
        }

        Release (ECMX)
        Store (GBSS (Local0, Local1), Local2)
        Store (Local2, Index (DerefOf (Index (NBIX, Arg0)), 0x11)) // +7 because of Revision, Cycle Count, Measurement Accuracy, Max/Min Sampling Time, Max/Min Averaging Interval 
        Acquire (BTMX, 0xFFFF)
        And (NGBF, Not (Local7), NGBF)
        Release (BTMX)
        Return (Zero)
    }
}
```
