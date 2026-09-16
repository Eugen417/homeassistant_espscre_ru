# Test results app 0.2.56 / firmware 0.2.48 (2026-09-16)

Max found that the back arrow on the colour card of Studio 1 (firmware 0.2.44) did not respond, while the
card itself kept working. Cause: `settings_screen::attach_hold` (firmware 0.2.44+) creates the transparent
hold strip for the settings page in `on_boot`, after the YAML cards on `home_page`. It therefore lay on top of
them, and LVGL's hit test gave taps in the top bar region (x 32-448, y 0-72 on the Guition) to the strip
instead of the card's back button (x 16-76, y 16-76) or its action at the top right. See CHANGELOG 0.2.56
and the compatibility note in docs/RELEASING.md.

## Automated

- Python (`.venv-portal`): 215 tests OK. `tests/test_settings_page.py` adds
  `test_the_hold_strip_lies_under_every_card`: `attach_hold` moves the strip under `below`, both profiles pass
  `id(brightness_overlay)`, it is the first card on `home_page` (before colour, climate, climate mode and the
  dim wake overlay), and the page controls come before it.
- C++ (`clang++ -std=c++17 -Wall -Wextra -Werror -I. -I components/smart_display`): 14/14 PASS.
- `generate_packages.py --check` green.
- LVGL 9.5 `lv_obj_move_to_index` read in the bundled source: moving the strip and then the fill line to the
  index of `brightness_overlay` leaves strip < line < every card in the child order, and the hit test searches
  the last child first.

## Builds (ESPHome 2026.6.2)

| Profile | RAM | Flash | vs 0.2.55 |
|---|---|---|---|
| easy-guition-device.yaml | 24.0% (78,732 B) | 18.4% (1,496,815 B) | +0 B RAM, +40 B flash |
| easy-cyd-device.yaml | 23.0% (75,484 B) | 76.1% (1,396,847 B) | +0 B RAM, +48 B flash |

## Not tested / for the owner

- Hardware and a real LVGL hit test. Nothing was flashed. Studio 1 updates through Home Assistant. On real
  glass after the update: the back arrow on the brightness, colour and climate cards, the button at the top
  right of the climate card, and holding the top bar on a tile page still opens the settings page.
