# Test results app 0.2.54 / firmware 0.2.46 (2026-09-16)

The standby code tidied after a review of Wake and Sleep: one rule for ending a Sleep, standby through
`go_home` / `close_cards`, `close_cards` as one call, the backlight light internal, and a dead
manual-profile branch removed. Storage, tile protocol, preferences and keys are unchanged. See
CHANGELOG 0.2.54 and the compatibility note in docs/RELEASING.md.

## Automated

- Python (`.venv-portal`): 209 tests OK. `tests/test_wake_sleep.py` now has 9 tests, run against both
  board profiles and both generated packages. New: `dim_display` sets `display_dimmed`, runs
  `apply_screen_settings` and then `go_home` or `settings_screen::close()` + `close_cards`, with no
  overlay hiding of its own; it has exactly two callers, and the standby interval only calls it with
  Auto standby on. `close_cards` is the single `runtime_tiles::dismiss()` call, and that callback
  hides the detail card, clears `active_entity` and hides the four overlays. `sleep_requested` is
  cleared in exactly one place (`wake_display`) and the Auto standby switch no longer mentions it.
  The `back_light` light is `internal: true`.
- C++ (`clang++ -std=c++17 -Wall -Wextra -Werror -I.`): 14/14 PASS (no C++ changed).
- `generate_packages.py --check` green.
- Home Assistant (read-only `search/related`): no automation, script or scene uses
  `light.studio_1_display_backlight`, the only backlight light in Max's Home Assistant.

## Builds (ESPHome 2026.6.2)

Both Easy Setup profiles, compiled one after the other from packages generated out of the board profiles:

| Profile | RAM | Flash | vs 0.2.53 |
|---|---|---|---|
| easy-cyd-device.yaml (package, local components) | 23.0% (75,468 B) | 76.1% (1,395,683 B) | -144 B RAM, -744 B flash |
| easy-guition-device.yaml (package, local components) | 24.0% (78,692 B) | 18.4% (1,495,307 B) | -152 B RAM, -764 B flash |

## Behaviour on the real profile logic (host build)

Same route as 0.2.53: the generated packages built for the Mac with ESPHome's host platform and driven
over the native API, now with the probe also reporting the settings page, the climate overlay and
`active_entity`, plus test-only actions that open a card and flip "also on standby". Result: 35/35 checks passed on the Guition package and 35/35 on the CYD package (the CYD still lists its RGB `LED` light, as intended).

Checked on both boards, besides everything from 0.2.53:

1. The backlight light is not among the entities Home Assistant gets.
2. With "also on standby" on and off: Sleep closes an open settings page and an open card (overlay
   hidden, `active_entity` empty); the standby time closes an open card too.
3. The standby time dims with Auto standby on; switching Auto standby off wakes that screen.
4. Sleep, then switching Auto standby from on to off: the screen stays asleep at the standby level;
   a tap on the overlay wakes it.

## Not tested / for the owner

- Hardware. Nothing was flashed: the bench boards (192.168.146.134 / .136) were unreachable, and Studio 1
  was busy with another session's touch log over USB. On real glass after the update: Sleep with a card
  open, and whether Home Assistant shows the old backlight light as unavailable (delete it there).
