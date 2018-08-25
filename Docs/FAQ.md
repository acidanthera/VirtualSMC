#### What is the benefit?
Depending on the circumstances VirtualSMC can bring better compatibility with present and future macOS releases providing broader SMC feature support and allowing more flexible SMC key emulation like MMIO, events (like in SMCLightSensor), permission support, etc. On older Macs it can be used to upgrade SMC generation with more features.

#### What are the requirements?
macOS 10.8.5 or newer. Compatible Lilu is required for full functionality, basic functionality will be available even on beta macOS versions or with `-liluoff` boot-arg. VirtualSMC.efi module is recommended for boot.efi compatibility when FileVault 2 is enabled. SMCHelper-64.efi is not compatible with VirtualSMC.efi and must be removed.

#### How can I debug issues?
Using DEBUG kexts and the usual boot-args to enable debug information in relevant kexts. Other than `-vsmcdbg` and generic boot-args like `keepsyms=1`, `-v`, `debug=0x100`, `io=0xff` you may be interested in AppleSMC debug support (`smc=0xff`) or AppleSmartBatteryManager debug support (`batman=0xff`). On 10.13 or newer you may have to [patch the kernel](https://applelife.ru/posts/686953) to be able to view the panic trace without the subsequent kext list. Good luck.

#### What is special about sensors?
Sensor kexts provide extra monitoring functionality like temperature, voltage, or extra functionality in dedicated SMC keys. The list of known SMC keys and sensors could be found in Docs directory. You are welcome to implement your own sensor kexts using a dedicated API, but check the basic manual first.

#### Why does sensor X not show the information in Y?
Make sure, that the information you are looking for is actually implemented in sensor X by checking the SMC key values (run `smcread -s`). If it is not, then make sure that this information is present on Apple hardware and consider making a pull-request. If it is not, then make sure that the software you check the information with does try to read the relevant key. Be aware, that some software like iStat Menus implement special sensor profiles based on Mac platform, and may not read all the sensors available.

#### Is authenticated restart supported?
Authenticated restart, normally invoked by `sudo fdesetup authrestart`, see `man fdesetup`, is supported when VirtualSMC.efi is present. However, it is insecure just as any software implementation is. If you care about privacy and security, you should disable it by passing `vsmchbkp=0` boot-arg.

The VirtualSMC implementation stores the encryption key in nvram and may encrypt it with a temporary key if higher RTC memory bank is available. Other than that, AptioMemoryFix will try to prevent reads of the encryption key after EXIT_BOOT_SERVICES, if installed.

#### What are the files in Docs?
- `SMCBasics.txt` contains generic information about SMC
- `SMCKeys.txt` contains documentation about SMC keys
- `SMCLegacyKeys.html` contains old documentation about SMC keys
- `SMCSensorKeys.txt` contains `libSMC.dylib` SMC sensor keys
- `SMCDumps` contains SMC key dumps created by running `smcread -s`, please add more!
- `SMCDatabase` contains SMC firmware dumps created with `smcread` from Apple updates
- `SMCTypes` contains summarised SMC key lists across SMC generations based on `SMCDatabase`
- `iStat.txt` contains iStat Menus key profiles (FauxMac is a profile when FakeSMC service is detected)
- `MacModels.txt` contains board-id <--> mac-model summary with SMC firmware dump statuses
- `SensorInfo.md` contains basic sensor how-to

#### What are the tools all about?
- `rtcread` allows to access RTC/CMOS memory and contains relevant AppleRTC information
- `smcread` allows to access SMC keys, dump SMC firmwares and `libSMC.dylib`
- `smc-fuzzer` is a fork of an old `smc` tool with some features missing in `smcread`
- `libaistat` allows to dump SMC key profiles from iStat Menus when used with `DYLD_INSERT_LIBRARIES`

#### What are the sensors for?
- `SMCLightSensor` implements an example of illumination sensor handling with a new SMC event API (requires `ACPI0008`/`_ALI`)
- `SMCBatteryManager` implements a complete emulation layer of AppleSmartBattery of SMC and SMBus protocols
- `SMCProcessor` implements temperature monitoring for Nehalem CPUs or newer
