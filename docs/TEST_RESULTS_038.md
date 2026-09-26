# Two buttons and button colours on an alert, 0.3.8 acceptance (firmware 0.3.3)

Tested on 26 September 2026 on three bench screens (a 4-inch Guition 4848S040, a Waveshare ESP32-S3-Touch-LCD-4.3 and
a CYD ESP32-2432S028) over OTA, with a real Home Assistant. The alert gets `show_alert_choice` (the seven fields of
`show_alert` plus `button_color`, `button2_text` and `button2_color`), and the `esp_screens_show_alert` event gets
these three and `button2_action` with `button2_data`, all optional.

## Automated checks

- `tools/check.sh --all` on 0.3.7 (main b1184b7) with this change: 740 Python tests, 28 C++ programs, the package,
  cell, board shape, entry file, icon and translation checks, and the editor's tests, types, build and bundle. Every
  board compiles with ESPHome 2026.9.0.
- New tests: `test_alert_overlay.cpp` checks the second button's text and colours and where two buttons stand on every
  glass, look and picture shape. `test_alert.py` and `test_alerts_reference.py` check the action's fields against the
  profiles. `test_alert_broadcast.py` covers the event: `show_alert_choice` for a screen with firmware 0.3.3, `show_alert`
  with one button for an older one, and each button's own action performed once.
- The host render harness (tools/render) passed its self test on all sixteen variants, lying down and standing up.
  It now also renders an alert with two buttons in the keys' own paints, in key colours, on a coloured card, and with a
  camera picture. A standing doorbell camera (3:4), whose picture goes beside the words on wide glass, keeps both
  buttons in the column of words. On this Mac a portrait variant once missed a page swipe while the machine was under
  heavy load; it passed when run again.

## Where two buttons go

Three layouts were rendered on every board before one was chosen:

- both buttons of the one button's width against the right edge: on the CYD a label such as "Remind me" ended in dots;
- one button at each edge of the card: the two answers drifted apart and no longer read as one choice;
- the two buttons sharing the row in equal halves, the first on the right (chosen): each is as large as the card
  allows, and the labels fit.

## On the screens

The alert was sent to each screen through its own `show_alert_choice` action, and someone at the screen pressed the
button asked for:

| Screen | Button pressed | Ending the screen reported |
|---|---|---|
| Guition | the first, green | `ok` |
| Guition | the second, red | `button2` |
| CYD | the second, grey | `button2` |
| Waveshare 4.3 | the first, orange | `ok` |

- The Guition's own LVGL render (`diagnostics/capture_ui.py`) showed the card over the screen's tiles with both keys
  in their colours, side by side.
- `show_alert` still shows one button on all three screens, also right after an alert with two, and closed by its
  timeout.
- With the add-on of this release as a local add-on, `esp_screens_show_alert` with `screen` and a second button reached
  the Waveshare as `show_alert_choice` ("1 of 1 screens", no one-button fallback). The press of the second button and
  its `button2_action` through the add-on were not tried on a screen; the add-on's handling of it is covered by the
  tests above, and the screen's side of it (the `button2` ending) by the presses in the table.

## Firmware sizes

| Board | Image | Share of the update slot |
|---|---|---|
| CYD | 1,657,056 B | 90.3 % of 1,835,008 B, +7,600 B against 0.3.7 |
| Guition 4848S040 | 2,078,000 B | 25.6 % |
| Waveshare 4.3 | 2,334,400 B | 28.7 % |
| Guition JC8012P4A1 | 2,051,088 B | 25.2 % |
| Waveshare 7 | 1,863,792 B | 47.4 % |
| Waveshare 4B | 2,065,904 B | 25.4 % |
| Waveshare 3.5 | 1,900,208 B | 23.4 % |
| Guition JC1060P470 | 2,138,704 B | 26.3 % |
| Guition JC1060P470 V2 | 2,138,752 B | 26.3 % |

The CYD is over 90 % of its slot, which this release accepts; a cleanup of the CYD build follows later.
