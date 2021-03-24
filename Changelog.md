VirtualSMC Changelog
====================
#### v1.2.2
- Improve manual fan control in SMCDellSensors (SMM access is enabled even if audio is played)
- Fixed sensor DEBUG logging with `-liludbgall` argument
- Improved startup performance when probing SuperIO chips by splitting vendors
- Added SuperIO device activation when it is disabled on probe

#### v1.2.1
- Fix version publishing for VirtualSMC and plugins

#### v1.2.0
- Improve manual fan control in SMCDellSensors (switch off manual control before going to sleep), rename control boot-args (start with -dell)

#### v1.1.9
- Improve manual fan control in SMCDellSensors (use control registers 0x35a3 and 0x34a3 to cover more Dell models)
- Fix processKext in SMCDellSensors (could be called multiple times for the same kext since flag Reloadable was set)
- Reduce audio lags in SMCDellSensors when USB audio device is used
- Allow not injecting TB0T SMC key when it is unavailable in SMCBatteryManager

#### v1.1.8
- Reduce audio lags in SMCDellSensors

#### v1.1.7
- Added MacKernelSDK with Xcode 12 compatibility
- Fixed SMCDellSensors loading on macOS 10.8
- Added VirtualSMC support for 10.6 (most plugins require newer versions)
- Fixed rare kernel panic in SMCSuperIO

#### v1.1.6
- Added battery supplement info, thx @zhen-zen
- Fix audio lags in Safari caused by reading SMM in SMCDellSensors plugin
- Fix module version for SMCDellSensors, SMCBatteryManager and SMCLightSensor
- Optimised floating point sensor key reading with fewer arithmetic operations
- Improved SMCProcessor CPU power consumption by relaxing core synchronisation
- Fix key sensor key enumeration on Macmini8,x and MacBookPro models

#### v1.1.5
- Improved CHLC key value reporting
- Fixed B0PS and B0St key size to resolve broken fully charged state
- Fixed sometimes stuck battery update thx to @zhen-zen
- Added workaround for kBRemainingCapacityCmd exceeding kBFullChargeCapacityCmd
- Added preliminary 11.0 support
- Fixed SMCProcessor model detection warning
- Fixed legacy smc tool value calculation
- Fixed running smcread on 11.0 without IOKit framework
- Added a new plugin SMCDellSensors for Temp/FAN monitor/control
- Added basic SMCBatteryManager compatibility with 11.0
- Fixed crashes when trying to read CLKT key

#### v1.1.4
- Fixed incorrect revision reporting on T2 models (e.g. Macmini8,1)

#### v1.1.3
- Fixed compatibility with 10.15 debug kernel with traptrace enabled

#### v1.1.2
- Improved performance with Lilu 1.4.3 APIs

#### v1.1.1
- Fixed SMCSuperIO crashes with unsupported chips
- SMCSuperIO detected chip name to ioreg

#### v1.1.0
- Minor performance improvements
- Added OpenCore builtin protocol support (VirtualSmc.efi can still be used in other bootloaders)
- Added fan and voltage reporting in SMCSuperIO through I/O Registry (requires client updates) by @joedmru

#### v1.0.9
- Fixed multiple issues with charge level reports in SMCBatteryManager

#### v1.0.8
- Added Xcode 11 compatibility to plugin kexts on 10.14

#### v1.0.7
- Unified release archive names
- Added new ssio sensors

#### v1.0.6
- Fixed `vsmcgen=1` support on select models
- Improve SMCBatteryManager compatibility with 10.15

#### v1.0.5
- Allow loading on 10.15 without `-lilubetaall`
- Fixed SMCBatteryManager compatibility with 10.15
- Changed RGEN 3 -> 2 for AppleIntelPCHPMC.kext compatibility

#### v1.0.4
- Removed exposed REV, RBr, RPlt keys from I/O Registry
- Minor EFI driver compatibility improvements
- Synced RVUF, RVFB with REV from booter keys
- Fixed prebuilt revisions for GEN 3 chip emulation
- Dropped custom prebuilt revisions in favour of loader inject

#### v1.0.3
- Added multiple new ssio sensors
- Improved 3rd generation SMC support (they have no REV and RBr keys)

#### v1.0.2
- Fixed TC0C/TC1C selection in SMCProcessor to match mac models
- Added per-plugin debug switches (`-scpudbg`, `-sbatdbg`, `-ssiodbg`, `-alsddbg`)

#### v1.0.1
- Added Penryn CPU support to SMCProcessor
- Improved keystore management
- Initial implementation of SuperIO devices: support fans reading

#### v1.0.0
- Initial release
