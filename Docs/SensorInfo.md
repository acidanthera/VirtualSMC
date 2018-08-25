#### Introduction

VirtualSMC is an emulator available under `SMC` name in I/O Registry, which is matched by AppleSMC.kext via its name, exported as `VirtualSMCAPI::ServiceName`. It contains a keystore providing a list of SMC keys to AppleSMC.kext via PMIO (first SMC generation, normally found in FakeSMC) and MMIO (second SMC generation) protocols. VirtualSMC is not only an emulator but also a library, which provides a plugin API (`VirtualSMCSDK`) to extend its functionality from external kexts.

The main objective of a plugin is to update the keystore by providing new keys or replacing the existing keys. Each plugin is allowed to submit a set of unique keys that do not conflict with other plugins, including key replacements, which can only be done once.

#### Key representation

Each key is a class instance derived from VirtualSMCValue, which represents an entity of:
- attributes, i.e. current value (`data`), size (`size`), access parameters (`attr`);
- access interfaces, i.e. `readAccess` and `writeAccess`, which allow to prepare the key current value before read or write operations;
- update interfaces, i.e. `update`, which is suppossed to perform current value modification and subsequent action execution.

Basic access parameters, like read, write, and authorise, are verified by VirtualSMC itself, so the main reason for  `readAccess` and `writeAccess` is to update the state of the current value or prohibit the access when the value is not available for whatever reason. Check the implementation of `#KEY` key, providing the amount of available SMC keys to a client in `VirtualSMCValueKEY::readAccess` for an example.

It is assumed that neither `readAccess`, nor `writeAccess`, not even `update` are invoked in an interrupt context, yet they may or may not be called within a page fault handler (`kernel_trap`) with a spinlock acquired. For this reason you are supposed not to use mutexes (e.g. `IOLock`) but rely on atomics (`vsmcatomic.h`) and spinlocks (`IOSimpleLock`).

#### Key submission

Each plugin is supposed to link to VirtualSMC.kext (for `VirtualSMCValue` common functions) and Lilu.kext (for logging, I/O Kit access, and other functionality) and is able to access their public interfaces exposed via relevant SDKs. Depending on the kernel and kext modification needs, a plugin may or may not be a Lilu plugin.

SMC provides two accessor types for the keys: by index and by name. This means that some keys are not indexed and are hidden. For this reason you are supposed to submit public and hidden keys separately.

In order to send the keys to the VirtualSMC keystore you are supposed to utilitse `VirtualSMCAPI::SubmitPlugin` function. This function takes a pointer to a `VirtualSMCAPI::Plugin` structure, describing your plugin and providing the relevant keys it implements. The `VirtualSMCAPI::Plugin` structure contains:
- plugin name (`product`)
- plugin version (`version`)
- VirtualSMC API version (`apiver`), must always be set to `VirtualSMCAPI::Version`
- **sorted** list of implemented public keys (`data`)
- **sorted** list of implemented hidden keys (`dataHidden`)

Since the loading order is non-linear, you are supposed to register a VirtualSMC registration handler by invoking `VirtualSMCAPI::registerHandler` with a callback. This callback will be invoked on `gIOFirstPublishNotification` basis when VirtualSMC service is published. Refer to `SMCProcessor::probe` and `SMCProcessor::vsmcNotificationHandler` for a plugin submission example.

#### Other APIs

- To submit SMC events you are supposed to use `VirtualSMCSDK::postInterrupt` API.
- To build up SMC key instances you may rely on `VirtualSMCSDK::valueWithData` API as well as shortcuts for different types.
- Value coding and decoding may be done via `VirtualSMCSDK::decodeSp` and `VirtualSMCSDK::encodeSp` APIs.

#### Drawbacks

Depending on SMC generation, version, and SMC branch, the list of supposedly exposed SMC keys is different. In general this is not critical, and most of the software is able to read the key values depending on key types. However, under certain conditions the minimal and maximum threshold may also be different. Refer to `iStat.txt` for different Mac profile details. It is useful to refer to `VirtualSMCAPI::getDeviceInfo` function to obtain the current emulated SMC capabilities and reflect the changes.

#### VMware hints

1. In order to completely disable a built-in SMC device replace the following string in a .vmx file:

```
smc.present = "TRUE"
```
by
```
smc.present = "FALSE"
```

In either case, the built-in SMC device will be turned off by default.

2. It is required to disable System Integrity Protection to be able to load VirtualSMC from `/Library/Extensions`, which is done from Recovery HD via `csrutil disable` command.

3. Do not forget about `-vsmcoff -liluoff -f` and UEFI Shell when debugging crashes.

#### Recommendations

In older times SMC was used as an aggregation of custom keys that are not related Apple SMC. For example, a CPU can be characterised by two sensor categories:
- current and temperature
- frequency and active cores

Historically Apple SMC is unable to access CPU frequency, and this was done by external software like Intel Power Gadget. External software does expose a dedicated API, which is supposed to be utilised by userspace programs. However, certain SMC emulators(HWSensors) decided to implement their own custom keys to include the values of their preference. As a result different software started to implement dedicated profiles to misinterpret the available keys. This ill practice should not be continued and tolerated. Please only add the keys normally found in Apple SMC and do not confuse the users.
