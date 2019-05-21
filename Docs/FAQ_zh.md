#### VirtualSMC 有对我有什么帮助？
在不同情况下，VirtualSMC 能改善现有跟未来版本的 macOS 的兼容性，带来更多 SMC 支持， SMC 键值模拟例如 MMIO ，事件 （SMCLightSensor），权限支持等功能。在旧款的 Mac 型号上，可以用它来升级 SMC 世代以获得更多功能。

#### VirtualSMC 有什么配置需求？
macOS 10.8.5 或以上. 确保使用正确的 Lilu 版本以获得完整功能, 在测试版的 macOS 下，使用 `-liluoff` 以启用基本支持。如果有用文件保险箱 (FileVault) 建议使用 VirutalSMC 以获得完整的 boot.efi 支持。 VirtualSMC 不兼容 SMCHelper-64.efi，请避免一起使用。 

#### 如何进行错误排查？
使用 DEBUG 版 Kext（也包括 Lilu）， 和相关的 Kext 的排错启动参数。 除了添加 `-vsmcdbg` `keepsyms=1`, `-v`, `debug=0x100`, `io=0xff` 这些常见的排错参数外，还有 AppleSMC 排错参数 (`smc=0xff`)， AppleSmartBatteryManager 排错参数 (`batman=0xff`)。  10.13 或以上可使用 [内核补丁](https://applelife.ru/posts/686953) 来获得内核崩溃追踪以避免使用后续 kext。祝好运。

#### 为什么内核崩溃报告会提到 VirtualSMC.kext？
大多数情况下 VirtualSMC 跟内核崩溃无关。 VirtualSMC 出现在堆栈跟踪是因为 VirtualSMC 把 kernel_trap 包装在模拟的 SMC 设备里。

#### SMC 传感器有什么用？
传感器 Kext 提供额外的信息例如温度， 电压， 还可通过特有 SMC 键值提供一些额外的功能。 已知的传感器列表可在 Docs 目录下查阅。 你可以使用特有 API 来开发传感器 Kext, 但请事先阅读一些基本文档。

#### 为什么用了监控器 Kext， 资源监控软件依然看不见相关信息？
检查传感器 Kext 是否有提供你想要查看的传感器信息， 可以通过查看 SMC 键值来确认 (运行 `smcread -s`). 如没有, 检测此信息是否在原生苹果产品使用上并考虑创建一个 Pull request。 如没有, 检查你所用的资源管理器软件确实有尝试读取相关键值。 请注意, 某些资源监控软件例如 iStat Menus 会根据不同的 Mac 平台使用其特有描述文件, 故某些你所使用的硬件也许会被忽略。

#### 是否支持 authenticated restart（ FileVault 免密码重启）?
Authenticated restart, 一般情况下由 `sudo fdesetup authrestart` 触发, 详情见 `man fdesetup`, 如有用 VirtualSMC.efi 即可支持。 可是, 正如任何软件功能整合一样，它并不是很安全。 如果你很在意安全与隐私, 你应该加上 `vsmchbkp=0` 启动参数禁止此功能。

此功能的整合由 VirtualSMC 把加密密钥储存在 NVRAM， 如有更高的 RTC 记忆库可用，还会由临时密钥进行再次加密。 除此之外, 如果 AptioMemoryFix 存在的话，在 EXIT_BOOT_SERVICES 后 AptioMemoryFix 会禁止此密钥被再读取。

#### Docs 文件夹里的文件有什么作用?
- `SMCBasics.txt` 包含关于 SMC 的基本信息
- `SMCKeys.txt` 包含已更新的 SMC 键值描述 (html 文档可利用此文件)
- `SMCKeysMacPro.html` 包含旧版 MacPro 的 SMC 键值描述键值描述
- `SMCSensorKeys.txt` 包含 `libSMC.dylib` SMC 传感器
- `SMCDumps` 包含 SMC 键值描述信息导出文件 ，可通过运行 `smcread -s` 生成。我们需要更多 SMC 信息导出文件，如有机会请补充。
- `SMCDatabase` 包含 SMC 固件信息导出文件， 可通过运行 `smcread` + 已更新的 SMC 固件
- `SMCTypes` 基于 `SMCDatabase`，创建，包含各世代 SMC 简述键值列表。
- `iStat.txt` 包含 iStat Menus 键值描述文件 (比如说当检测到 FakeSMC 时它会用 “FauxMac” 描述文件）
- `MacModels.txt` 包含 board-id <--> mac-model 简述和 SMC 固件导出文件状态
- `SensorInfo.md` 包含基本 SMC 知识 

#### 这些命令行工具都有什么作用?
- `rtcread` 可访问 RTC/CMOS 内存并包含相关 AppleRTC 信息
- `smcread` 可访问 SMC 键值, 导出 SMC 固件所包含的键值和 `libSMC.dylib`
- `smc-fuzzer` 一个原 `smc` fork，部分缺失的功能已由 `smcread` 补充
- `libaistat` 当 iStat Menus 跟 `DYLD_INSERT_LIBRARIES` 一起使用时，可导出 SMC 键值描述文件

#### 这些传感器 Kext 有什么作用?
- `SMCLightSensor` 通过新的 SMC 事件 API，是一个光线传感器的例子 (需要 `ACPI0008`/`_ALI`)
- `SMCBatteryManager` 添加 SMC 跟 SMBus 协议完整的 AppleSmartBattery 模拟层，电池相关的传感器
- `SMCProcessor`  给 Penryn CPU 或以上提供温度传感器支持
- `SMCSuperIO` 风扇信息读取