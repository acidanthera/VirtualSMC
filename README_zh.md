VirtualSMC
========

[![Build Status](https://travis-ci.com/acidanthera/VirtualSMC.svg?branch=master)](https://travis-ci.com/acidanthera/VirtualSMC) [![Scan Status](https://scan.coverity.com/projects/16571/badge.svg?flat=1)](https://scan.coverity.com/projects/16571)

高级的 Apple SMC 核心模拟。 需要 [Lilu](https://github.com/vit9696/Lilu) 作为依赖以正常工作。

[English](README.md)  
简体中文 (当前语言)  

#### 特点
- 支持 64 位的 macOS 10.6 以及更新系统 (推荐使用 10.9 及以后的系统)
- 实现 MMIO 协议以及适配现代版本 macOS 的中断响应机制
- 正确提交键值属性以及它们的读写保护
- 允许根据机器型号动态调节使用不同代的 SMC
- 支持通过插件添加可扩展的传感器
- 支持启动参数 `smcdebug=XX` (10.9+)
- 替换查找到的硬件 SMC (你需要刷写特定的固件来完全禁用 SMC)

#### 启动代码
- 添加 `-vsmcdbg` 以开启 debug 输出 (需要 DEBUG 版本支持)。
- 添加 `-vsmcoff` 以禁用所有 Lilu 增强。
- 添加 `-vsmcbeta` 以在不支持的系统中启用 Lilu 增强 (11 及以下版本为默认开启)。
- 添加 `-vsmcrpt` 以报告缺失的的 SMC 密钥到系统日志中。
- 添加 `-vsmccomp` 以优先使用硬件 SMC（如果有）
- 添加 `vsmcgen=X` 以强制暴露 X-gen SMC 设备 (支持 1 和 2 代)。
- 添加 `vsmchbkp=X` 以设置 HBKP dumping mode (0 - 关闭, 1 - 普通, 2 - 没有加密)。
- 添加 `vsmcslvl=X` 以设置 serialisation level 的值 (0 - 关闭, 1 - 普通, 2 - 包含敏感数据 (默认))。
- 添加 `smcdebug=0xff` 以启用 AppleSMC debug 信息输出。
- 添加 `watchdog=0` 以禁用看门狗 (WatchDog) 倒计时 (if you get accidental reboots).

#### 感谢
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
- 非常感谢所有参与到 Apple SMC 项目中探索的贡献者和研究者!
