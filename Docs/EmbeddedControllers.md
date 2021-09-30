## SMCSuperIO Embedded Controller mapping

To use Embedded Controller sensors for any fan reading in macOS, one must inject the EC controller identifier into LPC device via `DeviceProperties` using the new `ec-device` key. First, one needs to determine the `LPC` device path, e.g. `PciRoot(0x0)/Pci(0x1F,0x0)`, (standardised on all Intel chipsets, gfxutil can do that on other hardware) and then add the `ec-device` property with the _corresponding_ string value of the device model from the list below, e.g. `Intel_EC_V1` as a string value. For a quick test, the `ssioec` boot argument can also be used.

An example device injection for the Intel NUC 8 series:
```
		<key>PciRoot(0x0)/Pci(0x1f,0x0)</key>
		<dict>
			<key>ec-device</key>
			<string>Intel_EC_V8</string>
			<key>model</key>
			<string>Intel Corporation Coffee Lake Series LPC Controller</string>
		</dict>
```

_Note_: Currently only Intel NUC hardware is supported.

#### Debugger (`debug`)

This is a special type of EC driver available in DEBUG builds of SMCSuperIO. Instead of supporting particular hardware, it lets one probe the registers of any embedded controller and analyse its memory contents. This EC driver will provide FANs with values read from a particular memory region of the EC device memory. Each FAN represents one single byte:

- The memory region size (essentially the number of FANs) is controlled via `ssiowndsz` boot argument, default is 5.
- The memory region starting byte is controlled via `ssiownd` boot argument, default is 0.

Debugger firstly tries to use MMIO mode and then falls back to PMIO mode on failure.

The PMIO mode supports up to 256 bytes of memory. MMIO mode supports up to 64 KBs of memory with the (usually 256 bytes) EC region mapped somewhere in the area.
The `debug` driver dumps first 8 KBs of EC memory to the log to help to locate the mapped EC window in the MMIO mode.

#### Generic (`generic`)

This is another special type of EC driver available in SMCSuperIO to describe simple EC devices that report FANs via EC plainly. To use it in addition to  `ec-device` property one must additionally
inject other properties describing the amount of fans, their addresses, value sizes, and encoding. Currently the following properties are supported (all 32-bit numbers):

- `fan-count` — amount of fans, defaults to `1`.
- `fan0-addr` — 0 fan address, defaults to `0`.
- `fan0-size` — 0 fan size, defaults to `1`, can be `1` or `2`.
- `fan0-big` — 0 fan endianness, defaults to `0` (little endian), can also be `1` (big endian).
- `fan0-inverse` — 0 fan is reported in inverse values (maximum value is off, minimum value is on), defaults to `0`, normal mode.
- `fan0-mul` — 0 fan speed multipler used to translate sensor values to RPM, defaults to `1`, cannot be 0.
- `fan0-div` — 0 fan speed divisor used to translate sensor values to RPM, defaults to `1`, cannot be 0.
- `fan0-dividend` — 0 fan speed dividend used to translate sensor values to RPM, unused when 0.

The effective formula is `fan0-dividend / (val * fan0-mul / fan0-div)` or `val * fan0-mul / fan0-div` when there is no dividend.

Use `fan1-addr` for the second fan and so on.

## Supported devices with respective EC Identifiers

### Intel EC V1 (`Intel_EC_V1`)

- `MKKBLY35` Firmware:<br/>
Intel® Compute Card **CD1M3128MK**<br/>

- `MKKBLi5v` Firmware:<br/>
Intel® Compute Card **CD1IV128MK**<br/>

### Intel EC V2 (`Intel_EC_V2`)

- `AYAPLCEL` Firmware:<br/>
Intel® NUC 6 Kit **NUC6CAYH**<br/>
Intel® NUC 6 Kit **NUC6CAYS**<br/>

### Intel EC V3 (`Intel_EC_V3`)

- `BNKBL357` Firmware:<br/>
Intel® NUC 7 Enthusiast Mini PC with Windows® 10 **NUC7i7BNHXG**<br/>
Intel® NUC 7 Enthusiast Mini PC with Windows® 10 **NUC7i7BNKQ**<br/>
Intel® NUC 7 Home Mini PC with Windows® 10 **NUC7i3BNHXF**<br/>
Intel® NUC 7 Home Mini PC with Windows® 10 **NUC7i5BNHXF**<br/>
Intel® NUC 7 Home Mini PC with Windows® 10 **NUC7i5BNKP**<br/>
Intel® NUC 7 Board **NUC7i3BNB**<br/>
Intel® NUC 7 Board **NUC7i5BNB**<br/>
Intel® NUC 7 Board **NUC7i7BNB**<br/>
Intel® NUC 7 Kit **NUC7i3BNH**<br/>
Intel® NUC 7 Kit **NUC7i3BNHX1 with Intel® Optane™ Memory**<br/>
Intel® NUC 7 Kit **NUC7i3BNK**<br/>
Intel® NUC 7 Kit **NUC7i5BNH**<br/>
Intel® NUC 7 Kit **NUC7i5BNHX1 with Intel® Optane™ Memory**<br/>
Intel® NUC 7 Kit **NUC7i5BNK**<br/>
Intel® NUC 7 Kit **NUC7i7BNH**<br/>
Intel® NUC 7 Kit **NUC7i7BNHX1 with Intel® Optane™ Memory**<br/>

### Intel EC V4 (`Intel_EC_V4`)

- `CYCNLi35` Firmware:<br/>
Intel® NUC 8 Home Mini PC with Windows® 10 **NUC8i3CYSM**<br/>
Intel® NUC 8 Home Mini PC with Windows® 10 **NUC8i3CYSN**<br/>

### Intel EC V5 (`Intel_EC_V5`)

- `HNKBLi70` Firmware:<br/>
Intel® NUC 8 Business Mini PC with Windows® 10 **NUC8i7HNKQC**<br/>
Intel® NUC 8 Enthusiast Mini PC with Windows® 10 **NUC8i7HVKVA**<br/>
Intel® NUC 8 Enthusiast Mini PC with Windows® 10 **NUC8i7HVKVAW**<br/>
Intel® NUC 8 Kit **NUC8i7HNK**<br/>
Intel® NUC 8 Kit **NUC8i7HVK**<br/>

