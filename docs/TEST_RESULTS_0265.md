# Test results app 0.2.65 / firmware 0.2.56 (2026-09-17)

Two requests from Max. The Wake button in Home Assistant should only bring back the light (and the second hand of an
analog clock), not postpone Back to page 1. And ESP Screens should open without choosing a screen by itself. See
CHANGELOG 0.2.65 and the compatibility note in docs/RELEASING.md. Built on 0.2.64 (`e68162b`).

## Cause

- Wake (0.2.53) was designed as "a tap": on a dimmed screen it ran `wake_display`, on a screen that was on it set
  `last_touch_ms`. That one clock also drove the Back to page 1 check in the one-second interval, so every Wake started
  the two minutes again. An automation pressing Wake on every motion kept an open card or page 2 up as long as someone
  moved in the room. The same held for an alert ending and for Auto standby switched on.
- `refresh()` and `applyLive()` in `static/app.js` selected `inventory.screens[0]` whenever nothing was selected, since
  the first version of the editor.

## Automated

- Python (`.venv-portal`): 314 tests OK (312 on 0.2.64). `tests/test_wake_sleep.py`: `test_wake_is_a_tap` became
  `test_wake_is_light_not_a_tap` (Wake runs `wake_display` and then `back_to_page_1_when_due`, and neither touches
  `last_use_ms`), and the new `test_back_to_page_1_counts_from_the_last_touch` checks in both profiles and both packages
  that the rule reads `last_use_ms` only, runs from the interval and Wake only, and that exactly the boot, the three
  touchscreen triggers, `open_settings` and the UI self test write that clock, while `alert_dismiss` and Auto standby
  write only `last_touch_ms`. `tests/test_editor_startup.py`: no `select()` besides the screen button, the
  "Choose a screen" card and the install card start hidden and follow the selection, and the 0.2.58 catalogue redraw
  stays.
- C++ (`clang++ -std=c++17 -Wall -Wextra -Werror -I.`): 16/16 PASS.
- `generate_packages.py --check` green.

## Firmware on the Mac (host builds, driven over the ESPHome API)

The 0.2.53 Wake/Sleep harness on host builds of both packages (own device names and API ports), with a probe that
also logs `tile_page`, the age of `last_use_ms`, `runtime_tiles::awake()` (what the second hand follows) and the Back
to page 1 setting. **Guition 48/48, CYD 48/48.** The 35 earlier checks (standby timer, Sleep with Auto standby off
across a real minute tick, alert, calibration) all still pass. The 13 new ones:

| Check | Result on both boards |
|---|---|
| Back to page 1 on at its default | on, 120 s |
| Wake on page 2, touch 100 s ago | standby time restarts (`idle_ms` < 3 s), page-1 clock keeps its 100 s |
| Wake again with the page-1 clock at 125 s | page 1 within the next interval pass, screen stays on |
| A card open with the clock at 125 s, Wake pressed three times | the card closes anyway |
| Sleep on page 2 with "Also on standby" off | dimmed, page 2 kept, `awake()` false |
| Page-1 clock at 300 s while in standby | still page 2: the rule waits for the light |
| Wake after that | in the same pass: bright, page 1, `awake()` true (the probe right after it logs `idle_ms=0`) |
| Sleep and Wake with 30 s on the clock | page 2 stays |
| `open_settings` from Home Assistant with the clock at 300 s | the page stays open, clock restarted |
| `open_settings` -1 | closes, page 1 |
| An alert on page 2 with the clock at 300 s | page 2 kept while the alert is up |
| The alert dismissed | page 1, standby time restarted |

A real touch cannot be injected into the host's SDL touchscreen; the profile test covers the three touchscreen
triggers that write the page-1 clock.

## Builds (ESPHome 2026.6.2)

Check profiles with the generated packages and local components, as for 0.2.64. Both compile without errors.

| Profile | RAM | Flash | vs 0.2.64 |
|---|---|---|---|
| CYD | 23.7% (77,572 B) | 77.5% (1,421,263 B) | +72 B RAM, +272 B flash |
| Guition | 24.7% (80,932 B) | 18.8% (1,524,939 B) | +80 B RAM, +292 B flash |

(The 0.2.64 numbers come from that release's check builds, whose device names were five characters shorter.)

## ESP Screens

The real add-on from this tree on the demo home (two screens, fake Home Assistant), in the browser pane:

- Opening the page: nothing selected, the right side says "Choose a screen", the install card and the editor hidden.
  The Refresh button and a light inventory select nothing.
- Clicking a screen opens the editor with its top bar, tiles and the entity list (34 results), and marks it in the
  list. With an unsaved name, clicking the other screen asks "You have unsaved changes. Open a different screen
  anyway?"; No keeps the screen, Yes opens the other one. Live updates afterwards keep that choice.
- After a reload nothing is selected again. A live update with no screens shows "Start with a screen" with its button;
  with the screens back, "Choose a screen" returns. Settings and back to My screens keep the empty choice.
- At 375 px the screens sit in their row above a short "Choose a screen" card (no tall empty card, no icon, no
  horizontal scroll); tapping a screen opens the editor.
- No console errors.

## Not tested

- Physical screens: not flashed in this round. Wake on Studio 1 from Home Assistant after the update is Max's check.
- The page in Home Assistant's Ingress frame.
