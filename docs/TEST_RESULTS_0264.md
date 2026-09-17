# Test results app 0.2.64 / firmware 0.2.55 (2026-09-17)

An alert with a long title (`show_alert` with title "Washing machine is done" and subtitle "Empty it before the laundry
smells") showed the title on two lines on the Guition, the second line on top of the subtitle, although the comment on
`ALERT_TITLE_MAX` and README_EXTENDED promise one line that ends in an ellipsis. See CHANGELOG 0.2.64 and the
compatibility note in docs/RELEASING.md. Built on 0.2.62 while Dark mode was under way in another session, rebased
onto 0.2.63 when it was released, and tested again there (only the version lines, CHANGELOG and RELEASING conflicted).

## Cause

LVGL 9.5.0 (`src/widgets/label/lv_label.c`, as ESPHome 2026.6.2 builds it): `lv_label_refr_text` only puts the dots of
`LV_LABEL_LONG_MODE_DOTS` on a label whose wrapped text is taller than its content area. `alert_title` had a width but
no height, so `LV_EVENT_GET_SELF_SIZE` made it exactly as tall as its wrapped text and the dots never came. With a
height of one line, `lv_label_get_letter_on` finds the letter on that line (which it breaks anywhere, since it is the
last one that shows) and the text ends in "...".

ESPHome's `lv_font_t.line_height` is FreeType's size height. The generated code builds `headline` (Roboto 500) as
`font::Font(..., 26, 32, 7, ...)` at 27 px and `font::Font(..., 17, 21, 5, ...)` at 18 px (baseline, height, descender),
and freetype-py (FreeType 2.13.2) gives the same 32 and 21 px.

## Automated

- Python (`.venv-portal`): 312 tests OK on 0.2.63 (302 on 0.2.62). New in `tests/test_alert.py`: in both profiles and
  both packages the title has `height: ${ALERT_TITLE_H}` and the `headline` font, `ALERT_TITLE_H` equals the line
  height computed from the font file (`head` and `hhea`, rounded as FreeType does: 32 and 21), and the title ends above
  the subtitle. The test fails on 0.2.62 before the fix.
- C++ (`clang++ -std=c++17 -Wall -Wextra -Werror -I. -I components/smart_display`): 16/16 PASS on 0.2.63 (15/15 on
  0.2.62).
- `generate_packages.py --check` green.

## Builds (ESPHome 2026.6.2)

Check profiles with the generated packages and local components, as for 0.2.62, built for this release and for 0.2.63
(the same check profiles from `bc2a754`). All compile without errors.

| Profile | RAM | Flash | vs 0.2.63 |
|---|---|---|---|
| CYD | 23.7% (77,500 B) | 77.4% (1,420,991 B) | +0 B RAM, -8 B flash |
| Guition | 24.7% (80,852 B) | 18.8% (1,524,647 B) | +0 B RAM, +8 B flash |

## Firmware on the Mac (host builds, driven over the ESPHome API)

`show_alert` on host builds of both packages over a demo page, before and after the change, with a probe that logs the
title's and subtitle's position and height as LVGL laid them out and the text the title shows:

| Board | Title | Before: title height, bottom | After |
|---|---|---|---|
| Guition | Washing machine is done | 64 px (2 lines), y 90, over the subtitle at y 68 | 32 px, y 58: "Washing machine is d..." |
| Guition | Door open | 32 px, y 58 | the same pixels |
| Guition | a title of 64 bytes | 96 px (3 lines), y 122 | 32 px, y 58: "The dishwasher in the ..." |
| CYD | Washing machine is done | 21 px (fits), y 37 | the same pixels |
| CYD | Door open | 21 px, y 37 | the same pixels |
| CYD | a title of 48 bytes | 42 px (2 lines), y 58, over the subtitle at y 44 | 21 px, y 37: "The dishwasher in the ki..." |

Comparing the renders pixel by pixel: no difference for the three titles that fit, and for the others only inside the
title and subtitle area. The "g" of "Washing" keeps its descender in the one-line box, which is as high as the label
of a title that fits always was.

On 0.2.63 (rebased, firmware reporting 0.2.55) the probe gives the same numbers and texts for all six, in the light
look and with the Dark mode switch on; the light card is pixel-identical to the render before the rebase, and the dark
card shows the same single line with its ellipsis.

## How long a title can be

The Claude skill and the editor's alert tips said the CYD shows about forty characters of a title. Measured with
FreeType's advances of Roboto 500 at the label widths, on seven typical titles: 20-23 characters before the "..." on
the Guition, 24-25 on the CYD, which matches the cuts LVGL made in the renders above ("Washing machine is d" and "The
dishwasher in the ki"). Both now say about twenty. The editor, served from this tree on a demo home, shows the new tip
in Settings → Alerts → Tips without console errors, and `claude_skill.text()` carries the new sentence.

## Not tested

- Physical screens: not flashed in this round.
