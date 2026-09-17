# Test results app 0.2.60 / firmware 0.2.52 (2026-09-17)

Max saw grey sliders for a playing media player's volume and an open cover's position. Since 0.2.42 (a grey slider
end for a light that is off) a slider showed the tile's colour only while `Tile::active()` held, and that rule knows
`on`, `cleaning`, `active`, home, above the horizon and climate not off: never `playing` or `open`, and never a number.
Home Assistant colours tile sliders by `stateActive()` (frontend `src/common/entity/state_active.ts`, read on GitHub):
a media player is active unless off or standby, a cover unless closed, and its cover position feature keeps the open
colour for a closed cover so the slider does not look disabled. Every caller of `active()` was checked (the grey circle
of switches, people and timers, a light's own colour, the palette cache, both sliders, and `was_on` of the value
overlay in both profiles, which covers and media players no longer reach; the top bar and cards don't use it).
Widening `active()` would only have changed slider colours, but the closed cover needs a slider rule anyway, so the fix
is a separate `Tile::slider_active()` and `active()` stays as it was. See CHANGELOG 0.2.60 and docs/RELEASING.md.

## Automated

- C++ (`clang++ -std=c++17 -Wall -Wextra -Werror -I. -I components/smart_display`): 15/15 PASS, on HEAD plus only this
  change in a separate worktree. `test_runtime_model` adds `test_slider_colours`: a light and a fan on and off; a media
  player playing, paused, idle, buffering and on (coloured) against off, standby, unavailable and unknown (grey); a cover
  open, opening, closing and closed (coloured) against unavailable; a number and an input_number with a value against
  unknown; a tile that has received nothing. The test fails on the old rule (compiled against
  `slider_active() { return active(); }`).
- Python (`.venv-portal`): 282 tests OK in that worktree, with the version bump. `generate_packages.py --check` green.

## Builds (ESPHome 2026.6.2)

ESP32 check profiles with the generated packages and local components. They were built from the shared working tree,
which also held other sessions' unfinished edits; the version bump alone was not built again.

| Profile | RAM | Flash |
|---|---|---|
| CYD | 23.5% (77,060 B) | 77.8% (1,427,919 B) |
| Guition | 24.5% (80,436 B) | 18.8% (1,531,471 B) |

RAM is the same as in 0.2.59.

## Firmware on the Mac (host builds)

Guition and CYD host builds with a demo home that runs the real manager, on three pages: a playing Sonos (volume), an
open blind (position) and a boiler setpoint (slider) on double-width tiles; a radio in standby, a closed blind and a
paused speaker; small sliders for an idle speaker, a TV that is off, an open blind, a number, a lamp that is on and a
hallway light that is off. Playing, paused and idle players show blue, blinds purple (the closed one as the short end
at 0 %), numbers teal and the lamp amber; the radio in standby and the TV stay grey, and the light that is off shows
its empty grey track as before. No compiler warnings from the smart_display headers.

## Not tested / for the owner

- Real hardware: nothing was flashed. After updating a screen, a playing speaker's volume slider and an open blind's
  position slider are coloured, and the speaker's slider turns grey in standby.
- A media player that is off or in standby still shows its grey fill with the handle; Home Assistant hides the handle
  there. Left as it is.
