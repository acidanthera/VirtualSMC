## SMC Fuzzer

Originally taken from [smc-fuzzer](https://github.com/theopolis/smc-fuzzer) with minor modifications.

*devnull*'s SMC read/write code, along with simple fuzz options.

This `smc` tool uses the `AppleSMC` IOKit interface and a userland API
for interacting with the System Management Controller (Mac embedded controllers).
The tool focuses on the SMC key/value API, but could be expanded to more API methods.

### Help / Usage

```
$ ./smc -h
Apple System Management Control (SMC) tool 1.01
Usage:
./smc [options]
    -c <spell> : cast a spell
    -q         : attempt to discover 'hidden' keys
    -z         : fuzz all possible keys (or one key using -k)
    -f         : fan info decoded
    -h         : help
    -k <key>   : key to manipulate
    -l         : list all keys and values
    -r         : read the value of a key
    -w <value> : write the specified value to a key
    -v         : version
 ```

### Discover unreported keys

Use the `-q` switch to brute force discover ((125-33)^4) readable keys.

#### Results

On a few Mac Pros this has found:

```
  CRDP  [flag]  (bytes 00)
  FPOR  [ch8*]  (bytes 00 00 00 00 00 00 00 00)
  KPPW  [ch8*]  (bytes 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00)
  KPST  [ui8 ]  0 (bytes 00)
  MOJO  [ch8*]  (bytes 00)
  MSSN  [ui16]  5 (bytes 05)
  OSK0  [ch8*]
  OSK1  [ch8*]
  zCRS  [ui8 ]  (bytes 00)
```

`OSK0` and `OSK1` are the 64-byte [binary protection key](http://osxbook.com/book/bonus/chapter7/tpmdrmmyth/).

`KPPW` and `KPST` are protection inputs and status keys.
Refer to Alex Ionescu's RECON 2014 talk and use the `-c` switch.
~Setting the correct key allows enumerating 'hidden' keys and mutating their values.~

### A bad time

Changing simple values as the super user will halt your machine.
Example: if you set `DUSR` to `1` your machine will instant-halt.

```
$ make
$ sudo ./smc -z
```

### A disappointing time

No value should be writable as a non-privileged user.

```
$ make
$ ./smc -z
```

### Resources

- [@jBot-42](https://github.com/Jbot-42) - [A complete guide to SMC](http://jbot-42.github.io/Articles/smc.html) for programmers
- Alex Ionescu - [Apple SMC The place to be definitely, for an implant](https://www.youtube.com/watch?v=nSqpinjjgmg) (RECon 2014 video)
- Alex Ionescu - ["Spell"unking in Apple SMC Land](http://www.nosuchcon.org/talks/2013/D1_02_Alex_Ninjas_and_Harry_Potter.pdf) NoSuchCon 2013
- InversePath (Andrea Barisani, Daniele Bianco) - [Embedded systems exploitation](https://dev.inversepath.com/download/public/embedded_systems_exploitation.pdf), includes several slides on SMC
- Trammell Hudson - [Thunderstrike 31C3](https://trmm.net/Thunderstrike_31c3), a class of EFI/firmware attacks
- Apple - [HT201518](https://support.apple.com/en-us/HT201518) downloads for several SMC versions


