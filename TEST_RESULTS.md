# Test results — ESP32-2432S028 — September 12, 2026

Tested on the USB-connected `cyd-2432s028`, ESP32 rev3.1, ESPHome
2026.6.2 / ESP-IDF. The firmware reports compile time
`2026-09-12 09:54:02 +0200` and was written and verified over USB.

## Passed

- C++ regression tests for noise, contact bounce, jumping touches,
  real drag movements, temperature rounding, and `millis()` overflow.
- Five layout/configuration tests for screen bounds, overlap, modal
  layers, conditional pagination, and guards ahead of all tile handlers.
- ESPHome validation with `TILE_COUNT: 6`; navigation is tied in the code to
  `count > 6`, and tiles above the configured count stay hidden.
- Full ESP-IDF build and USB upload with verification of the written data.
- Connection to the existing Home Assistant installation and receipt of
  entity states.
- **10 page checks and 50 overlay cycles on the real device.**
  The render-start counter rose from 38 to 101 between the first and
  last page check: the test also verifies actual draw activity.
- Runtime geometry: tile area `(0,40)–(319,203)`, Next button
  `(211,206)–(310,237)`. The areas don't overlap.
- Free heap during the ten test measurements: 86,416–139,696 bytes
  per `esp_get_free_heap_size()`. No growing memory loss or reset
  during this short render test.

## Physical touch diagnostics

The user repeatedly tapped the four screen corners. The raw
measurement at top-left sometimes jumped more than a hundred pixels off. After the driver filter,
the samples stayed within recognizable corner clusters. The remaining skew was
corrected with an affine correction on the corner medians; the regression test
places those medians within five pixels of the target. The source values and
coefficients are in the configuration, tests, and CYD_STABILITY.md.

After the final flash, the user confirmed that the correct buttons respond
and the controls now work well.

## Limitations

This is not a long-duration endurance test. Home Assistant actions were not automatically
performed during the screen test. The user confirmed the physical controls after the final calibration correction
with: "Yes, this works well now". The ten-minute
standby is configured; nobody waited the full ten minutes to let the
complete idle timeout run out.

Local raw logs are in `diagnostics/acceptance-ui-test.log` and
`diagnostics/acceptance-runtime.log` and are not included in Git.

## Portable installation — additional software check

On September 12, 2026, the new onboarding flow was tested:

- 17 Python tests pass: layout, calibration fit, independent verification,
  faulty/noisy measurements, unique secrets, not overwriting, and export.
- Both C++17 regression programs pass (touch filter and UI guard).
- Starter ZIP unpacked into a fresh temporary folder; a new profile was created with its own
  generated keys. A full ESPHome 2026.6.2 / ESP-IDF build
  with calibration boot mode succeeded.
- Generated C++ checked for identity correction on the new board.
  The separate local profile retains the six previously measured correction coefficients.
- The Git index and existing history were checked for the local credentials;
  no matches. Local profiles and secrets are excluded.

The five-point wizard was tested with synthetic measurement data. In this
round it was not physically run again or flashed to the existing board;
physical acceptance remains mandatory for every new panel. The hardware test
above concerns the previously installed firmware.
