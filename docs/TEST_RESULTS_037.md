# Kept and prepared pages, 0.3.7 acceptance (firmware 0.3.2)

Tested on 26 September 2026 on three bench screens (a 4-inch Guition 4848S040, a Waveshare ESP32-S3-Touch-LCD-4.3 and
a CYD ESP32-2432S028) over OTA, with a real Home Assistant and the published add-on 0.3.6. The screens carried
stress layouts of eight pages (Guition, CYD) and seven pages (Waveshare, the most a 3 by 3 grid takes): clocks, a
forecast, climate with modes, graphs, lights, a media card over a whole page with its album cover, tall media tiles,
live cameras, cover, vacuum and the sun path. docs/KEPT_PAGES.md describes the design.

## Automated checks

- `tools/check.sh`: 738 Python tests, 28 C++ programs (two new: `test_kept_pages`, `test_picture_store`), the
  package, cell, board shape, entry file, icon and translation checks, and the editor's tests, types, build and bundle.
- `tools/check.sh --firmware`: every board compiles with ESPHome 2026.9.0. See the table below for sizes.
- The host render harness (tools/render) built each variant twice, once as it ships and once with
  `KEPT_PAGES_HOST=1`, where the host program keeps and prepares its pages as a board with PSRAM does. Every render
  of the two runs is identical pixel for pixel (`tools/compare_renders.py`), and every self test passed in both.
  The run with kept pages found one false alarm in the self test: on the 10.1-inch standing up, a clock card that
  had shown a centred on/off card before kept that card's centred name, hidden under the dial, and the geometry
  check took it for a centred stack. The check now reads the alignment only of a name on the card; the screen never
  showed anything wrong (the same could happen without kept pages when a slot changes tile).
  On this Mac the Guition variant stopped in its protocol recovery step in both runs, and so did a clean checkout of
  main at 0.3.6, while CI passed it on the same commit: a local timing matter, not this change. CI on the pull request
  ran every variant.

## On the screens

- **Page switches.** The first tour after the start, all pages prepared: median 28 ms of CPU and 104 ms to the first
  frame on the Guition, where firmware 0.3.1 took 54 ms and 134 ms (about 250 ms for the media card with its cover).
  The rest of the frame time is LVGL drawing the page (about 200 ms for a page with a graph and the sun path).
- **Preparing.** After a restart, "Preparing pages n/total" built 7 pages in 1.2 s on the Guition and 6 pages in
  1.7 s on the Waveshare (70 to 240 ms each). A layout replaced from the add-on was prepared in the background in
  2.2 s (19 to 52 ms a page, as the card sets are reused). The CYD, without PSRAM, shows no such step and builds each
  page when it is shown, as before.
- **Kept pages up to date.** A tile that changed while its page was away was drawn again on the way back, and while
  the screen was idle the page was brought up to date before anyone turned to it. Dark mode on and off and Page
  buttons off and on redrew every kept page with the new look and the new room (LVGL snapshots checked), and both
  settings were set back.
- **Pictures.** The album cover of a media card over a whole page is fetched ahead once all pages are prepared and
  never again for the same track; a camera page that comes back shows its last picture at once.
- **Stripes.** A walk of the PSRAM heap took 1.7 ms with one page built and 2.3 ms with eight (3,300 and 4,600
  blocks), 0.12 ms for the internal heap. With no walk while the glass is lit, the owner saw no stripes or shifted
  frames on the Guition while camera pictures and covers loaded, where every load showed them before.
- **Self test.** `diagnostics/run_ui_test.py` passed on all three screens: 10 page checks and 50 overlay render
  cycles each, with every page kept.
- **Soak.** Two rounds of about 25 and 20 minutes with a tour through every page on each screen every 90 seconds,
  plus the owner's own use: no restart, no error in the log, free internal memory level (Guition about 70 KB,
  Waveshare about 75 KB, CYD 110 to 120 KB). With every page kept the Guition had 5.3 MB of PSRAM free, the Waveshare
  4.9 MB.
- **Page keys.** The chevron of the first page's Previous key is grey again (LVGL snapshot).

## Firmware sizes (ESPHome 2026.9.0, the user's build shape)

| Board | Firmware | Share of its update slot |
| --- | ---: | ---: |
| CYD | 1,649,440 B | 89.9 % (under the 90 % line of docs/RELEASING.md) |
| Guition | 2,071,328 B | 25.5 % |
| Waveshare 4.3 | 2,327,648 B | 28.6 % |
| JC8012P4A1 | 2,045,024 B | 25.2 % |
| Waveshare 7 | 1,857,040 B | 47.2 % |
| Waveshare 4B | 2,059,248 B | 25.3 % |
| Waveshare 3.5 | 1,892,384 B | 23.3 % |
| JC1060P470 and V2 | 2,132,656 B | 26.2 % |

## Limits

- The ESP32-P4 boards (JC8012P4A1, JC1060P470) keep and prepare pages too; they compile and render on the host but
  were not tried on the glass in this round. The Waveshare 4B and 7 inch neither.
- No physical tap was made by the tests; the owner used the screens by hand during the round.
- The cover mark without Home Assistant's token is covered by unit tests; the published add-on 0.3.6 still sent the
  old mark during these tests.
