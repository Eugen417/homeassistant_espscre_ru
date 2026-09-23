# Test results app 0.2.130 (firmware stays 0.2.104, 2026-09-23)

A tooling release: the host harness of 0.2.129 (`tools/render/run.py`) now changes pages with a finger through ESPHome's
own touchscreen and looks at the screen in the moment after each action, and the releases of 0.2.128 and 0.2.129 were
put through the whole chain on a screen of their own. docs/TESTING.md describes the levels.

## The harness, made true to the glass

Each of these was found by the harness failing on a correct firmware, and fixed in the harness:

- A finger has to go through ESPHome's touchscreen, not straight into LVGL: the page swipe, the touch guard and the
  board's touch transform all live there.
- On the capacitive boards a page changes with a swipe from the edge of the glass; a wipe over the tiles is a flick
  and changes nothing, as designed. The CYD changes page with LVGL's gesture anywhere.
- The screen turns touches with ESPHome's own rotation, so the finger goes back through that, and through the board's
  swap and mirror, to what its touch chip would report.
- Every board reads its touch panel every 20 ms; the host's panel was read every 50 ms, so LVGL saw a finger that
  stopped between reads and never counted a swipe. The host now reads it as often as the board does.
- "Swipe between pages" is off until someone turns it on; the harness turns it on through its switch.

Result: 10 of 10 variants pass. On every board with more than one page, a swipe from the right edge brings page 2 and
one from the left edge page 1; the page title, read back every 50 ms from the lifting of the finger, never stood in dots
for a moment (on the narrow boards standing up the long title ends in dots every time, as it should). The alert card,
read back as it opens, before its picture and right after it, never had a part outside the card or over another.

## The whole chain

A 4-inch Guition used as a test screen, and a Home Assistant with ESPHome Device Builder and a second, older screen:

- ESP Screen Manager updated to 0.2.129 in Home Assistant; New screen offered the six boards of the catalog.
- New screen with **Download**: the profile with its own keys, and the firmware built by the add-on from the published
  packages (0.2.104). Flashed from a computer after erasing the board, as ESPHome Web does.
- Home Assistant discovered the screen; confirming it paired it with the key taken from Device Builder.
- Tiles given in ESP Screens with real entities (weather with its forecast, a temperature graph with its history, a
  person, the sun, a camera, media players, a thermostat, blinds, a light): on the glass, in the add-on's language.
- `esp_screens_show_alert` with a real camera (720 x 576): both screens got it. The test screen on 0.2.104 got the
  picture at 321 x 257, the frame its card makes for those proportions, 247,802 bytes, the same to the byte as the
  add-on's rule gives; the older screen on 0.2.102 got its old 16:9 frame. One snapshot, two sizes. On the glass 2.35 s
  after the event, no error line.

Not done here: the CYD with firmware 0.2.104 on its glass.

## Gates

`tools/check.sh`: 14 of 14. `tools/check.sh --render`: 10 of 10. CI of 0.2.129 (checks, both firmware builds and the
render job) was green.
