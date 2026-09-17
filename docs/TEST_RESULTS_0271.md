# Test results app 0.2.71 / firmware 0.2.60 (2026-09-17)

Max, dragging the small slider of his living room light group on the Guition: after letting go the slider "sprints
back" to the old value and then creeps to the one he chose. The lamps fade, Home Assistant reports every step, and the
tile followed each report. This release keeps the slider where the finger left it while the light gets there. See
CHANGELOG 0.2.71 and the compatibility note in docs/RELEASING.md. Built on 0.2.70 (`91a2445`).

## What happened before

- The tile's small slider (`runtime_tiles.h`, the mini slider in `render`) took `slider_value(t)` on every state
  message once the finger was up. The card's control slider already held its dragged value while the command was
  pending, but only until the first differing state, which for a fading group is a step on the way.
- Home Assistant's own tile slider does the same: `ha-control-slider` follows the entity's state on every update, so
  the jump shows there too. Nothing in Home Assistant holds the target.

## Automated

- Python (`.venv-portal`): 386 tests OK, the same set as 0.2.70 (firmware only; the app raises `FIRMWARE_VERSION`).
- C++ (`clang++ -std=c++17 -Wall -Wextra -Werror -I.`): 17/17 PASS. `tests/test_action_feedback.cpp` holds the new
  rules: a fade reporting 80, 128 and 180 keeps the sent 204 in front and the hold alive, a report within 3 % ends it,
  a fan that reports 66 once and stays quiet lets go after 1.5 s, no report at all ends with the wait at 3 s (or the
  8 s cap after an "it worked" answer), a drag on an off light shows it on and a report of off wins, a refusal puts
  the reported value back, and a cover never holds.
- `generate_packages.py --check` and `generate_icons.py --check` green.

## Firmware on the Mac (host builds, driven over the ESPHome API)

`slider_host.py` (scratchpad, after `tap_host.py`): the tap-actions demo home on 8297, the cooker hood light with its
small slider, a `drag_slider` action that sets the LVGL slider and sends its release as a finger would, and a probe
that logs the slider position, the tile's value label, the hold flag and the busy sheet. **Guition 17/17, CYD 17/17.**

| Check | Guition | CYD |
|---|---|---|
| The hood tile shows its small slider at 20 % | pass | pass |
| A drag to 80 % sends light.turn_on with brightness 204 | pass | pass |
| The slider stays at 80 % and the tile says 80 % | pass | pass |
| Reports of 80, 120 and 170 keep the slider at 80 %, no sheet | pass | pass |
| A report of 200 (within 3 %) takes over: 78 % | pass | pass |
| A report short of the target keeps the slider | pass | pass |
| After 1.5 s of quiet the slider follows that report | pass | pass |
| A drag without any report holds first | pass | pass |
| and lets go after the wait, showing the last report | pass | pass |
| A refused drag says Refused, then the last report shows again | pass | pass |
| A drag on an off light shows it on at 60 % | pass | pass |
| Home Assistant reporting it off wins | pass | pass |
| No errors in the log | pass | pass |

The first Guition run read two of these wrong: the host's LVGL loop stalled and answered five probes in one burst, so
a wait counted in tries ran out before the redraw. The harness now waits by the clock; the second run and the CYD run
passed every check.

## Builds (ESPHome 2026.6.2)

Check profiles with the generated packages and local components; no errors or compiler warnings.

| Profile | RAM | Flash | vs 0.2.70 |
|---|---|---|---|
| CYD | 24.0% (78,796 B) | 79.2% (1,453,683 B) | +400 B RAM, +332 B flash |
| Guition | 25.7% (84,372 B) | 21.0% (1,708,967 B) | +400 B RAM, +340 B flash |

The RAM is four fields per tile (the sent value, the reported value and two timestamps).

## Not tested

- Real hardware: neither bench screen ran this firmware before the release. Max's living room group on the Guition
  is the case that started this; the Update round on his screens follows the push.
- A fan with fixed steps and a media player's volume were only covered by the C++ test, not on the host.
