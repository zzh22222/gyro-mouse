# ESP32-S3 gyroscope mouse

A hand-held IMU that acts as a USB mouse: rotate your wrist, the pointer follows; turn the rotary encoder clockwise and it fires the left button. No driver, no Bluetooth - the board enumerates as a standard HID mouse. All code written from scratch.

## Hardware

- MPU6050-compatible module on I2C: `SDA` GPIO8, `SCL` GPIO9, address `0x68`
- Rotary encoder: `A` GPIO7, `B` GPIO6, common `C` GND, `VCC` left unconnected
- The board's **UART** port is used for flashing and the serial log; the **Native** port carries the HID mouse. Both are plugged in.
- Build: Arduino IDE 2.x, esp32 core 3.3.12, *ESP32-S3 Dev Module*, **USB CDC On Boot = Disabled**

## Why there are four probe sketches

The IMU is a clone (`WHO_AM_I` = `0x70`, not `0x68`) with larger and temperature-moving offsets, so nothing could be assumed:

- `i2c_health/` - checks `endTransmission` status and bytes returned. 16 lines at rest, `e1=0 n=14`, stable to 0.25 dps: wiring and power cleared before anything else was blamed.
- `axis_static/` - module flat on a desk, gravity direction identifies the axes. Result: `gz` yaw, `gy` pitch, `gx` roll (roll deliberately unused, which is what makes the pointer immune to wrist-roll jitter).
- `axis_probe/`, `axis_probe2/`, `dir_test/` - a gyroscope measures rate, so a "turn and come back" gesture has symmetric peaks and near-zero net integral: a fixed reporting window cannot recover gesture direction at all. These latch the max positive / max negative excursion of the running integral so the sign is readable after the fact.

Both direction signs were finally set by hand in the real grip: a sign measured on a desk does not transfer to a hand-held device (they differ by 180 degrees here).

## Drift

- Static offset: averaged over 500 samples at boot, module untouched.
- Thermal drift: the clone moves ~0.5 dps within minutes; at `SENS=40` that is ~20 px/s of pointer motion nobody commanded. Fixed by a leaky integrator - while judged still, the residual is absorbed into the bias with a ~1 s time constant (`TRK=0.005` per 5 ms cycle).
- Design error worth recording: the stillness threshold must be **larger** than the residual it corrects. At 0.8 dps, a steady 0.9-1.5 dps offset was classified as motion - integrated forever, never absorbed, a constant crawl. Now 1.5 dps, and stillness is judged only on `gz`/`gy`; `gx` is excluded because a still hand shows 3.8-13 dps there and would keep the device permanently "moving".

## Pointer output

Rate is integrated to pixels with a float accumulator that carries its remainder between reports, so sub-pixel rates are not lost; each report is clamped to the HID limit of 127. `DEADZONE` 0.3 dps suppresses the noise floor at gesture start.

## Trigger

The encoder works as a directional input, not a button: this module's push switch is not broken out on its five pins (`C` is the common, measured permanently low). On each falling edge of `A`, the level of `B` gives the direction; clockwise starts a 250 ms left-button hold, so turning continuously holds it down and counter-clockwise does nothing.

## Known limitations

Driven by raw body-frame rates with no attitude fusion, so yaw and pitch cross-couple once the wrist rolls (measured: `gy` 29.6 dps during a nominally pure-yaw move while `gz` was 8 dps). The fix is a rigid grip or 6-axis fusion. The module must be mounted rigidly - loose tape acts as a spring and produces 190 dps spikes.

`SENS` (40) is the only knob that matters for feel. Pin Windows pointer speed and turn off *Increase pointer precision* first, or the same rotation gives different pixel counts every time.
