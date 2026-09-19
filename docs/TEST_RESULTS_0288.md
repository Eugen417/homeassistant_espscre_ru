# Test results app 0.2.88 / firmware 0.2.74 (2026-09-19)

The camera measurements of 0.2.87 showed the bench screens' Wi-Fi dozing: their Device Builder YAMLs date from before
app 0.2.24 and have no `power_save_mode`, so they ran ESPHome's default for the ESP32, `light` (`WIFI_PS_MIN_MODEM`: the
radio sleeps between the router's beacons, and what Home Assistant sends waits at the router until the next one). After
the pros and cons Max chose: "zet hem maar op none, voor iedereen vanaf nu voor altijd", with overriding still possible.

## What changed

`packages/core.yaml` has a `wifi:` block with one key, `power_save_mode: none`. ESPHome merges it under the screen's own
`wifi:` (the network, the password, the fallback hotspot stay there), and the screen's own YAML wins on any key it sets
(`merge_config` in `esphome/config_helpers.py`: the main config over every package, a later package over an earlier
one). The YAML ESP Screens writes has had the same line since app 0.2.24 and keeps it. The app's Override YAML still
refuses a `wifi:` section (`Firmware.PROTECTED_OVERRIDE_KEYS`), so the way to another mode is the screen's own YAML in
the ESPHome Device Builder.

## Checked

`esphome config` (ESPHome 2026.6.2) on copies of the bench screens' Device Builder YAMLs with the packages of this
branch:

| YAML | `power_save_mode` after the merge |
|---|---|
| Guition Wallbox, as the Device Builder has it (no line) | `NONE` |
| the same with `power_save_mode: light` under `wifi:` | `LIGHT` |
| CYD, as the Device Builder has it (no line) | `NONE` |

The Guition's build from its Device Builder YAML generates `set_power_save_mode(wifi::WIFI_POWER_SAVE_NONE)`.

## Measured on the bench Guition

Flashed over the air from its Device Builder YAML (no `power_save_mode` line) with this branch:

| | fw 0.2.73, `light` | fw 0.2.74, `none` from the package |
|---|---|---|
| Ping from the Mac (30 × 0.2 s) | 103-191 ms, 117 ms average | 5-89 ms, 15 ms average |
| Camera, cold open | 4.55 s | 4.37 s |
| Camera, warm open | 2.06 s | 2.08 s |
| Loop over 50 ms during an open | 5 of 12 opens (58-107 ms) | 1 of 5 opens (101 ms) |

With the 0.2.87 measurements of a power-save-off build (none in six opens), a loop waited over 50 ms in one of eleven
opens with the Wi-Fi awake. The camera times hardly change: the camera's own snapshot and the download dominate them.
The power the radio now spends listening was not measured; by Espressif's datasheets the receiver draws about
90-100 mA while it listens, a few tenths of a watt on a mains-powered screen.

## Automated

- `tools/check.sh`: Python 474 tests OK (new: `test_easy_package` "the Wi-Fi never dozes but stays the owner's": the
  core's `wifi:` block holds only that key, no other package has one, the app's YAML still writes it), C++ 19/19,
  packages, icons, editor tests, types, build and bundle PASS.
- `tools/check.sh --firmware --baseline 1678816`: CYD 1,678,816 B = 91.5 % (+0 B: the checkout profiles already set
  `power_save_mode: none`), Guition 2,055,280 B = 25.3 %.

## Not tested

- The CYD on its panel: config check only. It gets the setting with its next **Update**.
- Studio 1: its YAML is in the studio's Home Assistant; it gets the setting with its next **Update** unless its own
  YAML sets another mode.
