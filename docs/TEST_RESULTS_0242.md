# Test results app 0.2.42 / firmware 0.2.36 (2026-09-15)

Smooth page swipes, slider handles inside the fill, the busy sheet over the whole card and a new
colour card for lights. See CHANGELOG 0.2.42, RELEASING "Compatibility 0.2.42" and
docs/SWIPE_PROFILE.md for the measurements.

## Automated

- Python (`.venv-portal`): 139 tests OK.
- C++ (clang++ -std=c++17 -Wall -Wextra -Werror -I.): 13/13 PASS.
- `generate_packages.py --check` green; `node --check app.js` clean.
- ESPHome 2026.6.2 compile SUCCESS, one after another per board (manual profile first, then Easy
  Setup, so the upload took the Easy build): `guition-device.yaml` (RAM 32.6%, flash 19.7%),
  `easy-guition-device.yaml` (RAM 32.5%, flash 18.7%), `device.yaml` (RAM 31.6%, flash 82.3%),
  `easy-cyd-device.yaml` (RAM 31.5%, flash 77.5%). Packages carry `SCREEN_FIRMWARE_VERSION` 0.2.36.

## Bench boards (USB)

- Both Easy Setup builds uploaded over USB; `device_info` reports project 0.2.36, compiled
  2026-09-15 10:54:46 (Guition) and 10:54:48 (CYD), matching the build logs.
- `diagnostics/run_ui_test.py` on both release builds: 10/10 `page_check=PASS`, 50 overlay render
  cycles, no failures, no `page fill still under way` line. The same passed on the diagnostic
  builds before.
- Swipe measurements with `-DSWIPE_PROFILE=1` builds (same code plus the profiler), 12 page
  switches each through `swipe_test`, median / p90:
  - Guition (owner's layout, 10 tiles, 2 pages): first frame 52 / 54 ms, page complete
    244 / 250 ms, longest loop gap 74 / 75 ms (baseline 276 / 283 ms), slowest frame 40 / 42 ms.
  - CYD (8 tiles, 2 pages): first frame 83 / 84 ms, page complete 250 / 256 ms, longest loop gap
    109 / 110 ms (the skeleton frame over SPI).
- Earlier builds of this round were swiped by hand by the owner on the Guition (33 swipes for the
  baseline, 30 on the step fill); a fade variant was tried and dropped at the owner's request.

## Renders (ESPHome host + SDL, both boards, firmware code of this release)

- Light cards at 1, 3, 5, 60, 90 and 100 %: the white handle sits inside the fill on mini sliders
  and control sliders, 1 % shows one round end with the handle inside.
- Busy sheet on a wide card with brightness controls: covers the whole card, spinner centred.
- Colour card at both ends and in the middle of every track (knobs whole, not cut off), and for a
  light with only colour temperature or only colour (two cards each). The close and done
  buttons are unchanged; the demo preview hides them.

## Not tested / for the owner

- The final release build by hand on the glass (the owner swiped the earlier builds of this round).
- The grey slider end of a light that is off on a real light (render path only through the palette
  code; the demo lights were on).
- The colour card on a real colour light with Home Assistant commands (renders use the demo
  preview without actions).
- The update from 0.2.35 to 0.2.36 through the app.