### Intel EC V6 (`Intel_EC_V6`)

- `DNKBLi7v` Firmware:<br/>
Intel® NUC 7 Board **NUC7i7DNBE**<br/>
Intel® NUC 7 Kit **NUC7i7DNHE**<br/>
Intel® NUC 7 Kit **NUC7i7DNKE**<br/>

- `DNKBLi30` Firmware:<br/>
Intel® NUC 7 Business Mini PC with Windows® 10 Pro **NUC7i3DNHNC**<br/>
Intel® NUC 7 Business Mini PC with Windows® 10 Pro **NUC7i3DNKTC**<br/>
Intel® NUC 7 Board **NUC7i3DNBE**<br/>
Intel® NUC 7 Kit **NUC7i3DNHE**<br/>
Intel® NUC 7 Kit **NUC7i3DNKE**<br/>

### Intel EC V7 (`Intel_EC_V7`)

- `JYGLKCPX` Firmware:<br/>
Intel® NUC 7 Essential Mini PC with Windows® 10 **NUC7CJYSAL**<br/>
Intel® NUC 7 Kit **NUC7CJYH**<br/>
Intel® NUC 7 Kit **NUC7PJYH**<br/>

### Intel EC V8 (`Intel_EC_V8`)

- `BECFL357` Firmware:<br/>
Intel® NUC 8 Enthusiast Mini PC with Windows® 10 **NUC8i7BEHGA**<br/>
Intel® NUC 8 Enthusiast Mini PC with Windows® 10 **NUC8i7BEKQA**<br/>
Intel® NUC 8 Home Mini PC with Windows® 10 **NUC8i3BEHFA**<br/>
Intel® NUC 8 Home Mini PC with Windows® 10 **NUC8i5BEHFA**<br/>
Intel® NUC 8 Home Mini PC with Windows® 10 **NUC8i5BEKPA**<br/>
Intel® NUC 8 Kit **NUC8i3BEH**<br/>
Intel® NUC 8 Kit **NUC8i3BEHS**<br/>
Intel® NUC 8 Kit **NUC8i3BEK**<br/>
Intel® NUC 8 Kit **NUC8i5BEH**<br/>
Intel® NUC 8 Kit **NUC8i5BEHS**<br/>
Intel® NUC 8 Kit **NUC8i5BEK**<br/>
Intel® NUC Kit **NUC8i7BEH**<br/>
Intel® NUC Kit **NUC8i7BEK**<br/>

### Intel EC V9 (`Intel_EC_V9`)

- `FNCML357` Firmware:<br/>
Intel® NUC 10 Performance Kit **NUC10i3FNH**<br/>
Intel® NUC 10 Performance Kit **NUC10i3FNHF**<br/>
Intel® NUC 10 Performance Kit **NUC10i3FNK**<br/>
Intel® NUC 10 Performance Kit **NUC10i5FNH**<br/>
Intel® NUC 10 Performance Kit **NUC10i5FNHF**<br/>
Intel® NUC 10 Performance Kit **NUC10i5FNHJ**<br/>
Intel® NUC 10 Performance Kit **NUC10i5FNK**<br/>
Intel® NUC 10 Performance Kit **NUC10i5FNKP**<br/>
Intel® NUC 10 Performance Kit **NUC10i7FNH**<br/>
Intel® NUC 10 Performance Kit **NUC10i7FNHC**<br/>
Intel® NUC 10 Performance Kit **NUC10i7FNK**<br/>
Intel® NUC 10 Performance Kit **NUC10i7FNKP**<br/>
Intel® NUC 10 Performance Mini PC **NUC10i3FNHFA**<br/>
Intel® NUC 10 Performance Mini PC **NUC10i3FNHJA**<br/>
Intel® NUC 10 Performance Mini PC **NUC10i5FNHCA**<br/>
Intel® NUC 10 Performance Mini PC **NUC10i5FNHJA**<br/>
Intel® NUC 10 Performance Mini PC **NUC10i5FNKPA**<br/>
Intel® NUC 10 Performance Mini PC **NUC10i7FNHAA**<br/>
Intel® NUC 10 Performance Mini PC **NUC10i7FNHJA**<br/>
Intel® NUC 10 Performance Mini PC **NUC10i7FNKPA**<br/>

- `PNWHL357` Firmware:<br/>
Intel® NUC 8 Pro Board **NUC8i3PNB**<br/>
Intel® NUC 8 Pro Kit **NUC8i3PNH**<br/>
Intel® NUC 8 Pro Kit **NUC8i3PNK**<br/>

- `PNWHLV57` Firmware:<br/>
No model references found.<br/>

- `QNCFLX70` Firmware:<br/>
Intel® NUC 9 Pro Compute Element **NUC9V7QNB**<br/>
Intel® NUC 9 Pro Compute Element **NUC9VXQNB**<br/>
Intel® NUC 9 Pro Kit **NUC9V7QNX**<br/>
Intel® NUC 9 Pro Kit **NUC9VXQNX**<br/>

- `QXCFL579` Firmware:<br/>
Intel® NUC 9 Extreme Compute Element **NUC9i5QNB**<br/>
Intel® NUC 9 Extreme Compute Element **NUC9i7QNB**<br/>
Intel® NUC 9 Extreme Compute Element **NUC9i9QNB**<br/>
Intel® NUC 9 Extreme Kit **NUC9i5QNX**<br/>
Intel® NUC 9 Extreme Kit **NUC9i7QNX**<br/>
Intel® NUC 9 Extreme Kit **NUC9i9QNX**<br/>

- `CHAPLCEL` Firmware:<br/>
Intel® NUC 8 Rugged Kit **NUC8CCHKR**<br/>
Intel® NUC 8 Board **NUC8CCHB**<br/>

- `CBWHL357` Firmware:<br/>
Intel® NUC 8 Compute Element **CM8CCB**<br/>
Intel® NUC 8 Compute Element **CM8i3CB**<br/>
Intel® NUC 8 Compute Element **CM8i5CB**<br/>
Intel® NUC 8 Compute Element **CM8i7CB**<br/>
Intel® NUC 8 Compute Element **CM8PCB**<br/>

