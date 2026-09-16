# Test results app 0.2.52 / firmware 0.2.44 (2026-09-16)

The settings page on the screen itself, the `screen.settings` card, and "back to page 1 by itself".
Firmware and app both change; the protocol grows two optional keys and the stored layouts gain two
settings with defaults, so an old screen with the new app and a new screen with an old app both keep
working. See CHANGELOG 0.2.52 and docs/SETTINGS.md.

## Automated

- Python (`.venv-portal`): 200 tests OK. New `tests/test_settings_page.py` (7 tests): every key the
  firmware reports back (`changed("...")` in settings_screen.h) is a key the add-on validates; the
  `settings` block the screen receives still holds exactly its eleven frozen keys while `auto_home`
  and `auto_home_seconds` ride beside it; the defaults and the refusals of the two new settings;
  `screen.settings` as a built-in tile that asks for firmware 0.2.44; and both board profiles carrying
  the page, the hold strip (clear of the Guition's swipe bands), the `open_settings` action and the
  `go_home` script.
- C++ (`clang++ -std=c++17 -Wall -Wextra -Werror -I.`): 14/14 PASS, including new
  `tests/test_settings_screen.cpp`: the duration ladder and its symmetry, a number stopping at its
  ends, a time wrapping around midnight and walking whole hours while a key is held, every text
  ("45 sec", "1 h 30", "22:00", "10:00 PM"), the rows reading and writing the real settings (including
  a lower Brightness pulling the two dim levels down), a row hidden on a board that cannot turn, the
  table itself (every menu row opens a real page, every control has a reader and a writer), how many
  rows fit per board and when the pager appears, and that one change stores, applies and reports once.
- `generate_packages.py --check` green, `generate_icons.py --check` green (183 glyphs: three new).
- `esphome config` green for both board profiles.

## Builds (ESPHome 2026.6.2)

Full board profiles compiled for the ESP32, not just the host:

| Profile | RAM | Flash |
|---|---|---|
| home-like-2432s028.yaml (CYD) | 23.0% (75,492 B) | 80.6% (1,479,167 B of 1,835,008) |
| guition-4848s040.yaml | 24.0% (78,732 B) | 19.4% (1,576,143 B of 8,126,464) |
| easy-cyd-device.yaml (package, local components) | 23.0% (75,276 B) | 76.0% (1,395,143 B) |
| easy-guition-device.yaml (package, local components) | 24.0% (78,508 B) | 18.4% (1,494,075 B) |

The two Easy profiles were compiled one after the other, as RELEASING asks, from packages generated
out of the board profiles above.

What the page costs, measured by building the same CYD profile at 0.2.51 (firmware 0.2.43) in a clean
worktree: **+608 bytes of RAM** (74,884 -> 75,492) and **+10.7 KB of flash** (1,468,191 -> 1,479,167).
The rows are a `constexpr` table in flash and the LVGL objects are built when the page opens and
deleted when it closes, like the cards of a tile, so a closed page costs only that handful of bytes.

The ESP32 build also caught what the Mac could not: on xtensa `int32_t` is `long`, so a row reader
that returns a plain `int` does not convert to `Read` at all. Every reader now says `-> int32_t`.

## Rendered on the real firmware (host build, SDL)

`.esphome/readme-render/host_build.py <board> --compile` builds the actual firmware for the Mac and
`render_settings.py <board>` drives it over its own API. Both boards were rendered and looked at:

- Every page on both boards: the menu, Brightness, Night, Screen, This screen.
- Holding the top bar: the blue line is halfway after 0.9 s of a 1.5 s hold and disappears on release.
- The time picker worked like a finger: four taps on `+` moved 22:00 to 23:00 (quarters), then holding
  walked whole hours to 03:00. Switching the clock to 12 hour turned the same times into 3:00 AM and
  7:00 AM.
- A switch row turning its dependent rows live and grey again (Night mode, Auto standby).
- The `screen.settings` card on the overview, and a tap on it opening the menu.
- "Back to page 1 by itself": with the last touch aged past the timeout, the open page closed and the
  overview came back on page 1.

## Not tested / for the owner

- Hardware. Nothing was flashed: the hold gesture on real glass (resistive CYD and capacitive Guition),
  whether 1.5 s feels right, and whether a hold that becomes a swipe cancels cleanly.
- The write-back to a running ESP Screens (`esphome.screen_setting` event), which the firmware already
  used for the Home Assistant number entities and which this page reuses unchanged.
- `Restart` from the page, and `esphome.<screen>_open_settings` from Home Assistant.
