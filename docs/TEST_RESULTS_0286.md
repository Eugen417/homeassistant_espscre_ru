# Test results app 0.2.86 / firmware 0.2.72 (2026-09-19)

Max reported that a tap on the on/off switch at the right of a wide tile went "from off to on to off in one click",
as if it registered a double click. A tap on the tile itself worked. The tile was `light.badkamer_3` on his Guition
Wallbox, a wide tile with the control *On/off switch*.

## Cause

No double tap: the switch sent the opposite of what it showed. `runtime_tiles::control_event` did two things in the
wrong order since firmware 0.2.59 (app 0.2.70, the "tap looks instant" round):

1. `Tile::optimistic(t.state != "on")` draws the new position at once. Since 0.2.59 it writes that position into
   `Tile::state` (before, a separate `optimistic_on` field held it and `state` stayed as it was).
2. `tile_controls::key_action(t, TOGGLE)` then chose `turn_off` when `state` was "on", which it now always was for a
   light that had been off.

So an off light got `light.turn_off`. Home Assistant answered "it worked" without a new state, the wait ended 800 ms
after that answer, `end_wait` undid the optimistic position, and the knob slid back to off. An on light got
`light.turn_on` the same way. The other three callers of `optimistic()` (the tile's own tap, the light card's switch,
a slider on an off light) choose their action before they call it, or don't read `state` afterwards.

Evidence before the fix:

- The two lines of `control_event`, run against the released headers (origin/main 311fa2a), print
  `light off -> knob shows on, sends light.turn_off` and `light on -> knob shows off, sends light.turn_on`.
- Home Assistant's logbook for `light.badkamer_3` only had `light.toggle` state changes from the screen (taps on
  the tile). A `turn_off` sent to a light that is off changes nothing, so it leaves no line in the logbook.

## What changed

- `tile_controls::press_key(Tile &, command, arg)`: chooses the action with `key_action` first, then shows a toggle's
  new position with `optimistic()`, and only when there is an action to send.
- `runtime_tiles::control_event` calls `press_key` instead of the two lines.
- `tests/test_tile_controls.cpp`: an off light sends `light.turn_on` and shows on, and undoing that brings back off;
  an on fan sends `fan.turn_off` and shows off; a toggle on a sensor sends nothing and leaves its state alone; a
  cover's open key leaves the state alone.

## Automated

- `tools/check.sh`: Python 471 tests, C++ 19/19, packages, icons, Vitest, vue-tsc, the editor build and the bundle
  in Git: all pass.
- `tools/check.sh --firmware --baseline 1677488` (ESPHome 2026.6.2): CYD 1,677,504 B = 91.4 % of the 1,835,008 B
  slot, +16 B against 0.2.85 (tight band, well under the 8 KB rule); Guition 2,054,640 B = 25.3 %, the same as 0.2.85.
  Built before the version bump, which only changes the version text (same length).

## On the screen

With Max's OK the fix went over the air to his Guition Wallbox (192.168.146.136). The YAML was the one his ESPHome
Device Builder holds for it, with the packages and components of this branch instead of GitHub. That build still says
0.2.71, so ESP Screens offers the released 0.2.72 afterwards. `device_info` read back the build's compile time
(2026-09-19 08:10:24).

Max then tapped the switch and the tile, and the screen's log and Home Assistant's logbook agree on every tap:

| Time | Where | Sent | Light after |
|---|---|---|---|
| 08:13:12.9 | switch (light on) | `light.turn_off` | off |
| 08:13:14.5 | switch | `light.turn_on` | on |
| 08:13:15.8 | switch | `light.turn_off` | off |
| 08:13:16.5 | tile | `light.toggle` | on |
| 08:13:17.1 | tile | `light.toggle` | off |
| 08:13:17.6 | switch | `light.turn_on` | on |
| 08:13:19.2 | switch | `light.turn_off` | off |

Home Assistant reported each new state 220-430 ms after the send. One tap at 08:13:18.1, 526 ms after the one
before, was dropped by the 600 ms same-control debounce (`TouchGuard::accept`), as before. Max: "werkt super!".

## Not tested

- The switch of a switch, an input boolean or a fan on real hardware: they go through the same `press_key` line
  and the unit tests.
- The CYD: the fix is the same shared code. Its screen has no tile with a switch now, and it gets 0.2.72 through
  **Update**.
