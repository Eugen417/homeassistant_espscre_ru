# Test results app 0.2.82 / firmware 0.2.69 (2026-09-18)

GitHub issue #9 (lokitol): "when using the next page tiles, is it possible to remove the bottom of the page
next/previous buttons and extend the size of the tiles to utilise all the screen?" Max chose a screen setting with a
warning in the editor for pages that can't be reached. During the round he also asked for new page buttons: only MDI
icons in the corners, no words. He picked proposal B from two renders: bare chevrons and page dots. He kept the whole
half of the bar as the key, lit up under the finger, with no round shadow, and asked for the same buttons on the
screen's settings page. He approved the final renders before release ("ik ben blij, wat mij betreft releasen").

## What changed

- Setting `page_buttons` (default on): preference record `0x50474231`, `settings_screen::set()`, the row "Page
  buttons" in the Screen group, switch "Page buttons" in both profiles, `SETTING_RULES`/`SETTING_ENTITIES`/
  `SETTINGS_BESIDE_BLOCK` in the app, a row in the editor and in the Claude skill's entity table.
- `runtime_tiles::place_page` places the rows (`rows()`): the profile's rows with the bar, the whole room down to the
  bottom margin without it (Guition 108 → 122 px, CYD 52 → 61 px). Large cards lower their YAML places by half the
  growth; the single analog clock keeps the profile's dial.
- Page keys: each half of the band under the tiles (Guition 240×60, CYD 160×36) with a chevron; `page_number` holds
  the dots (`settings_screen::page_dots`); the settings page's pager draws the same.
- Editor: `strandedPages()` + `pageReachWarning()`, shown above the pages and under the Screen card.

## Automated

- `tools/check.sh`: Python 455 tests, C++ 18/18 (`test_settings_screen.cpp` covers the row and `set("page_buttons")`),
  packages, icons, Vitest 81/81 (`strandedPages`, the warning's words and when it stays quiet), vue-tsc, build.
- `tools/check.sh --firmware` (ESPHome 2026.6.2): CYD 1,655,136 B = 90.2 % of the 1,835,008 B slot, +1,632 B against
  0.2.81 (tight band: the delta is stated), Guition 2,029,088 B = 25.0 % of its 8,126,464 B slot.

## On the host (the real firmware for the Mac, SDL, own API ports 6191/6192)

- Renders of every card kind, the README showcase layout, the eight full-page cards and a one-page layout, each with the
  page buttons on and off, on both boards; the settings page's Screen group (two pages now) and dark mode (switched
  while a page is shown, and after a page change). Max checked them all before release.
- `ui_self_test` with the bar on and off, on both boards, with two layouts: every `page_check` PASS (10/10 per run) and
  no `Tile geometry`, `Card place`, `Panel bounds` or `Part bounds` failure. `check_tile_geometry` now also checks that
  every card lies inside the tile area, clear of the bar, and that without the bar the area reaches the bottom margin.
- LVGL's own hit test (`lv_indev_search_obj`) across the band: every point of the left half is Previous, every point
  of the right half Next (the dots take no touches); just above the band a finger gets the card; without the bar the
  band belongs to the cards.
- The small slider of 0.2.81 on a taller card, with a virtual finger (a second LVGL pointer): the strip claims the top
  and middle of its card, a drag that starts above the strip sets the light (`light.turn_on`), the page stays, and a
  still press beside the strip is the tile's tap. 16/16 on both boards with the bar on and off.
- Editor in a browser against a demo add-on whose screen owns its settings: the warning names the pages ("You have 1
  Go to page tile, to page 1, but none to pages 2, 3 and 4") and follows a new Go to page tile at once; the Page
  buttons switch sends `PUT .../settings` (200) and the warning goes when it is on.

## Not tested

- On real hardware: the new keys under a finger (GT911 and XPT2046), the taller cards, and switching the setting from
  the screen's settings page and from Home Assistant. The bench boards were not flashed in this round.
