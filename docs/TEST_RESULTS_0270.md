# Test results app 0.2.70 / firmware 0.2.59 (2026-09-17)

Max, after the 0.2.67 update round on his own screens: "ik vind sws alle busy heel lang duren" - every busy state feels
longer than it needs to be. This release makes a tap look instant and keeps the spinner for commands that really take a
while. See CHANGELOG 0.2.70 and the compatibility note in docs/RELEASING.md. Built on 0.2.69 (`45fcbdd`).

## What the old rule did, measured

- `Tile::loading()` held a tile busy for at least 1000 ms (150 ms for a switch) and then, without a confirming state,
  up to 6000 ms. The 1000 ms came from the first runtime tiles; nothing measured it.
- The bench screens' own logs during the 0.2.67 hardware round, against Max's Home Assistant: a tap was answered with a
  new state in 342, 399, 404, 417, 434, 467, 550 and 599 ms. Eight of eight under 600 ms, so the sheet always outlived
  the command.
- `cover.stop_cover` on a cover that already stands still brings no new state at all: Max pressed the middle key in the
  cover card and the sheet stayed the full six seconds. The log shows the command going out at 21:43:31 with no state
  after it.
- Home Assistant itself shows no spinner: `ha-control-switch._toggle()` flips the switch before the command goes out
  (180 ms CSS transition) and the state that follows confirms it.

## Automated

- Python (`.venv-portal`): 386 tests OK, the same set as 0.2.69 (this release is firmware only; the app just raises
  `FIRMWARE_VERSION`).
- C++ (`clang++ -std=c++17 -Wall -Wextra -Werror -I.`): 17/17 PASS. `tests/test_action_feedback.cpp` now holds the new
  rules: nothing drawn in the first 400 ms while a second finger is still ignored, the sheet from 400 ms, the cap at
  3000 ms, an "it worked" answer ending the wait 800 ms later, a tap that opens a card drawing no sheet at all, the
  clock wrapping around, a switch confirming at 250 ms without ever drawing a sheet, and the optimistic stand with its
  undo and a state message overruling it.
- `generate_packages.py --check` and `generate_icons.py --check` green.

## Firmware on the Mac (host builds, driven over the ESPHome API)

`tap_host.py`, now with the busy sheet in its probe (`busy=`) and the probed slot in every line, so the host's
one-to-two-second log lag cannot be read as another card's answer. **Guition 34/34, CYD 34/34.** The eight new checks:

| Check | Guition | CYD |
|---|---|---|
| A tap on a lamp shows the new stand at once | pass | pass |
| and draws no sheet yet | pass | pass |
| The sheet comes when Home Assistant takes longer | pass | pass |
| An answer that brings no new state lets go in a moment | pass | pass |
| and the tile shows Home Assistant's own stand again | pass | pass |
| A command without an answer keeps the sheet | pass | pass |
| and lets go after three seconds | pass | pass |
| A refused switch says Refused, then the old stand is back | pass | pass |

The 26 checks of 0.2.67 (On / off, Perform action with its data, a refusal, holding, an unanswered call, Home
Assistant's words) pass unchanged.

## Max's own screen

The Guition at his desk got firmware 0.2.59 over the air from this tree (`esphome upload`, own name, keys and Wi-Fi;
Home Assistant kept the device and the app reported Synced again within half a minute). His verdict after tapping:
"wauw dit is perfect, werkt veel fijner."

## Builds (ESPHome 2026.6.2)

Check profiles with the generated packages and local components; no errors or compiler warnings.

| Profile | RAM | Flash | vs 0.2.69 |
|---|---|---|---|
| CYD | 23.9% (78,396 B) | 79.2% (1,453,351 B) | +112 B RAM, +1,060 B flash |
| Guition | 25.6% (83,972 B) | 21.0% (1,708,627 B) | +112 B RAM, +1,024 B flash |

The RAM is the answer timestamp and the two optimistic flags per tile, plus one LVGL timer while a command is under way.

## Not tested

- The CYD on real hardware: it kept firmware 0.2.58 until the release, so its Update round follows the push.
- A refusal on Max's own screen: it needs a tile whose action Home Assistant rejects, which the 0.2.67 round already
  showed there ("value must be at most 100 at 'brightness_pct'").
