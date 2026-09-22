# Test results 0.2.111 (firmware 0.2.95)

A card under a finger darkens the moment the finger lands and fades back over 200 ms when it lifts.

## Why (2026-09-22)

The press of a card was a 45 % transparent card on the page, a shade lighter that was easy to miss, and it snapped
back the moment the finger lifted; the owner asked for a press that is clearly visible and a short fade out, done
with LVGL's own animations and kept lean. The busy sheet with the spinner was out of scope: it is for an answer
that takes long, not for the tap itself.

## What 0.2.111 does, verified

| Check | Result |
| --- | --- |
| `tools/check.sh` (Python tests 580, every `tests/*.cpp` 24/24, packages, cards of every grid, board shapes, icons, translations, editor tests, types, build, bundle) | 21 passed, 0 warned, 0 failed (`--all`) |
| `tools/check.sh --firmware`: every board profile compiles with ESPHome 2026.9.0 (CYD, Guition 4-inch, Waveshare 4.3-inch, Waveshare 7-inch, Guition 10.1-inch); CYD image 1,623,872 bytes, 88.5 % of the update slot, 208 bytes less than 0.2.110 | PASS |
| `tests/test_theme.cpp`: `pressed()` is an eighth towards black in light (white gives E0E0E0, a pastel keeps its hue), darker than the page, and an eighth towards white in dark | PASS |
| Guition 4-inch on the bench, this change as a bench build labelled 0.2.94 over OTA (image 09:52:53, made on the tree before 0.2.110 landed), `device_info` reports 0.2.94 | PASS |
| CYD on the bench, the same bench build over OTA (image 09:54:58), `device_info` reports 0.2.94 | PASS |
| The owner's verdict after tapping on both boards: the press and its fade are what the project needed | PASS |

## How the fade is made

- The card's PRESSED state carries `theme::pressed()` of the card's own colour (of the page for a card without a
  background) at full opacity, in place of the theme's 45 % veil; `render_slot` keeps it with the card's colour, so a
  change of card or of look updates it.
- The fade is an LVGL style transition on `bg_color` and `bg_opa`, 200 ms ease-out, on one shared style every card
  gets in `bind()`. LVGL takes the transition of the most specific state that matches the new state, so the PRESSED
  state carries a local transition of 0 ms: the press stays instant, only letting go fades.
- `press_ground()` ends a running fade when a slot gets a new colour (a page switch within the 200 ms), so the new
  card never wears the old one's shade. Adding a style again is LVGL's public way to end an object's transitions.
- No object, no layer, no full-screen redraw: each step of the fade invalidates the card alone.
