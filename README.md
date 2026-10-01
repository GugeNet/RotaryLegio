# RotaryLegio — v1.5

A rotary-speaker effect for the Noise Engineering Legio platform, built with libDaisy. Independent horn and drum models provide stereo movement, Doppler modulation, and mechanical acceleration and deceleration. Three cabinet voicings, microphone geometry, horn/drum balance, and drive shape the sound.

## Quick start

1. Install the firmware below.
2. Connect audio to the inputs and both outputs to your stereo signal chain. The effect averages the two inputs into one cabinet and produces stereo output. Bypass preserves separate left and right channels.
3. Center both switches and both pots, with Hit and Pitch unpatched.
4. The effect starts enabled at Slow speed. Move the left switch up to accelerate to maximum, or down to brake.

No calibration is required. Settings reset at power-off.

## Controls

| Control | Function |
| --- | --- |
| Left switch: up / center / down | Maximum speed / Pitch-controlled speed / Brake |
| Right switch: up / center / down | Open / Classic / Dark cabinet |
| Decay pot + CV | Doppler depth |
| Wack pot + CV | Microphone distance and stereo spread |
| Encoder short press | Cycle Maximum speed / Horn–drum balance / Drive |
| Encoder turn | Adjust the selected parameter |
| Encoder hold for 0.8 seconds | Toggle latched bypass in any switch position |
| Pitch CV | Slow at 0 V; linear sweep to selected maximum at +5 V |
| Hit high, left switch centered | Request selected maximum; release returns to Pitch target |

### Encoder and LEDs

The encoder starts on the maximum-speed page. One, two, or three white flashes identify Maximum speed, Horn–drum balance, or Drive. Right LED brightness shows the selected value.

During normal operation the LEDs pulse with horn and drum rotation. Steady dim white indicates bypass. Continuous rapid white flashes on both LEDs indicate audio callback CPU load exceeded 80%; this warning stays latched until reboot.

### Speed behavior

Slow is fixed at horn 46.2 RPM / drum 43.2 RPM. Maximum defaults to 414/384 RPM. The encoder adjusts maximum over horn 207–621 RPM / drum 192–576 RPM without changing Slow.

With the left switch centered and Hit low, Pitch sweeps from Slow to the selected maximum. Negative CV is clamped to Slow. Hit high and the Fast switch position reach the same maximum as +5 V Pitch. Brake overrides both. All speed changes retain inertia; allow several seconds to settle. Stopping rotation still passes audio through the stationary cabinet.

Pitch controls rotation speed, not audio pitch, and is not a 1 V/octave control. The documented Pitch input range is -2 V to +5 V; use 0–5 V for the speed sweep. Use sustained Hit gates for an obvious acceleration; short pulses may have little audible effect.

## Firmware installation

The prebuilt image is [firmware/LegioRotary.bin](firmware/LegioRotary.bin). Connect the module by USB and enter its STM32 ROM DFU bootloader using the module's bootloader procedure. This image targets internal flash at `0x08000000`.

With dfu-util on PATH, run from this repository:

```sh
dfu-util -a 0 -s 0x08000000:leave -D firmware/LegioRotary.bin -d ,0483:df11
```

Windows with the default Daisy toolchain installation:

```powershell
& 'C:\Program Files\DaisyToolchain\bin\dfu-util.exe' -a 0 -s 0x08000000:leave -D firmware/LegioRotary.bin -d ',0483:df11'
```

Restart normally after flashing if necessary. No calibration or special startup gesture is needed.

## Build from source

The source and Makefile are unchanged from v1.5. Requirements: GNU Make, ARM embedded GCC, and libDaisy. The supplied image was built with ARM GCC 10.2.1 and libDaisy commit `facb66c76b5482918741695f4268b0185e474644`.

From this repository, create a sibling dependency checkout and build:

```sh
git clone https://github.com/electro-smith/libDaisy.git ../libDaisy
git -C ../libDaisy checkout facb66c76b5482918741695f4268b0185e474644
git -C ../libDaisy submodule update --init --recursive
make -C ../libDaisy -j
make LIBDAISY_DIR=../libDaisy -j
```

Put the ARM toolchain executables on PATH. `LIBDAISY_DIR` overrides the original development path retained in the Makefile. Build output goes to `build/`, which Git ignores. libDaisy is external and is not bundled.

### Tests

Run in an x64 Visual Studio developer PowerShell:

```powershell
./tests/run-tests.ps1
```

Six suites cover button handling, DSP, cabinet voicing, switch mapping, rotor tuning, and Speed CV. See [BUILD_RESULT.md](BUILD_RESULT.md) for recorded build results and the firmware SHA-256 checksum.

## Validation status

The ARM build and all six host test suites passed. The fixed Pitch conversion assumes normalized ADC 0.25 at 0 V and 0.875 at +5 V. These are provisional values, not measured endpoints from the target module; electrical endpoint accuracy remains to be verified. Hardware CPU profiling, stack measurement, and sustained-operation checks also remain pending.

For a hardware check, center the left switch and compare 0 V with +5 V, allowing about six seconds to settle. At +5 V, applying Hit should not increase speed. Adjust maximum with the encoder: +5 V and Hit should follow the new maximum, while 0 V stays Slow.
