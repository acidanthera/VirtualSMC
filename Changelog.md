VirtualSMC Changelog
====================
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
