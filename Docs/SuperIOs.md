## SMCSuperIO fan configuration
SMCSuperIO can read fan speeds on ITE/Nuvoton/Winbond and set fan speeds on ITE. The default configuration often works but there can be issues such as fan speed controlling a different fan, set speed not matching or non-existent fans showing.

You can find out how to configure this module here.

`#` in the property name should be equal to the fan number displayed in your fan control software (eg. first fan = fan0, second fan = fan1). Hide non-existent fans last as doing it first will make it harder to figure out correct fan index.

### fan#-hide
Hides the fan when set to value 01 or above. Example `01` **MUST** be type **Data**.

### fan#-control
The control index to use for the fan, useful for fixing a different/non-existent fan being controlled when setting speed. By default it's equal to the fan index which works for some configurations. Example `02`, **MUST** be type **Data**.

### fan#-pwm
RPM to PWM curve. It can be automatically generated with tool `fanpwmgen`.

```
❯ ./fanpwmgen -h
VirtualSMC fan#-pwm generation tool
Usage:
./fanpwmgen [options]
    -f <num>   : the fan number to use
    -s <steps> : number of steps to use when generating curve, default 16 and max 256
    -t <time>  : time to wait before going to next step, default 2 seconds
    -h         : help
```

Before running `fanpwmgen` you **MUST** make sure there is no `fan#-pwm` curve already set for that fan. Also **make sure all fan control software is closed**

For example to generate a curve for fan #1 use `./fanpwmgen -f 1`. Once finished running it will output the fan curve after `fan#-pwm: ` which you can add to DeviceProperties. **MUST** be type **String**.