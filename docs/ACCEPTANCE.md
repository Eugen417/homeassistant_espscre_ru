# Acceptance test and handover

Note the date, board variant/MAC, Git commit, ESPHome version, device name, and the
names of the two measurement files. Don't include passwords in the report.

## Without real HA actions

1. Calibrate using [CALIBRATING.md](CALIBRATING.md); flash the result and run a
   new, independent measurement with `verify`. A successful fit alone
   doesn't prove the firmware coordinates are correct.
2. Flash normal mode. Check that the measurement screen disappears.
3. Check every visible tile and its edges: no other tile should respond.
   Tap calmly and rapidly in succession, but expect no repeat within the
   deliberate debounce period. A swipe should not trigger a tile action afterward.
4. With 1–6 tiles: no page button. With 7–10: repeat at least twenty times:
   Next → tile → Previous. The bottom tile should not respond to the
   page button. Open/close overlays; a tap inside one should not leak through.
5. Run the built-in render test via the encrypted API:

   ```sh
   python diagnostics/run_ui_test.py --host display-kitchen.local --name display-kitchen
   ```

   Requires ten successful page checks, fifty overlay cycles, and progress
   from real draw callbacks. This doesn't test physical touch or HA actions.
6. Leave the board on for at least 15 minutes. Check that it doesn't restart,
   that standby only starts after ten minutes without touch, and that the first
   tap wakes the screen without toggling a device.

## With Home Assistant

Only now enable direct actions, reflash, and choose safe test devices.
Check every configured tile with its real HA entity:

- A tap performs exactly one intended action; the feedback is correct.
- Changes from HA also appear on the screen.
- Light slider/color, climate setpoint, and supported modes work where used.
- Vacuum actions and speeds work where supported. Note any missing functionality.
- After a power interruption, Wi-Fi and HA come back; name, calibration, and tiles persist.
- After a brief Wi-Fi/HA interruption, the connection recovers without a USB flash.

Keep logs local only. Check for resets, watchdog messages, repeated
connection errors, and noticeably slow components. A short render test doesn't
prove stability over days; note the duration actually observed.

## Automated development checks

From the repo, with the Python environment active:

```sh
python -m unittest discover -s tests -p 'test_*.py'
c++ -std=c++17 -Wall -Wextra -pedantic tests/test_cyd_ui.cpp -o /tmp/test_cyd_ui
/tmp/test_cyd_ui
c++ -std=c++17 -Wall -Wextra -pedantic tests/test_touch_filter.cpp -o /tmp/test_touch_filter
c++ -std=c++17 -Wall -Wextra -pedantic tests/test_light_controls.cpp -o /tmp/test_light_controls
/tmp/test_light_controls
/tmp/test_touch_filter
c++ -std=c++17 -Wall -Wextra -pedantic tests/test_light_controls.cpp -o /tmp/test_light_controls
/tmp/test_light_controls
python -m esphome config device.yaml
python -m esphome compile device.yaml
```

The C++ commands above are for macOS/Linux with a compiler. On Windows,
the developer can use whatever C++17 toolchain is available. Only flash the
checked profile to the identified board.

## Handover report

```text
Board / MAC:
Git commit / ESPHome version:
Device name / HA entities:
Calibration measurement + independent verification:
Render test:
Physical pagination/touch/standby:
Real HA actions:
Observed stable duration:
Limitations / not yet tested:
Local profile files safely stored:
```

Give the owner the three local YAML files and the measurement files through a
suitable private channel. For a different owner, use the clean repository
or starter ZIP, so they get their own keys and their own panel calibration.

## Light card

- RGB+white light: three sliders; RGB-only: color and brightness; white-only:
  white temperature and brightness. A plain dimmer keeps the brightness card.
- Drag slowly across the rainbow, release, and check the physical light.
  Repeat for white temperature and brightness. Brightness keeps the color mode.
- Reopen the card and check the received values. Test two lights with
  different temperature ranges; no value should leak from the other light.
- The firmware render diagnostic includes `Light sliders: PASS`: twenty preview events
  per slider, exactly one commit on release, no duplicate commit, no commit
  after an aborted touch, and four capability combinations. This runs with
  swapped-in callbacks and doesn't control any real lights.
