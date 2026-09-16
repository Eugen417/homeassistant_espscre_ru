# Test results app 0.2.55 / firmware 0.2.47 (2026-09-16)

A light's colour slider on Studio 1 sometimes turned red on release. The chain behind it, all measured
live on Studio 1 (firmware 0.2.43) and Max's Home Assistant: an automation with a state trigger on the
six-lamp group `light.studio_1_lights` and no `to:` set `number.studio_1_standby_after` to its current
value after every colour change. The screen reported it (`esphome.screen_setting`), ESP Screen Manager
saved the layout and resent layout, header and all 13 tile states, and the screen drew its whole page
again. While it did, it stopped reading the touch panel. The GT911's stray (0, 0) contact then became
the finger's last position, and LVGL moved the slider to its end on release. See CHANGELOG 0.2.55 and
the compatibility note in docs/RELEASING.md.

## Live measurements before the fix (read-only captures)

- 20 colour commits from Studio 1 captured as `call_service` events: 17 carried the colour under the
  finger. Two were `hs_color [0, 100]` (red) on `light.studio_1_lights` that Max had not chosen, at
  10:29:07.484 and 10:29:11.132. They left 75 ms and 84 ms after a full resync reached the screen. The
  third, `[360, 100]` on the ambilight, Max chose on purpose.
- Every colour change on the lamp group was followed by layout, header and all tile states within
  0.7 s. On `light.studio_2_ambilight`, which the automation does not watch, only that one tile was sent.
- The chain reproduced on demand, with Max's permission: `number.set_value` 86400 on
  `number.studio_1_standby_after` (unchanged) → `esphome.screen_setting` at +0 ms → a dry replica of
  the add-on logged `sent.pop by server.py:713 run <- server.py:526 save` → the real add-on sent layout,
  header and 14 states at +341 ms.
- The colour card does not follow Home Assistant while open (no subscriptions since 0.2.50), so no
  incoming message can move its knob directly. LVGL 9.5 sets a slider's value from the last touch point
  on release (`update_knob_pos(obj, false)`), before the card's own RELEASED handler commits it.
- Earlier Studio 1 logs: a stray (0, 0) was the last sample in 4 of 90 traced edge touches, always just
  before release (`swipe id=0 st=2 x=0 y=0`, then `st=6 x=0 y=0`).

## Automated

- Python (`.venv-portal`): 214 tests OK. New `tests/test_quiet_resync.py` (5 tests): an unchanged
  setting event is neither saved nor resent, while a real change and a brightness that lowers the standby
  brightness are. Every setting number in both profiles and both packages returns before its preference
  write and event. `layout_changed()` / `refresh_all()` are guarded by the change conditions. The
  Guition installs the (0, 0) filter and the edge swipe skips it, the CYD does not. Both slider handlers
  log a jump on release.
- C++ (`clang++ -std=c++17 -Wall -Wextra -Werror -I. -I components/smart_display`): 14/14 PASS.
  `tests/test_cyd_ui.cpp` adds `GhostTouch` (a stray sample repeats the previous read, a touch cannot
  start in the corner, the left edge itself and a rotated corner stay real touches) and `release_jump`.
- `generate_packages.py --check` green.

## Builds (ESPHome 2026.6.2)

Both Easy Setup profiles, one after the other, from the generated packages with local components:

| Profile | RAM | Flash | vs 0.2.54 |
|---|---|---|---|
| easy-guition-device.yaml | 24.0% (78,732 B) | 18.4% (1,496,775 B) | +40 B RAM, +1,468 B flash |
| easy-cyd-device.yaml | 23.0% (75,484 B) | 76.1% (1,396,799 B) | +16 B RAM, +1,116 B flash |

## Not tested / for the owner

- Hardware. Nothing was flashed. Studio 1 is Max's production screen, and its Wi-Fi password lives only in
  ESPHome Device Builder, so Max updates it through Home Assistant. On real glass after the update:
  drag the colour slider of the lamp group several times while the automation runs. Every commit should
  carry the colour under the finger, the log should show no full resync after a colour change, and it may
  show `touch: GT911 stray contact at (0,0) ignored`.
- A host build was not run for this release. The read-callback wrapper and the layout guard were only
  compiled for the ESP32 boards.