- `CBWHLMIV` Firmware:<br/>
Intel® NUC 8 Compute Element **CM8v5CB**<br/>
Intel® NUC 8 Compute Element **CM8v7CB**<br/>

- `EBTGL357` Firmware:<br/>
Intel® NUC 11 Compute Element **CM11EBC4W**<br/>
Intel® NUC 11 Compute Element **CM11EBi38W**<br/>
Intel® NUC 11 Compute Element **CM11EBi58W**<br/>
Intel® NUC 11 Compute Element **CM11EBi716W**<br/>

- `EBTGLMIV` Firmware:<br/>
Intel® NUC 11 Compute Element **CM11EBv58W**<br/>
Intel® NUC 11 Compute Element **CM11EBv716W**<br/>

### Intel EC VA (`Intel_EC_VA`)

- `PATGL357` Firmware:<br/>
Intel® NUC 11 Performance Kit **NUC11PAHi3**<br/>
Intel® NUC 11 Performance Kit **NUC11PAHi5**<br/>
Intel® NUC 11 Performance Kit **NUC11PAHi7**<br/>
Intel® NUC 11 Performance Kit **NUC11PAKi3**<br/>
Intel® NUC 11 Performance Kit **NUC11PAKi5**<br/>
Intel® NUC 11 Performance Kit **NUC11PAKi7**<br/>
Intel® NUC 11 Performance Mini PC **NUC11PAQi50WA**<br/>
Intel® NUC 11 Performance Mini PC **NUC11PAQi70QA**<br/>

- `TNTGLV57` Firmware:<br/>
Intel® NUC 11 Pro Board **NUC11TNBv5**<br/>
Intel® NUC 11 Pro Board **NUC11TNBv7**<br/>
Intel® NUC 11 Pro Kit **NUC11TNHv5**<br/>
Intel® NUC 11 Pro Kit **NUC11TNHv50L**<br/>
Intel® NUC 11 Pro Kit **NUC11TNHv7**<br/>
Intel® NUC 11 Pro Kit **NUC11TNHv70L**<br/>
Intel® NUC 11 Pro Kit **NUC11TNKv5**<br/>
Intel® NUC 11 Pro Kit **NUC11TNKv7**<br/>
Intel® NUC 11 Pro Mini PC **NUC11TNKv5**<br/>
Intel® NUC 11 Pro Mini PC **NUC11TNKv7**<br/>

- `TNTGL357` Firmware:<br/>
Intel® NUC 11 Pro Board **NUC11TNBi3**<br/>
Intel® NUC 11 Pro Board **NUC11TNBi5**<br/>
Intel® NUC 11 Pro Board **NUC11TNBi7**<br/>
Intel® NUC 11 Pro Kit **NUC11TNHi3**<br/>
Intel® NUC 11 Pro Kit **NUC11TNHi30L**<br/>
Intel® NUC 11 Pro Kit **NUC11TNHi30P**<br/>
Intel® NUC 11 Pro Kit **NUC11TNHi5**<br/>
Intel® NUC 11 Pro Kit **NUC11TNHi50L**<br/>
Intel® NUC 11 Pro Kit **NUC11TNHi50W**<br/>
Intel® NUC 11 Pro Kit **NUC11TNHi7**<br/>
Intel® NUC 11 Pro Kit **NUC11TNHi70L**<br/>
Intel® NUC 11 Pro Kit **NUC11TNHi70Q**<br/>
Intel® NUC 11 Pro Kit **NUC11TNKi3**<br/>
Intel® NUC 11 Pro Kit **NUC11TNKi5**<br/>
Intel® NUC 11 Pro Kit **NUC11TNKi7**<br/>

- `PHTGL579` Firmware:<br/>
Intel® NUC 11 Enthusiast Kit **NUC11PHKi7C**<br/>
Intel® NUC 11 Enthusiast Mini PC **NUC11PHKi7CAA**<br/>

### Intel EC VB (`Intel_EC_VB`)

- `INWHL357` Firmware:<br/>
Intel® NUC 8 Mainstream-G Kit **NUC8i5INH**<br/>
Intel® NUC 8 Mainstream-G Kit **NUC8i7INH**<br/>
Intel® NUC 8 Mainstream-G Mini PC **NUC8i5INH**<br/>
Intel® NUC 8 Mainstream-G Mini PC **NUC8i7INH**<br/>

### HP Pavilion 14 CE2072NL (`generic`)

- `ec-device` = `"generic"`
- `fan-count` = `1`
- `fan0-addr` = `0xC3`
- `fan0-size` = `2`
- `fan0-big` = `0`

<details>
<summary>Spoiler: EC RAM details</summary>

