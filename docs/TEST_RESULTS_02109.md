# Test results 0.2.109 (firmware 0.2.93)

A page switch lands in one piece: the old page stays on the glass until the new one is complete, and the
complete page replaces it in one frame.

## Why (2026-09-22)

The owner found the page switch of 0.2.42 to 0.2.108 cheap to look at: every card of the new page came up as
an empty frame first and the cards were filled in two at a time, a page visibly being built. That design kept
touch polling alive between the fill steps when a page cost 300 ms of CPU; the content pass has since become
two to three times cheaper (docs/SWIPE_PROFILE.md, build 7: 57 ms for a page on the Guition, 29 ms on the
CYD), so drawing the page before showing it costs less in feel than watching it fill. A fade or a slide was
weighed and left out: every frame of a screen-wide animation is a full software redraw, 50 to 110 ms on these
boards, so it would run at 10 to 16 frames per second.

## What 0.2.109 does, verified

| Check | Result |
| --- | --- |
| `tools/check.sh` (Python tests 580, every `tests/*.cpp` 24/24, packages, cards of every grid, board shapes, icons, translations, editor tests, types, build, bundle) | 13 passed, 0 warned, 0 failed |
| `tools/check.sh --firmware`: every board profile compiles with ESPHome 2026.9.0 (CYD, Guition 4-inch, Waveshare 4.3-inch, Guition 10.1-inch); CYD image 1,624,080 bytes, 88.5 % of the update slot, 579 bytes less than 0.2.107 | PASS |
| Guition 4-inch on the bench, firmware 0.2.93 over OTA (image 08:24:43), `device_info` reports 0.2.93 | PASS |
| CYD on the bench, firmware 0.2.93 over OTA (image 08:27:16), `device_info` reports 0.2.93 | PASS |
| Waveshare 4.3-inch on the bench over USB, lying down (image 08:29:26) and then standing up (08:33:18), `device_info` reports 0.2.93 | PASS |
| The owner's verdict after swiping on all three boards: the switch is whole and the screen feels more responsive than before | PASS |

## Seen on the way, not part of this release

- The Waveshare standing up draws a page in 400 ms or more, strip by strip from the top. ESPHome rotates every
  strip in software for an RGB panel that cannot rotate itself, into a rotation buffer it always places in PSRAM,
  writing the pixels with a stride of a column; with a 92 KB strip (12 % draw buffer) and a 32 KB data cache that
  write pattern thrashes, and the panel's DMA already reads the whole picture out of the same PSRAM sixty times a
  second. Lying down there is no rotation step at all. To be measured with the `-DSWIPE_PROFILE=1` build at 12 %
  and at 6 % draw buffer; a one-line change in ESPHome (rotation buffer in internal RAM like the draw buffer) is
  the real fix. Left for a later round at the owner's request.
