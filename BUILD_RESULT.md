# Build result — v1.5, 2026-09-30

ARM GCC 10.2.1 compile/link succeeded without warnings.

- Flash: 105,656 bytes (80.61% of 128 KiB).
- AXI SRAM: 31,824 bytes; D2 SRAM: 16,704 bytes; backup SRAM: 12 bytes.
- No SDRAM allocation; 480 MHz, 48 kHz, 48-frame callbacks.
- Binary SHA-256: 2232702984D49320F2F2646A89E692716194DCBD39AF92F02AEECF21F182FA7B
- Six optimized MSVC suites passed: button, DSP, voicing, panel mapping, rotor tuning, Speed CV.
- Speed coverage includes always-active startup, interpolation and clipping, fixed Slow, encoder maximum, Hit/Fast/CV agreement, Hit release, debounce, Brake priority, nonfinite input, reboot reset, and full-engine inertia/stop.
- Calibration/session tests and implementation are absent because that feature was removed.

Hardware electrical endpoints, audition, stack usage and CPU profiling remain unverified. See README.md for the provisional fixed voltage conversion. This image has not been flashed by the agent.

## RotaryLegio naming verification — 2026-10-01

Renamed the source and build target to RotaryLegio and rebuilt successfully without warnings. The rebuilt RotaryLegio.bin is byte-for-byte identical to the original v1.5 image; the checksum and memory figures above are unchanged.