```ASL
OperationRegion (ERAM, EmbeddedControl, Zero, 0xFF)
Field (ERAM, ByteAcc, NoLock, Preserve)
{
    SMPR,   8,         /* 00h */
    SMST,   8,         /* 01h */
    SMAD,   8,         /* 02h */
    SMCM,   8,         /* 03h */
    SMD0,   256,       /* 04h */
    BCNT,   8,         /* 24h */
    SMAA,   8,         /* 25h */
    Offset (0x40),
    SW2S,   1,         /* 40h */
        ,   2,
    ACCC,   1,
    TRPM,   1,
    Offset (0x41),
    W7OS,   1,         /* 41h */
    QWOS,   1,
        ,   1,
    SUSE,   1,
    RFLG,   1,
        ,   1,
        ,   1,
    Offset (0x42),
        ,   5,         /* 42h */
    UBOS,   1,
    Offset (0x43),
        ,   1,         /* 43h */
        ,   1,
    ACPS,   1,
    ACKY,   1,
    GFXT,   1,
        ,   1,
        ,   1,
    Offset (0x44),
        ,   7,         /* 44h */
    DSMB,   1,
    GMSE,   1,         /* 45h */
        ,   1,
    QUAD,   1,
    Offset (0x46),
    Offset (0x47),
    ADC4,   8,         /* 47h */
    ADC5,   8,         /* 48h */
    Offset (0x4C),
    STRM,   8,         /* 4Ch */
    Offset (0x4E),
    LIDE,   1,         /* 4Eh */
    Offset (0x50),
        ,   5,         /* 50h */
    DPTL,   1,
        ,   1,
    DPTE,   1,
    Offset (0x52),
    ECLS,   1,         /* 52h */
    Offset (0x55),
    EC45,   8,         /* 55h */
    Offset (0x58),
    RTMP,   8,         /* 58h */
    ADC6,   8,         /* 59h */
    Offset (0x5E),
    TMIC,   8,         /* 5Eh */
    Offset (0x61),
    SHPM,   8,         /* 61h */
    ECTH,   8,         /* 62h */
    ECTL,   8,         /* 63h */
    Offset (0x67),
    LDDG,   1,         /* 67h */
        ,   1,
    GC6R,   1,
    IGC6,   1,
    Offset (0x68),
        ,   3,         /* 68h */
    PLGS,   1,
    Offset (0x69),
        ,   6,         /* 69h */
    BTVD,   1,
    Offset (0x6C),
    GWKR,   8,         /* 6Ch */
    Offset (0x70),
    BADC,   16,        /* 70h */
    BFCC,   16,        /* 72h */
    BVLB,   8,         /* 74h */
    BVHB,   8,         /* 75h */
    BDVO,   8,         /* 76h */
    Offset (0x7F),
    ECTB,   1,         /* 7Fh */
    Offset (0x82),
    MBST,   8,         /* 82h */
    MCUR,   16,        /* 83h */
    MBRM,   16,        /* 85h */
    MBCV,   16,        /* 87h */
    VGAV,   8,         /* 89h */
    FGM2,   8,         /* 8Ah */
    FGM3,   8,         /* 8Bh */
    Offset (0x8D),
        ,   5,         /* 8Dh */
    MBFC,   1,
    Offset (0x92),
    Offset (0x93),
    Offset (0x94),
    GSSU,   1,         /* 94h */
    GSMS,   1,
    Offset (0x95),
    MMST,   4,         /* 95h */
    DMST,   4,
    Offset (0xA0),
    QBHK,   8,         /* A0h */
    Offset (0xA2),
    QBBB,   8,         /* A2h */
    Offset (0xA4),
    MBTS,   1,         /* A4h */
    MBTF,   1,
        ,   4,
    AD47,   1,
    BACR,   1,
    MBTC,   1,         /* A5h */
        ,   2,
    MBNH,   1,
    Offset (0xA6),
    MBDC,   8,         /* A6h */
    Offset (0xA8),
    EWDT,   1,         /* A8h */
    CWDT,   1,
    LWDT,   1,
    AWDT,   1,
    Offset (0xAA),
        ,   1,         /* AAh */
    SMSZ,   1,
        ,   5,
    RCDS,   1,
    Offset (0xAD),
    SADP,   8,         /* ADh */
    Offset (0xB2),
    RPM1,   8,         /* B2h <- actually not FAN RPM despite the name */
    RPM2,   8,         /* B3h <- actually not FAN RPM despite the name */
    Offset (0xB7),
    Offset (0xB8),
    Offset (0xBA),
    Offset (0xBB),
    Offset (0xBC),
    Offset (0xC1),
    DPPC,   8,         /* C1h */
    Offset (0xC8),
        ,   1,         /* C8h */
    CVTS,   1,
    Offset (0xC9),
    TPVN,   8,         /* C9h */
    Offset (0xCE),
    NVDX,   8,         /* CEh */
    ECDX,   8,         /* CFh */
    EBPL,   1,         /* D0h */
    Offset (0xD2),
        ,   7,         /* D2h */
    DLYE,   1,
    Offset (0xD4),
    PSHD,   8,         /* D4h */
    PSLD,   8,         /* D5h */
    DBPL,   8,         /* D6h */
    STSP,   8,         /* D7h */
    Offset (0xDA),
    PSIN,   8,         /* DAh */
    PSKB,   1,         /* DBh */
    PSTP,   1,
        ,   1,
    PWOL,   1,
    RTCE,   1,
    Offset (0xE0),
    DLYT,   8,         /* E0h */
    DLY2,   8,         /* E1h */
    Offset (0xE5),
    GP12,   8,         /* E5h */
    SFHK,   8,         /* E6h */
    Offset (0xE9),
    DTMT,   8,         /* E9h */
    PL12,   8,         /* EAh */
    ETMT,   8,         /* EBh */
    Offset (0xF2),
    ZPOD,   1,         /* F2h */
        ,   4,
    WLPW,   1,
    WLPS,   1,
    ENPA,   1,
    Offset (0xF4),
    SFAN,   8,         /* F4h */
    Offset (0xF8),
    BAAE,   1,         /* F8h */
    S3WA,   1,
    BNAC,   1,
        ,   1,
    EFS3,   1,
    S3WK,   1,
    RSAL,   1
}
```

```
(DBG) 0000: 00 80 50 24 02 49 4F 4E 00 00 00 00 00 00 00 00
(DBG) 0010: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
(DBG) 0020: 00 00 00 00 0B 00 00 00 03 00 00 00 00 00 00 00
(DBG) 0030: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
(DBG) 0040: 02 01 04 0C 04 44 00 2C 00 00 00 00 00 00 00 10
(DBG) 0050: 00 00 00 0F 00 05 00 00 43 2E 00 00 00 00 00 08
(DBG) 0060: 00 00 00 00 00 00 00 00 08 40 00 0C 00 0E 0E 00
(DBG) 0070: 1F 0E 07 0C 1E 2D 80 00 00 00 00 00 00 00 00 08
(DBG) 0080: 00 00 01 51 F7 81 07 AD 2B 00 00 00 00 C0 00 00
(DBG) 0090: 00 00 07 0C 05 00 3F E2 09 90 33 00 00 FE 0B 03
(DBG) 00A0: 03 80 00 10 01 00 00 48 00 15 00 00 00 04 1E 00
(DBG) 00B0: 00 00 00 00 00 3F 0E 00 00 00 00 07 3F 3D 67 61
(DBG) 00C0: 03 00 00 27 0B 00 00 00 00 47 00 00 00 00 D1 00
(DBG) 00D0: 3F 00 00 00 32 06 00 01 3F 00 1D 00 00 00 00 00
(DBG) 00E0: 00 00 50 50 42 00 00 00 00 5F 56 00 00 AA 00 00
(DBG) 00F0: 08 00 A2 00 00 00 00 08 00 00 31 35 33 32 30 30
```
</details>

