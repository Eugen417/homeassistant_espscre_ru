# Test results 0.3.6 (firmware 0.3.1)

The 7-inch Guition JC1060P470 and JC1060P470 V2, added as experimental boards (GitHub #28).

## What was checked

- `esphome config` (ESPHome 2026.9.0) on both new checkout entries: valid. The first panel builds on ESPHome's own
  `model: JC1060P470` with a 20 MHz Wi-Fi link; the V2 on `model: CUSTOM` with the new panel's timings and init table
  and a 10 MHz link. Every option they use exists in ESPHome 2026.6.2, the packages' `min_version`, as well.
- `esphome compile` of both, ESPHome 2026.9.0: both build. Flash 2,118,770 bytes (first panel) and 2,118,802 bytes (V2),
  26.1 % of the 8,126,464-byte slot; static RAM 125,778 and 125,786 of 576,464 bytes.
- The 10.1-inch JC8012P4A1 moved its ESP32-P4, Wi-Fi, touch bus and backlight blocks into the shared
  `packages/hardware/guition-esp32p4.yaml`. Its resolved `esphome config` before and after is the same data (only the
  order of keys differs, and the new `HOSTED_SDIO_FREQUENCY` substitution at its old 40 MHz).
- `tools/render/run.py jc1060p470 jc1060p470-portrait`: the host build of the real firmware passes its self test lying
  down (1024 x 600, 4 x 4) and standing up (600 x 1024, 2 x 7), 40 page checks each.
- `tools/check.sh`: every fast check passes.

## Not checked

Nothing ran on the glass: nobody on the project has this board. Touch direction, which way up the picture is for the
case, colours, the reset after a warm restart, the Wi-Fi link's stability, standby and the wake, and free heap with a
full layout wait for testers. docs/JC1060P470.md lists what to report.
