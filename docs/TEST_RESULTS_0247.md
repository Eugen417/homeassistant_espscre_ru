# Test results app 0.2.47 / firmware 0.2.40 (2026-09-15)

The new climate card on the Guition with fan and swing on a card of their own, and no single mode
key on either board. See CHANGELOG 0.2.47 and RELEASING "Compatibility 0.2.47".

## Automated

- Python (`.venv-portal`): 170 tests OK (the icon tests cover the new `arrow-oscillating` glyph).
- C++ (`clang++ -std=c++17 -Wall -Wextra -Werror -I . -I components/smart_display`): 13/13 PASS.
  `climate_card.h` draws LVGL objects and is covered by the host renders below, not by a unit test.
- `generate_packages.py --check` green, `generate_icons.py --check` verified (150 pickable icons,
  180 glyphs).
- ESPHome 2026.6.2 compile SUCCESS, one after another: `easy-guition-device.yaml` (RAM 32.9%, flash
  18.9%), `easy-cyd-device.yaml` (RAM 32.0%, flash 78.5%), `guition-device.yaml` (RAM 33.0%, flash 20.0%)
  and `device.yaml` (RAM 32.0%, flash 83.3%). Packages carry `SCREEN_FIRMWARE_VERSION` 0.2.40.

## Bench boards (OTA)

- Both Easy Setup builds uploaded over the network; `device_info` reports project 0.2.40, compiled
  2026-09-15 15:42:18 (Guition) and 15:44:03 (CYD), matching the build logs. ESP Screens reports both
  screens Synced on 0.2.40.
- `preview_runtime_card` opened the climate card of the owner's `climate.airco_remeha_diva_ac` on
  the Guition with live data from app 0.2.46. The capture (`capture_ui.py`) shows Off with a grey 19°,
  the five mode keys (heat/cool, heat, cool, dry, fan only), Fan with Low chosen and Swing with Off
  chosen. The vacuum card of 0.2.46 is unchanged.

## Renders (ESPHome host + SDL, both boards, this release's code)

- Guition climate card in four cases: heating with three modes and a fan row, an air conditioner with
  five modes, fan and swing (the owner's attributes), the same with two swing modes, and a heat-only
  thermostat without fan or swing (no mode keys, setpoint centred). Nothing runs off the screen; the
  card with two rows ends 10 px above the bottom edge.
- CYD climate card: unchanged apart from the heat-only thermostat, which now shows the centred setpoint
  without a mode key.
- `docs/images/guition-climate.png` comes from the air conditioner render.

## Not tested / for the owner

- The Guition climate card by hand: − / + taps and holding, mode keys, fan and swing choices with the
  real air conditioner (tapping sends real commands).