_Note_: Found and tested by @1alessandro1. Only one fan header exists, so both fans will be reported in sync as a single fan as shown [here](https://github.com/acidanthera/bugtracker/issues/395#issuecomment-808756416).

### HP Pavilion 15-cb073tx (`generic`)

- `ec-device` = `"generic"`
- `fan-count` = `2`
- `fan0-addr` = `0xB2`
- `fan0-size` = `2`
- `fan0-big` = `0`
- `fan1-addr` = `0xC3`
- `fan1-size` = `2`
- `fan1-big` = `0`

_Note_: Found and tested by @zty199, with @1alessandro1's help. EC RAM details are exactly the same as HP Pavilion 14 CE2072NL, but one more fan connector exists on the motherboard.

### HP ZBook 17 G5 (`generic`)

- `ec-device` = `"generic"`
- `fan-count` = `2`
- `fan0-addr` = `0x2E`
- `fan0-size` = `1`
- `fan0-big` = `0`
- `fan0-inverse` = `1`
- `fan0-mul` = `BE`
- `fan0-div` = `11`
- `fan1-addr` = `0x35`
- `fan1-size` = `1`
- `fan1-big` = `0`
- `fan1-inverse` = `1`
- `fan1-mul` = `BE`
- `fan1-div` = `11`

### HP Probook 430 G3

- `ec-device` = `generic`
- `fan-count` = `1`
- `fan0-addr` = `0x2E`
- `fan0-big` = `0`
- `fan0-div` = `11`
- `fan0-inverse` = `1`
- `fan0-mul` = `BE`
- `fan0-size` = `1`

### HP OMEN Laptop 15-ek0xxx (`generic`)

- `ec-device` = `"generic"`
- `fan-count` = `2`
- `fan0-addr` = `0xB0`
- `fan0-size` = `2`
- `fan0-big` = `0`
- `fan1-addr` = `0xB2`
- `fan1-size` = `2`
- `fan1-big` = `0`

<details>
<summary>Spoiler: EC RAM details</summary>

```ASL
OperationRegion (ERAM, EmbeddedControl, Zero, 0xFF)
Field (ERAM, ByteAcc, NoLock, Preserve)
    {
        SMPR,   8, 
        SMST,   8, 
        SMAD,   8, 
        SMCM,   8, 
        SMD0,   256, 
        BCNT,   8, 
        SMAA,   8, 
        Offset (0x30), 
        BTPL,   8, 
        BTPH,   8, 
        BCLC,   8, 
        Offset (0x34), 
        SRP1,   8, 
        SRP2,   8, 
        Offset (0x40), 
        SW2S,   1, 
            ,   2, 
        ACCC,   1, 
        TRPM,   1, 
        Offset (0x41), 
        W7OS,   1, 
        QWOS,   1, 
            ,   1, 
        SUSE,   1, 
        RFLG,   1, 
        Offset (0x42), 
            ,   4, 
        KBBL,   1, 
        Offset (0x43), 
            ,   1, 
            ,   1, 
        ACPS,   1, 
        ACKY,   1, 
        GFXT,   1, 
        Offset (0x44), 
            ,   7, 
        DSMB,   1, 
        Offset (0x47), 
        TNT2,   8, 
        TNT3,   8, 
        TNT4,   8, 
        IRSN,   8, 
        TNT5,   8, 
        STRM,   8, 
        Offset (0x4E), 
        LIDE,   1, 
        Offset (0x4F), 
        Offset (0x50), 
            ,   2, 
        PTHM,   1, 
            ,   1, 
        BSEV,   1, 
        DPTL,   1, 
        IHEF,   1, 
        Offset (0x52), 
        ECLS,   1, 
        Offset (0x55), 
        EC45,   8, 
        TPL4,   8, 
        Offset (0x58), 
        RTMP,   8, 
        TNT1,   8, 
        Offset (0x5B), 
        HPTC,   8, 
        Offset (0x5F), 
            ,   1, 
        Offset (0x61), 
        SHPM,   8, 
        OMCC,   1, 
        Offset (0x67), 
        LDBG,   1, 
            ,   1, 
        GC6R,   1, 
        IGC6,   1, 
            ,   3, 
        HDNK,   1, 
            ,   3, 
        PLGS,   1, 
        Offset (0x69), 
            ,   4, 
        BCTF,   1, 
        BMNF,   1, 
        BTVD,   1, 
        BF10,   1, 
        Offset (0x6C), 
        GWKR,   8, 
        Offset (0x70), 
        BADC,   16, 
        BFCC,   16, 
        BVLB,   8, 
        BVHB,   8, 
        BDVO,   8, 
        Offset (0x7F), 
        ECTB,   1, 
        Offset (0x82), 
        MBST,   8, 
        MCUR,   16, 
        MBRM,   16, 
        MBCV,   16, 
        GPUT,   8, 
        Offset (0x8B), 
        LEDM,   3, 
        Offset (0x8D), 
            ,   5, 
        MBFC,   1, 
        Offset (0x90), 
        NVDO,   8, 
        ECDO,   8, 
        Offset (0x94), 
        GSSU,   1, 
        GSMS,   1, 
        Offset (0x95), 
        HPCM,   8, 
        Offset (0xA0), 
        QBHK,   8, 
        Offset (0xA2), 
        QBBB,   8, 
        Offset (0xA4), 
        MBTS,   1, 
            ,   6, 
        BACR,   1, 
        Offset (0xA6), 
        MBDC,   8, 
        Offset (0xA8), 
        ENWD,   1, 
        TMPR,   1, 
        Offset (0xAA), 
            ,   1, 
        SMSZ,   1, 
        SE1N,   1, 
        SE2N,   1, 
        SOIE,   1, 
            ,   2, 
        RCDS,   1, 
        Offset (0xAD), 
        SADP,   8, 
        Offset (0xB0), 
        RPM1,   8,      /* 0xB0, FAN0 RPM Address */
        RPM2,   8, 
        RPM3,   8,      /* 0xB2, FAN1 RPM Address */
        RPM4,   8, 
        Offset (0xB7), 
        DGTP,   8, 
        Offset (0xBA), 
        CLOW,   8, 
        CMAX,   8, 
        Offset (0xC1), 
        DPPC,   8, 
        Offset (0xC5), 
        SHB1,   1, 
        SHB2,   1, 
        SHB3,   1, 
        SHB4,   1, 
        SHOK,   1, 
        SHFL,   1, 
        SHNP,   1, 
        SHEN,   1, 
            ,   1, 
        CVTS,   1, 
        Offset (0xD0), 
        EBPL,   1, 
        Offset (0xD2), 
        S1A1,   8, 
        S2A1,   8, 
        PSHD,   8, 
        PSLD,   8, 
        DBPL,   8, 
        STSP,   8, 
        Offset (0xDA), 
        PSIN,   8, 
        PSKB,   1, 
        PSTP,   1, 
            ,   1, 
        PWOL,   1, 
        RTCE,   1, 
        Offset (0xDC), 
        S1A0,   8, 
        S2A0,   8, 
        NVDX,   8, 
        ECDX,   8, 
        DLYT,   8, 
        DLY2,   8, 
        KBT0,   8, 
        Offset (0xE6), 
        SFHK,   8, 
        Offset (0xE9), 
        DTMT,   8, 
        PL12,   8, 
        ETMT,   8, 
        COLM,   1, 
        FFFF,   1, 
        FFFS,   1, 
        Offset (0xF2), 
        ZPDD,   1, 
            ,   6, 
        ENPA,   1, 
        Offset (0xF4), 
        SFAN,   8, 
        Offset (0xF8), 
        NVDS,   1, 
        NVPM,   1, 
        Offset (0xF9), 
            ,   7, 
        FTHM,   1
    }
```

```
Method (GM2D, 0, NotSerialized)
    {
        Debug = "HP WMI Command type 0x2D for WMI 20008h command"
        WSMI (0x00020008, 0x2D, Zero, 0x80, Zero)
        Local0 = Package (0x03)
            {
                Zero, 
                0x80, 
                Buffer (0x80)
                {
                        0x00                                             // .
                }
            }
        If ((^^PCI0.LPCB.EC0.ECOK == One))
        {
            Local1 = ^^PCI0.LPCB.EC0.RPM2 /* \_SB_.PCI0.LPCB.EC0_.RPM2 */
            Local1 <<= 0x08
            Local1 += ^^PCI0.LPCB.EC0.RPM1 /* \_SB_.PCI0.LPCB.EC0_.RPM1 */
            Divide (Local1, 0x64, Local2, Local3)
            DerefOf (Local0 [0x02]) [Zero] = Local3
            Local1 = ^^PCI0.LPCB.EC0.RPM4 /* \_SB_.PCI0.LPCB.EC0_.RPM4 */
            Local1 <<= 0x08
            Local1 += ^^PCI0.LPCB.EC0.RPM3 /* \_SB_.PCI0.LPCB.EC0_.RPM3 */
            Divide (Local1, 0x64, Local2, Local3)
            DerefOf (Local0 [0x02]) [One] = Local3
        }

        Return (Local0)
    }
```

```
Method (FRSP, 0, NotSerialized)
    {
        Local2 = Zero
        If ((\_SB.PCI0.LPCB.EC0.ECOK == One))
        {
            Local0 = \_SB.PCI0.LPCB.EC0.RPM1
            Local1 = \_SB.PCI0.LPCB.EC0.RPM2
            Local1 <<= 0x08
            Local0 |= Local1
            If ((Local0 != Zero))
            {
                Divide (0x00075300, Local0, Local0, Local2)
            }
        }

        Return (Local2)
    }
```

</details>

### Hasee Z7M-KP7S1 (`generic`)

- `ec-device` = `"generic"`
- `fan-count` = `2`
- `fan0-addr` = `0xD0`
- `fan0-size` = `2`
- `fan0-big` = `1`
- `fan0-dividend` = `0x20E6BC`
- `fan1-addr` = `0xD2`
- `fan1-size` = `2`
- `fan1-big` = `1`
- `fan1-dividend` = `0x20E6BC`

<details>
<summary>Spoiler: EC RAM details</summary>

```ASL
OperationRegion (RAM, SystemMemory, 0xFF700100, 0x0100)
Field (RAM, ByteAcc, Lock, Preserve)
{
    NMSG,   8, 
    SLED,   4, 
    Offset (0x02), 
    MODE,   1, 
    FAN0,   1, 
    TME0,   1, 
    TME1,   1, 
    FAN1,   1, 
        ,   2, 
    Offset (0x03), 
    LSTE,   1, 
    LSW0,   1, 
    LWKE,   1, 
    WAKF,   1, 
        ,   2, 
    PWKE,   1, 
    MWKE,   1, 
    AC0,    8, 
    PSV,    8, 
    CRT,    8, 
    TMP,    8, 
    AC1,    8, 
    BBST,   8, 
    Offset (0x0B), 
    Offset (0x0C), 
    Offset (0x0D), 
    Offset (0x0E), 
    SLPT,   8, 
    SWEJ,   1, 
    SWCH,   1, 
    Offset (0x10), 
    ADP,    1, 
    AFLT,   1, 
    BAT0,   1, 
    BAT1,   1, 
        ,   3, 
    PWOF,   1, 
    WFNO,   8, 
    BPU0,   32, 
    BDC0,   32, 
    BFC0,   32, 
    BTC0,   32, 
    BDV0,   32, 
    BST0,   32, 
    BPR0,   32, 
    BRC0,   32, 
    BPV0,   32, 
    BTP0,   16, 
    BRS0,   16, 
    BCW0,   32, 
    BCL0,   32, 
    BCG0,   32, 
    BG20,   32, 
    BMO0,   64, 
    BIF0,   64, 
    BSN0,   32, 
    BTY0,   64, 
    Offset (0x67), 
    Offset (0x68), 
    ECOS,   8, 
    LNXD,   8, 
    ECPS,   8, 
    Offset (0x6C), 
    BTMP,   16, 
    EVTN,   8, 
    Offset (0x72), 
    PRCL,   8, 
    PRC0,   8, 
    PRC1,   8, 
    PRCM,   8, 
    PRIN,   8, 
    PSTE,   8, 
    PCAD,   8, 
    PEWL,   8, 
    PWRL,   8, 
    PECD,   8, 
    PEHI,   8, 
    PECI,   8, 
    PEPL,   8, 
    PEPM,   8, 
    PWFC,   8, 
    PECC,   8, 
    PDT0,   8, 
    PDT1,   8, 
    PDT2,   8, 
    PDT3,   8, 
    PRFC,   8, 
    PRS0,   8, 
    PRS1,   8, 
    PRS2,   8, 
    PRS3,   8, 
    PRS4,   8, 
    PRCS,   8, 
    PEC0,   8, 
    PEC1,   8, 
    PEC2,   8, 
    PEC3,   8, 
    CMDR,   8, 
    CVRT,   8, 
    GTVR,   8, 
    FANT,   8, 
    SKNT,   8, 
    AMBT,   8, 
    MCRT,   8, 
    DIM0,   8, 
    DIM1,   8, 
    PMAX,   8, 
    PPDT,   8, 
    PECH,   8, 
    PMDT,   8, 
    TSD0,   8, 
    TSD1,   8, 
    TSD2,   8, 
    TSD3,   8, 
    CPUP,   16, 
    MCHP,   16, 
    SYSP,   16, 
    CPAP,   16, 
    MCAP,   16, 
    SYAP,   16, 
    CFSP,   16, 
    CPUE,   16, 
    Offset (0xC6), 
    Offset (0xC7), 
    VGAT,   8, 
    OEM1,   8, 
    OEM2,   8, 
    OEM3,   16, 
    OEM4,   8, 
    Offset (0xCE), 
    DUT1,   8,      /* 0xCE */
    DUT2,   8,      /* 0xCF */
    RPM1,   16,     /* 0xD0 CPU FAN RPM Address */
    RPM2,   16,     /* 0xD2 GPU FAN RPM Address */
    RPM4,   16, 
    Offset (0xD7), 
    DTHL,   8, 
    DTBP,   8, 
    AIRP,   8, 
    WINF,   8, 
    RINF,   8, 
    Offset (0xDD), 
    INF2,   8, 
    MUTE,   1, 
    Offset (0xE0), 
    RPM3,   16, 
    ECKS,   8, 
    Offset (0xE4), 
        ,   4, 
    XTUF,   1, 
    EP12,   1, 
    Offset (0xE5), 
    INF3,   8, 
    Offset (0xE7), 
    XFAN,   8, 
    Offset (0xF0), 
    PL1T,   16, 
    PL2T,   16, 
    TAUT,   8
}
```
</details>

### ASUS K550JX (`generic`)

- `ec-device` = `"generic"`
- `fan-count` = `1`
- `fan0-addr` = `0x93`
- `fan0-big` = `0`
- `fan0-div` = `1`
- `fan0-dividend` = `0x41CDB4`
- `fan0-inverse` = `0`
- `fan0-mul` = `2`
- `fan0-size` = `2`

<details>
<summary>Spoiler: EC RAM details</summary>

```ASL
OperationRegion (ECOR, EmbeddedControl, Zero, 0xFF)
Field (ECOR, ByteAcc, Lock, Preserve)
{
    Offset (0x04), 
    CMD1,   8, 
    CDT1,   8, 
    CDT2,   8, 
    CDT3,   8, 
    Offset (0x80), 
    Offset (0x81), 
    Offset (0x82), 
    Offset (0x83), 
    EB0R,   8, 
    EB1R,   8, 
    EPWF,   8, 
    Offset (0x87), 
    Offset (0x88), 
    Offset (0x89), 
    Offset (0x8A), 
    HKEN,   1, 
    Offset (0x93), 
    TAH0,   16,      /* 0x93, FAN0 RPM Address */
    TAH1,   16, 
    TSTP,   8, 
    Offset (0x9C), 
    CDT4,   8, 
    CDT5,   8, 
    Offset (0xA0), 
    Offset (0xA1), 
    Offset (0xA2), 
    Offset (0xA3), 
    EACT,   8, 
    TH1R,   8, 
    TH1L,   8, 
    TH0R,   8, 
    TH0L,   8, 
    Offset (0xB0), 
    B0PN,   16, 
    Offset (0xB4), 
    Offset (0xB6), 
    Offset (0xB8), 
    Offset (0xBA), 
    Offset (0xBC), 
    Offset (0xBE), 
    B0TM,   16, 
    B0C1,   16, 
    B0C2,   16, 
    B0C3,   16, 
    B0C4,   16, 
    Offset (0xD0), 
    B1PN,   16, 
    Offset (0xD4), 
    Offset (0xD6), 
    Offset (0xD8), 
    Offset (0xDA), 
    Offset (0xDC), 
    Offset (0xDE), 
    B1TM,   16, 
    B1C1,   16, 
    B1C2,   16, 
    B1C3,   16, 
    B1C4,   16, 
    Offset (0xF0), 
    Offset (0xF2), 
    Offset (0xF4), 
    B0SN,   16, 
    Offset (0xF8), 
    Offset (0xFA), 
    Offset (0xFC), 
    B1SN,   16
}
```
</details>

<hr>

(List compiled by @MacKonsti proudly assisting @vit9696)

### Lenovo G480 

- `ec-device` = `generic`
- `fan-count` = `1`
- `fan0-addr` = `0x95`
- `fan0-big` = `1`
- `fan0-div` = `11`
- `fan0-inverse` = `1`
- `fan0-mul` = `F0`
- `fan0-size` = `1`

 <details>
 <summary>Spoiler: EC RAM details</summary>

 ```ASL
 OperationRegion (ECMB, SystemMemory, 0xFE802000, 0x0200)
             OperationRegion (RAM, EmbeddedControl, Zero, 0xFF)
             Field (RAM, ByteAcc, Lock, Preserve)
             {
                 Offset (0x0A), 
                     ,   1, 
                 BLNK,   1, 
                 Offset (0x0B), 
                 Offset (0x10), 
                     ,   1, 
                 KTEE,   1, 
                 Offset (0x11), 
                 KPPS,   1, 
                 Offset (0x13), 
                 URTB,   8, 
                 Offset (0x4E), 
                 SCID,   8, 
                 Offset (0x53), 
                 PNID,   8, 
                 Offset (0x5C), 
                 OSTP,   8, 
                 Offset (0x72), 
                     ,   2, 
                 KEYW,   1, 
                 RTCW,   1, 
                 LIDW,   1, 
                 BL2W,   1, 
                 TPDW,   1, 
                 Offset (0x76), 
                 SYSC,   4, 
                 SYSO,   4, 
                 Offset (0x90), 
                 SCPM,   1, 
                 Offset (0x91), 
                 TTID,   8, 
                 KTAF,   8
             }
             Field (RAM, ByteAcc, Lock, Preserve)
             {
                 Offset (0x7F), 
                 BNEN,   1, 
                 BNCM,   1, 
                 BNDM,   1, 
                 BNVE,   1, 
                 Offset (0x87), 
                 BNVA,   8
             }
             Method (RDEC, 1, Serialized)
             {
                 If (ECON)
                 {
                     OperationRegion (ECRM, EmbeddedControl, Arg0, One)
                     Field (ECRM, ByteAcc, Lock, Preserve)
                     {
                         ECRB,   8
                     }
                     Return (ECRB) /* \_SB_.PCI0.LPCB.EC__.RDEC.ECRB */
                 }
                 Else
                 {
                     Return (RBEC (Arg0))
                 }
             }
             Field (RAM, ByteAcc, Lock, Preserve)
             {
                 Offset (0x92), 
                 KCSS,   1, 
                 KCTT,   1, 
                 KDTT,   1, 
                 KOSD,   1, 
                 KVTP,   1
             }
             Field (RAM, ByteAcc, Lock, Preserve)
             {
                 Offset (0x01), 
                 TIID,   8, 
                 Offset (0x17), 
                 SMCS,   8, 
                 SMPR,   8, 
                 SMST,   8, 
                 SMAR,   8, 
                 SMCM,   8, 
                 SD00,   8, 
                 SD01,   8, 
                 SD02,   8, 
                 SD03,   8, 
                 SD04,   8, 
                 SD05,   8, 
                 SD06,   8, 
                 SD07,   8, 
                 SD08,   8, 
                 SD09,   8, 
                 SD10,   8, 
                 SD11,   8, 
                 SD12,   8, 
                 SD13,   8, 
                 SD14,   8, 
                 SD15,   8, 
                 SD16,   8, 
                 SD17,   8, 
                 SD18,   8, 
                 SD19,   8, 
                 SD20,   8, 
                 SD21,   8, 
                 SD22,   8, 
                 SD23,   8, 
                 SD24,   8, 
                 SD25,   8, 
                 SD26,   8, 
                 SD27,   8, 
                 SD28,   8, 
                 SD29,   8, 
                 SD30,   8, 
                 SD31,   8, 
                 SMBC,   8, 
                 Offset (0x50), 
                 LBBM,   1, 
                 BNBM,   1, 
                 CSBM,   1, 
                 OPBM,   1, 
                 ROBM,   1, 
                 Offset (0x51), 
                 DCTL,   8, 
                 GWSS,   1, 
                 GWHC,   1, 
                 HDPR,   1, 
                 DGPU,   1, 
                 TVEC,   1, 
                     ,   2, 
                 ASPL,   1, 
                 Offset (0x54), 
                 CAMC,   1, 
                 OTBP,   1, 
                     ,   1, 
                 GFXL,   1, 
                 OPEH,   1, 
                 OPSE,   1, 
                 Offset (0x55), 
                 CBST,   8, 
                 Offset (0x57), 
                     ,   1, 
                 SMBM,   1, 
                     ,   1, 
                 RSBM,   1, 
                 Offset (0x58), 
                 LSEN,   8, 
                 Offset (0x61), 
                 MBNG,   1, 
                 SBNG,   1, 
                 Offset (0x62), 
                 BLTM,   8, 
                 ODPS,   8, 
                 Offset (0x65), 
                 SVCU,   1, 
                     ,   1, 
                 AMCB,   1, 
                 Offset (0x66), 
                 ZPOF,   1, 
                 Offset (0x68), 
                 BFUR,   1, 
                 BAAU,   1, 
                 BFUP,   1, 
                 BFUF,   1, 
                 BFUS,   1, 
                 Offset (0x69), 
                 DUST,   8, 
                 Offset (0x71), 
                 WLEN,   1, 
                 BTEN,   1, 
                     ,   1, 
                 MUTE,   1, 
                 KBID,   3, 
                 USBP,   1, 
                 Offset (0x73), 
                 WWAN,   1, 
                 Offset (0x74), 
                 CRLW,   1, 
                 PS2K,   1, 
                 PS2M,   1, 
                 TPEN,   1, 
                 CHGE,   1, 
                 INTK,   1, 
                 Offset (0x75), 
                 SWBL,   1, 
                     ,   1, 
                     ,   1, 
                     ,   1, 
                     ,   1, 
                     ,   1, 
                 BLST,   1, 
                 Offset (0x76), 
                 Offset (0x82), 
                 BMAC,   4, 
                 BMDC,   4, 
                 BNAC,   4, 
                 BNDC,   4, 
                 Offset (0x95), 
                 FANS,   8, 
                 Offset (0xBA), 
                 ICPU,   8, 
                 Offset (0xD0), 
                 TMH0,   8, 
                 Offset (0xD2), 
                 TMH1,   8, 
                 Offset (0xD4), 
                 TMH2,   8, 
                 Offset (0xD6), 
                 TMH3,   8, 
                 Offset (0xD8), 
                 TMH4,   8, 
                 Offset (0xDA), 
                 TMH5,   8, 
                 Offset (0xDC), 
                 TMH6,   8, 
                 TMH7,   8
             }
  
 </details>
 
 _Note_: Found and tested by @mjs520
