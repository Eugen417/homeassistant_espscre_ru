# Test results app 0.2.50 / firmware 0.2.43 (2026-09-15/16)

Memory for the CYD: layer-free sliders, a leaner tile model, and the old manual profile out of both
board profiles. See CHANGELOG 0.2.50 and RELEASING "Compatibility 0.2.50".

## Automated

- Python (`.venv-portal`): 175 tests OK. New: `tests/test_layer_free.py` (no style that makes LVGL draw
  into a buffer of its own, and every slider fill keeps its track's radius, in both profiles, both
  packages and the firmware headers) and `tests/test_easy_package.py` (packages current, no secrets or
  local paths, the manual profile gone from profiles and packages, runtime parts present, and the
  generator refuses a fixed Home Assistant subscription).
- C++ (`clang++ -std=c++17 -Wall -Wextra -Werror -I . -I components/smart_display`): 13/13 PASS, with
  `test_runtime_model.cpp` and `test_tile_controls.cpp` also clean under
  `-fsanitize=address,undefined`. `test_cyd_ui.cpp` and `test_light_controls.cpp` were compiling the
  stale header copies in the repository root; they include the component headers now and still pass.
- `generate_packages.py --check` green.
- `esphome config` of both Easy profiles is identical before and after the manual-profile cleanup,
  apart from two reordered lines: the firmware that was tested on the bench is the firmware released.

## Build sizes (ESPHome 2026.6.2)

| Profile | RAM 0.2.49 | RAM 0.2.50 | Flash 0.2.49 | Flash 0.2.50 |
|---|---|---|---|---|
| easy-cyd-device.yaml | 32.0% | 22.8% (74,668 B) | 78.6% | 75.5% |
| easy-guition-device.yaml | 33.0% | 23.8% (77,892 B) | 19.0% | 18.3% |

`runtime_tiles::model` (twenty tile slots) shrank from 24,296 to 8,216 bytes.

## Bench boards

Both boards were flashed over the air with firmware 0.2.43 and ran from 19:08 to 08:00 the next
morning without a restart: `last_boot` unchanged, `Synced`, no memory warning in the log.

| | CYD before (0.2.42) | CYD after | Guition before | Guition after |
|---|---|---|---|---|
| Free heap after the sync | 64–77 KB | 101–103 KB | 51–65 KB | 84–97 KB |
| Lowest free heap since boot | not measured | 78.1 KB over 13 hours | not measured | 62.8 KB |
| Largest free block | 48 KB | 48 KB | 31.7 KB | 36.9–57.3 KB |

The owner used both screens for about ten minutes right after the update: the brightness slider of a
light tile and of the light card, the vacuum card with its cleaning settings, cards and page swipes.
Home Assistant confirmed every action within about 0.3 s. No `lv_draw_buf_create_ex: No memory`, no
watchdog reset, no rollback. On the Guition, LVGL reported 216–276 ms for single operations while a
card was being rebuilt, as it did before this release.

## Not tested / for the owner

- A screen that still ran the old manual profile. There are none left; the route is gone.
- The USB touch calibration was not measured again; its code and documentation are unchanged.
