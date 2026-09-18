# Test results app 0.2.85 / firmware 0.2.71 (2026-09-18)

Max noticed that the airco tile always looked on, also with the airco off, and asked whether other tiles did the same.
They did. The firmware had its own short list of what turns grey (switches, lights, binary sensors, persons, timers),
and gave an airco that is off Home Assistant's orange on the belief that Home Assistant draws it so. It does not: its
frontend has no colour for a climate that is off, so it falls back to the inactive grey. Max chose, on two questions:
only tiles that can be off light up over a whole page, and all four groups of Home Assistant's colours come along
(alarm sensors, batteries, running scripts/timers/cameras, weather and sun).

## Sources

Read from Home Assistant's frontend (dev branch on GitHub) and checked against the CSS variables and `const.ts` module
his own Home Assistant 2026.9 serves: `src/common/entity/state_active.ts` (what is active per domain),
`src/common/entity/state_color.ts` (the variable order `--state-<domain>-<device class>-<state>-color`,
`--state-<domain>-<state>-color`, `--state-<domain>-active|inactive-color`, `--state-active|inactive-color`),
`src/common/entity/color/battery_color.ts` (70 / 30), `src/panels/lovelace/cards/hui-tile-card.ts` (inactive grey,
a person's colour on its badge), `src/state-display/state-display.ts` (a climate tile shows its state and the current
temperature) and the `TIMESTAMP_STATE_DOMAINS` set (scene, button, image: active unless unavailable).

## What changed

- Firmware: `Tile::active()` is a port of `stateActive()` for the domains a tile can show; `Tile::lights_up()` limits
  the full-page glow to tiles that can be off; `Tile::slider_active()` is `active()` plus the cover exception.
  `tile_controls::accent()` replaces `runtime_tiles::domain_accent()` and follows `stateColorCss()` (alarm classes red,
  batteries by charge, script/timer/camera amber, weather per condition, sun amber/indigo, a person in a zone blue,
  a climate mode without a colour amber), keeping our own colour per kind for scenes, selects, numbers and sensors.
  `render_slot` greys every tile that is not active, lights up only `lights_up()` tiles, keeps a closed blind's
  slider coloured, and an airco that is off reads "Off" (with " · 21.5°" when it reports a current temperature).
  `theme.h` gained Home Assistant's light grey, blue grey, yellow, lime and the snowy blue.
- App: `header_bar.ALARM_CLASSES` is Home Assistant's list of eleven red classes (battery, heat, lock and sound were
  missing), so the top bar and the tiles agree.
- Tests: `tests/test_runtime_model.cpp` (58 state cases for `active()` and `lights_up()`, built-in cards, nothing
  received, the closed blind), `tests/test_tile_controls.cpp` (every colour rule, all weather conditions, battery
  bounds 69.5/70 and 29.9/30), `tests/test_state_colours.py` (the palette values against Home Assistant's, the alarm
  list in firmware, top bar and Home Assistant, a colour for every weather condition the add-on knows),
  `tests/test_off_look.py` and `tests/test_theme.py` follow the new lines.

## Automated

- `tools/check.sh`: Python 471 tests, C++ 19/19, packages, icons, Vitest, vue-tsc, build and the bundle: all pass.
- `tools/check.sh --firmware --baseline 1676912` (ESPHome 2026.6.2): CYD 1,677,488 B = 91.4 % of the 1,835,008 B
  slot, +576 B against 0.2.84 (tight band, well under the 8 KB rule); Guition 2,054,640 B = 25.3 % of its 8,126,464 B slot.

## On the screens

Max asked to skip a host build and check on the two screens that already run on his Home Assistant. After the
release both get this firmware over the air from the YAML his ESPHome Device Builder holds for them (packages from
GitHub main, the same route an Update in ESP Screens takes), with keys and touch calibration checked equal first.
Their tiles cover most of the change: the airco (off), scripts that are not running, the robot in its dock, lights
that are off, a battery sensor at 100 %, a temperature sensor, a playing Apple TV, a plug sensor that is on, scenes,
a button and an input boolean. What they show is reported to Max for his own look.

## Not tested

- Weather, sun, a person in a zone, a camera, a paused timer, a closed or open blind and the alarm classes on real
  hardware: none of Max's screens shows them now. The unit tests cover every rule.
- The editor's mockup still draws icons in one colour; it never showed state colours.
