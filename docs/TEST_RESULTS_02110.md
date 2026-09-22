# Validation for 0.2.110, firmware 0.2.94

Experimental Waveshare ESP32-S3-Touch-LCD-7 support, checked on 22 September 2026.
The work started from main at `cf305ce`. No physical 7-inch board was available.

## Automated checks

`tools/check.sh`: 13 checks passed, no warnings or failures.

- Python 3.13: 580 tests, including existing data and profile compatibility.
- C++: all 24 test programs.
- Editor: 123 Vitest tests, TypeScript checks and the production build. The committed bundle matches the build.
- Package, cell, board metadata, icon and translation checks.
- Installer coverage for the experimental label, the always-on backlight explanation, the 16-cell and 14-cell choices,
  and the board/orientation submitted when creating a screen. Both orientations and camera boxes pass the backend checks.

## Firmware builds

`tools/check.sh --firmware --baseline 1624080`, with the add-on's ESPHome 2026.9.0.
Every shipping board compiled sequentially, using placeholder secrets and the Wi-Fi fallback access point and captive portal.
Nothing was uploaded.

| Board | OTA image, bytes | OTA slot, bytes | Used |
|---|---:|---:|---:|
| `cyd` | 1,624,080 | 1,835,008 | 88.5% |
| `guition` | 2,039,424 | 8,126,464 | 25.1% |
| `waveshare43` | 2,295,440 | 8,126,464 | 28.2% |
| `jc8012p4a1` | 2,010,240 | 8,126,464 | 24.7% |
| `waveshare7` | 1,825,952 | 3,932,160 | 46.4% |

The CYD remains at 88.5%, with zero byte growth from 0.2.109.
The new board's configuration also passes `esphome config` on the minimum supported ESPHome 2026.6.2.
That minimum-version check validates configuration; it is not a local minimum-version firmware compilation.

## Host rendering

The actual shared core and new board layout were compiled with ESPHome's SDL host display and template hardware outputs.
A local demo server provided fictional entities, with no connection to a real Home Assistant instance.

Both landscape (800 x 480, 4 x 4) and portrait (480 x 800, 2 x 7) were rendered:
two pages of mixed tiles, page buttons hidden, settings, light, climate, cover, blind, media, vacuum, history,
colour controls, the busy indicator and an alert. All captures settled without geometry warnings.
Representative page, settings, detail-card and alert captures were visually inspected, with no clipped controls found.

The firmware's `ui_self_test` passed with page buttons shown and hidden in each orientation:
40 page checks in total, repeated colour and alert overlay renders, and all four light-slider checks passing.
The colour preview's 40-second action isolation was allowed to finish before running the slider callback checks.

## Physical acceptance still pending

No physical boot, touch, colour order, power cycle, backlight wake, Wi-Fi, camera download or long-running heap test
was performed on this board. Physical uptime measured: none. Camera support is enabled through the shared implementation,
but the host render does not establish its performance on the ESP32-S3.

The backlight stays on; standby, night mode and alert backlight flashes are disabled.
See [the board guide](WAVESHARE7.md) for hardware details and the physical testing checklist.
