# XPT2046 input stabilization

Based on ESPHome 2026.6.2's `esphome/components/xpt2046` (license alongside).
The sample handoff in `touchscreen/xpt2046.cpp` and filter/calibration members
in `touchscreen/xpt2046.h` differ from upstream. `touch_filter.h` is local.

The installed CYD produced widely separated coordinates at first contact.
The filter requires three samples within 100 ADC units before reporting a
new contact, ignores isolated large jumps, and requires two absent-pressure
samples before reporting release. Normal continuous drags use a small
moving average; large movements are accepted after three stable samples.

Use polling (20 ms in the device configuration), not an IRQ-only setup: the
filter needs follow-up samples while confirming a contact or release.
Run `tests/test_touch_filter.cpp` when changing the filter or updating ESPHome.

An optional affine raw-coordinate correction defaults to the identity.
The device profile sets its six coefficients at boot, fitted to median
filtered corner readings from this particular panel. Raw diagnostic getters
still expose the original filtered ADC values, allowing recalibration without
mistaking corrected coordinates for physical measurements.
