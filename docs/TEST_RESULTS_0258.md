# Test results app 0.2.58 / firmware 0.2.50 (2026-09-16)

Max asked for a detail card for blinds (his three Motionblinds venetian blinds report every cover feature and a
battery sensor on their device) in the style of the climate and vacuum cards. Checking Studio 1 twenty minutes
after its update to 0.2.49 found two bugs, and opening ESP Screens during that update a third. See CHANGELOG
0.2.58 and the compatibility note in docs/RELEASING.md.

## Automated

- Python (`.venv-portal`): 254 tests OK. New: `tests/test_cover_card.py` (the tilt and the device battery in
  the state message, the battery sensor watched, no layout sensor or node name "unavailable" while a screen
  restarts, the tap and preview routing to the cover card, commits on release, the tick's status line,
  disabled keys kept out of the tick, one radius for track and fill, every setting number published when it
  has no state), `tests/test_editor_startup.py` (the first full inventory redraws the tiles, top bar and
  entity list) and `tests/test_page_assets.py` (script and styles by content stamp).
- C++ (`clang++ -std=c++17 -Wall -Wextra -Werror -I. -I components/smart_display`): 14/14 PASS.
  `test_tile_controls` adds the cover card: what a venetian blind (255), curtains (15), a garage door (11),
  slats that tilt without a position (51) and a cover without features get, the status line with the tilt, keys filled while
  moving and disabled at the ends, and the tilt actions.
- `generate_packages.py --check` green.

## Builds (ESPHome 2026.6.2)

Check profiles with the generated packages and local components, the same as for 0.2.57.

| Profile | RAM | Flash | vs 0.2.57 |
|---|---|---|---|
| CYD | 23.4% (76,548 B) | 76.9% (1,411,823 B) | +8 B RAM, +5,596 B flash |
| Guition | 24.4% (79,924 B) | 18.6% (1,515,263 B) | +8 B RAM, +4,708 B flash |

The first ESP32 build failed on `std::clamp(lv_slider_get_value(...), 0, 1000)`: `int32_t` is a `long` on the
ESP32 toolchain, so the host builds cannot catch this. Fixed with `std::clamp<int>` before any release.

## Firmware on the Mac (host builds, driven over the ESPHome API)

- Cover card, Guition 20/20 and CYD 20/20 (the CYD run three times): a venetian blind with the demo home's
  state opens its card with "Open · 60% · Tilt 40%", both sliders and three keys; the position slider shows
  75% while it moves and sends `cover.set_cover_position` 75 on release, the tilt slider
  `cover.set_cover_tilt_position` 80; open, stop and close send their actions; a closing state at 30 % redraws
  the card with the close key filled, and the tick keeps the status line; fully open disables the open key
  after the tick and a tap on it sends nothing; unavailable disables sliders and keys; curtains get one slider,
  a garage door only keys with close disabled while closed, and its open key works; a tap on the blind's tile
  opens the card; no errors in the log.
- A timing flake in the first CYD run (a key pressed 1.2 s after Home Assistant's answer, while the card's
  once-a-second tick still showed the keys disabled) was the test waiting too briefly; the card behaves as the
  vacuum card does.
- Renders of the blind, curtains and garage door cards on both boards; the README shows them.

## Studio 1 after updating to 0.2.57 / firmware 0.2.49 (16:20, read-only through Home Assistant)

- The update succeeded ("Updated to firmware 0.2.49"): one restart at 16:10, back online within seconds on the
  same IP address, "Synced" within a second of reconnecting, no repairs, no add-on warnings; Home Assistant
  lists `esphome.studio_1_screen_message` as answering.
- Every setting entity had its value except `number.studio_1_night_brightness` ("unknown", so the panel showed
  it as unknown and refused changes): the number fix above.
- `sensor.esp_screens_unavailable` next to `sensor.esp_screens_studio_1`, written at the restart: the node fix
  above. Home Assistant drops the stale sensor at its next restart.
- An empty Screen settings panel in Safari was an old cached script (the served files matched the release);
  the page now loads its files by content stamp.

## Not tested / for the owner

- Real hardware and real blinds: the Motionblinds were unavailable during this work. On Studio 1 after the
  update: open a blind's card, drag both sliders and use the keys, check the battery, and that
  `number.studio_1_night_brightness` has a value in Home Assistant.
- Free heap on a CYD with the cover card open.
