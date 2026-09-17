# Test results app 0.2.62 / firmware 0.2.53 (2026-09-17)

A binary sensor tile read "On" or "Off" where Home Assistant says "Open" or "Closed", and a user (Remco) asked for a
different icon for lights that are off. Max asked to do it the way Home Assistant's web interface does. See CHANGELOG
0.2.62 and the compatibility note in docs/RELEASING.md.

## Home Assistant's own behaviour

- Source, `dev` branch: `homeassistant/components/light/icons.json` gives `mdi:lightbulb`, and `mdi:lightbulb-off`
  for state off (switch, fan and input_boolean have the same pattern); the frontend colours active lights
  `--state-light-active-color` (amber) and anything off `--state-inactive-color` (grey).
- Live on Max's dashboard: lights that are on have an amber icon on a pastel amber circle; lights that are off are grey,
  with a crossed-out icon where the icon has one; a light with its own icon (a mirror) keeps it and turns grey.

## Automated

- Python (`.venv-portal`): all tests OK. New `tests/test_binary_words.py` (5): the firmware's `BINARY_WORDS` equals
  `header_bar.BINARY_STATES`; the top bar and the history card give the same words for all 28 classes, and On/Off
  without a class or with an unknown one; every state message already carries `device_class` (no `x`); the tile, the
  card and the history heading use the words for binary sensors only, and the heading only takes its own entity's
  history words; all five text fonts of both profiles and packages carry every letter. New `tests/test_off_look.py`
  (3): `lightbulb-off` (F0E4F) in the fonts of both profiles and packages; the firmware's off bulb and grey rule; the
  top bar's off bulb, a light's own icon kept, no accent while off.
- C++ (`clang++ -std=c++17 -Wall -Wextra -Werror -I. -I components/smart_display`): 15/15 PASS;
  `test_tile_controls` checks door, motion, moisture, battery charging, battery, no class, an unknown class and a
  wrongly cased class.
- `generate_packages.py --check` and `generate_icons.py --check` (150 pickable icons, 184 glyphs) green.

## Builds (ESPHome 2026.6.2)

Check profiles with the generated packages and local components, as for 0.2.59. No warnings from `runtime_tiles.h`
or `tile_controls.h`.

| Profile | RAM | Flash | vs 0.2.59 |
|---|---|---|---|
| CYD | 23.5% (77,060 B) | 77.8% (1,428,127 B) | +0 B RAM, +1,112 B flash (0.2.60 and 0.2.61 included) |
| Guition | 24.5% (80,436 B) | 18.9% (1,531,911 B) | +0 B RAM, +1,580 B flash (0.2.60 and 0.2.61 included) |

## Firmware on the Mac (host builds, driven over the ESPHome API)

A demo home with fifteen binary sensors of every kind, lights on and off, and the front door and a light in the top
bar (`.esphome/readme-render/binary/`), with a probe that logs each card's text and colours:

- Before (0.2.52 code): every binary sensor read On or Off in amber, while the top bar said Open and Motion; the door
  card said "On" until its history arrived and then "Open"; the motion card opened after the door card said "Open".
- After, Guition and CYD: Open, Closed, Motion, Dry, Smoke, Disconnected, Not charging, Low, Problem, Up to date,
  Plugged in (also as a large value); a sensor without a class and an unknown class still say On. The door card says
  "Open" while loading and after the answer, and "Closed" once the door closes with the card open; the motion card says
  "Motion" while loading. The longest words fit a single CYD tile.
- Off look (Guition): an off light without an icon shows a grey crossed-out bulb, an off light with a chosen lamp icon
  a grey lamp, a closed door and a dry leak sensor grey circles; the open door, motion and a light at 50 % stay amber;
  the top bar shows the crossed-out bulb for the off light.

## Not tested

- Physical screens: not flashed in this round.
