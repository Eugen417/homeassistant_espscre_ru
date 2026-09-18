# Test results app 0.2.74 / firmware 0.2.62 (2026-09-18)

Max: "zou het lukken om nog veel meer tiles toe te staan?", then a full-page tile ("soms willen mensen op pagina 1 een
tile die het hele scherm beslaat ... zodat als ze hem hebben hangen als lichtknop, wil je dat hij meteen werkt zonder te
kijken") and navigation tiles, with the designs approved first (option B "oplichtend", navigation tile N1) and the
instruction "test dit echt uitvoerig, het is zo belangrijk dat dit goed werkt op beide borden op alle manieren". See
CHANGELOG 0.2.74 and the compatibility note in docs/RELEASING.md. Built on 0.2.73 (`1ed42dc`, the Vue editor).

## What changed, in one breath

- 48 tiles per screen (one per slot of the eight pages), the firmware's tile list on the heap and sized to the layout
  (PSRAM on a Guition), so a screen with twelve tiles pays for twelve.
- Tile size `full`: the whole page. One big button (the domain icon in a 128 / 64 px circle from a new icon font that
  carries only the domain icons, the name and the state centred) that lights up in the state colour while on; with a
  small slider, direct controls or a graph the double-width card's head stays on top and the control takes a strip
  at the bottom, with the value the double-width card had no room for (room temperature, blind position, media
  title) in between. Clock, forecast (with an hours strip) and sun path fill the page.
- Navigation tiles `screen.page_1` … `screen.page_8`: arrow (or a chosen icon), name, "Page n" and a chevron; a tap
  opens that page, holding does nothing. One per page it goes to, so an entity still appears once on a screen.
- Editor: Size Normal / Double-width / Full page, a "Goes to page" sheet, 48 from firmware 0.2.62, a full tile moves
  the other tiles of its page to the first free cells after it.

## Automated

- Python (`.venv-portal`): 396 tests OK, including the new `tests/test_full_page_tiles.py` (sizes and footprints,
  in-order packing, free slots, validation of full and page tiles, the firmware they need, the layout sensor, the wire
  form of built-in tiles, tile events for Claude in Home Assistant, the 48 limit). Correction: the committed 0.2.74
  runs 409 tests; 396 was counted on the branch before the rebase onto the Vue editor.
- C++ (`c++ -std=c++17 -Wall -Wextra -pedantic`): 17/17 PASS. `tests/test_runtime_model.cpp` now covers 48 tiles, a
  list sized to the layout, `screen.page_<n>` validation and page targets, and full tiles in `pack()` and `place()`
  (a page of their own, snapping to the page start, the page count).
- `tools/generate_icons.py --check` and `tools/generate_packages.py` green; the new fourth icon font carries the
  31-glyph subset of `tile_icons.BIG_GLYPHS` in both profiles.
- ESP32 check builds of both bench profiles compile (the first attempt caught one `std::max(int, long)`).

## Firmware on the Mac (host builds, driven over the ESPHome API with a virtual finger)

`.esphome/readme-render/fullpage/fullpage_host.py guition|cyd --compile` (git-ignored tooling) builds the board's
package as a host program, feeds it the full-page demo home (`demo_home_fullpage.py` on 8098, the real add-on with
screens that report firmware 0.2.62) and drives it with `fp_probe` (geometry, colours, texts, slider and key
positions, the page shown, `check_tile_geometry()`), `fp_finger` (a pointer indev, so taps, holds and drags run through
LVGL and the touch guard) and `fp_close`.

Results: Guition 37/37 checks, CYD 37/37 checks (the CYD layout has a media player where the Guition has a blind, so
the harness follows each board's demo layout). What the checks cover:

| Page | Checks |
|---|---|
| Full light, no control | page size (448 × 348 / 302 × 164), geometry, lit amber while on with "59 %", centred 128 / 64 px circle, a tap in the middle and one low in a corner send `light.toggle`, the card drops the lit colour at once, a hold opens the light card |
| Menu | three navigation tiles with "Page n" and a chevron (one double width), a tap on the first and on the wide one opens their pages, a hold does nothing |
| Full climate, mode keys | head on top, three keys bottom-centred, "20.5°" between, the third key sends `climate.set_hvac_mode cool`, a tap between head and keys opens the climate card |
| Full graph | head on top, no slider or panel, "21.4 °C", geometry, a tap asks for the history card |
| Full blind (Guition) / full media player (CYD) | three keys and "60 %" / "Blue in Green" in the middle, the first key sends `cover.open_cover` / the middle key `media_player.media_play_pause`, a tap between head and keys opens the card |
| Full forecast | geometry with the hours strip, a tap opens the weather card, six forecast/clock page flips with the second-hand tick running |
| Full clock | geometry, a tap does nothing |
| Full fan, small slider | a 64 / 40 px strip at the bottom, a tap above it sends `fan.toggle`, a drag from 30 % to 70 % sends `fan.set_percentage` near 70 |
| 48 tiles | eight pages of six with every name in place, Next seven times reaches page 8, the last tile still answers a tap, 42 singles plus a full tile with a slider on page 8, and the same slot back as a single tile with its own geometry |

### What the harness found before the release

- **A crash on a page switch from a full forecast to a full clock.** The forecast's hour labels are parts 18 and up,
  and part 18 is the analog clock's second hand. Right after a page switch the slot already names the clock while its
  parts still belong to the forecast, and the one-second tick called `part_line` on an hour label:
  `lv_line_set_points` on a label overwrites the label's text pointer, and the next `lv_obj_clean` freed a pointer
  into the point buffer ("pointer being freed was not allocated", the macOS crash report Max pasted). Fixed: the tick
  only moves the second hand while the slot's parts are a dial (`w.extra_mode == "analog"`), and a slot rebuilds its
  parts when it changes between full and double width (a CYD dial is large on a full card and small on a wide one,
  which swaps labels for lines at the same indices). Six forecast/clock page flips are part of the harness now.
- A full-page tile got the domain's default direct control (a toggle on a light) instead of being the big button;
  `resolve_controls` now gives a full tile no control unless one was chosen, and the editor shows "None" as chosen.
- The middle value of a full climate or cover card used the clock font, which has no '.' or '°' (it drew "20□5□");
  it uses the large value font now.
- On the CYD the full forecast took the Guition's three-row day columns and ran them together; a full forecast and
  sun path keep their board's layout.

## Hardware

Both bench boards were OTA-flashed with firmware 0.2.62 from the Mac (`esphome upload easy-<board>-device.yaml
--device <ip>`), the Guition first, and report 0.2.62 in Home Assistant. Heap right after boot (HA sensors): Guition
89 KB free / 60 KB minimum / 45 KB largest block (0.2.59: 76 / 57 / 33), CYD 116 KB / 107 KB / 59 KB (0.2.59: 106 /
96 / 51). Correction: the after-boot figures are not the steady state. With 48 populated tiles the CYD has about
97 KB free, a 49 KB largest block and a 78.6 KB minimum; a 13-tile steady state is about 98 KB free / 49 KB largest
block, so compare against those, not against 116 / 59.

- `hw_push.py <scene> [cyd]` pushes one full-page scene straight to a board's `screen_message` action and, on the
  Guition, snapshots the LVGL render through `ui_snapshot`: the big button (lit amber, in Max's dark look a brown
  amber), the menu of navigation tiles with chevrons, the climate card with the room temperature and the mode keys,
  the graph, the blind with its keys and position, the analog clock filling the page, the fan with its slider strip;
  all as on the host. The CYD has no snapshot action; it answered `device_info` after every scene.
- `hw_push48.py guition|cyd [flat]` pushed 43 tiles (seven full pages of singles and a full tile with a slider on
  page 8) and 48 single tiles; both boards accepted them.
- `hw_transition.py guition forecast,clock,forecast,button,clock,graph,forecast,fan,climate,clock` re-uses one slot for
  one card kind after another over one API connection; the board stayed up throughout.
- The home add-on (0.2.69) restores the real layout within about half a minute after every push, so the snapshots are
  taken right after a push.

## Editor (the real add-on on the demo home in the browser pane)

Checked twice: first in the previous editor (static/app.js, where the round was built), then in the Vue editor that
0.2.73 brought and into which session -75 ported the round (branch editor-fullpage, `00c9a4e`, with Vitest tests).
Previous editor: "12 / 48"; a full-page tile fills its page on the mockup; the tile sheet offers Normal / Double-width /
Full page with the hint; choosing Full page for Sam on a page with four other tiles kept Sam there and moved the other
four to the first free cells of page 8; ArrowDown moved the full tile a page down and swapped the full climate tile up;
"Go to page 4" from the picker landed on the free cell of page 8; the navigation-tile sheet shows "Goes to page" 1 … 8,
Size Normal / Double-width, the icon and the colours; Save & send stored `size: full` and the page tiles at their slots.
Vue editor: "8 pages · 12 / 48 tiles", the full tile drawn over its page, "Page 3 ›" on the navigation tiles, eight
"Go to page n" rows in the library, the tile drawer with Size Normal / Double-width / Full page (selected, with the
hint) and direct control None with the "at the bottom" hint, the navigation tile's drawer with Goes to page 1 … 8,
Size Normal / Double-width and the colours.

## Not tested

- A real finger on the glass with the new firmware (Max's visual check after the release, [[max-visual-check-after-flash]]).
- The add-on release on Max's Home Assistant Yellow: the add-on there is 0.2.69 until he updates it.
