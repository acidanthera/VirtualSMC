VirtualSMC
========

[![Build Status](https://github.com/acidanthera/VirtualSMC/workflows/CI/badge.svg?branch=master)](https://github.com/acidanthera/VirtualSMC/actions) [![Scan Status](https://scan.coverity.com/projects/16571/badge.svg?flat=1)](https://scan.coverity.com/projects/16571)

Advanced Apple SMC emulator in the kernel. Requires [Lilu](https://github.com/vit9696/Lilu) for full functioning.

English (Current)  
[简体中文](README_zh.md)  

#### Features
- Supports macOS 10.4 and newer (10.9 and newer is recommended)
- Implements MMIO protocol and interrupt-based responses for compatibility with modern OS
- Properly reports key attributes and r/w protection in the keys
- Allows tuning on per-model basis and allows to use different SMC generations
- Extensible by the plugins for sensor and key addition support
- Enables `smcdebug=XX` boot argument support on 10.9
- Replaces hardware SMC it finds (to disable SMC entirely you need to flash a dedicated firmware)

#### Boot arguments
- Add `-vsmcdbg` to enable debug printing (available in DEBUG binaries).
- Add `-vsmcoff` to switch off all the Lilu enhancements.
- Add `-vsmcbeta` to enable Lilu enhancements on unsupported OS (13 and below are enabled by default).
- Add `-vsmcrpt` to report about missing SMC keys to the system log.
- Add `-vsmccomp` to prefer existing hardware SMC implementation if found.
- Add `vsmcgen=X` to force exposing X-gen SMC device (1 and 2 are supported).
- Add `vsmchbkp=X` to set HBKP dumping mode (0 - off, 1 - normal, 2 - without encryption).
- Add `vsmcslvl=X` to set value serialisation level (0 - off, 1 - normal, 2 - with sensitive data (default)).
- Add `smcdebug=0xff` to enable AppleSMC debug information printing.
- Add `watchdog=0` to disable WatchDog timer (if you get accidental reboots).

#### Credits
- [Apple](https://www.apple.com) for macOS
- [netkas](http://netkas.org) for the original idea of creating a software SMC emulator
- [CupertinoNet](https://github.com/CupertinoNet) for reversing most of MMIO protocol and SMC headers
- [Alex Ionescu](https://github.com/ionescu007) for the initial reverse of SMC firmware
- [07151129](https://github.com/07151129) for co-devoloping VirtualSMC and invaluable help during the research
- [lvs1974](https://github.com/lvs1974) for developing laptop sensor support
- [usr-sse2](https://github.com/usr-sse2) for developing laptop sensor support
- [joedmru](https://github.com/joedmru) for developing Super I/O chips support
- [theopolis](https://github.com/theopolis) for [smc-fuzzer](https://github.com/theopolis/smc-fuzzer) tool
- [kokke](https://github.com/kokke) for [tiny-AES-c](https://github.com/kokke/tiny-AES-c)
- [vit9696](https://github.com/vit9696) for [Lilu.kext](https://github.com/vit9696/Lilu) and this software
- Additional big thanks go to all the contributors and researchers involved in Apple SMC exploration!
