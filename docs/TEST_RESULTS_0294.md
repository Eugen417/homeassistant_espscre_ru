# Test results app 0.2.94 / firmware 0.2.80 (2026-09-20)

The responsive round: a board declares its grid, its density and its look, and the firmware, the manager and the
editor follow (docs/RESPONSIVE.md). The rule for the release: the CYD and the Guition draw what they drew before,
and a screen on main moves over in one go. This is what was checked, and how.

## Gates

- `tools/check.sh`: 13 of 13 (541 Python tests, 21 C++ tests, the package, cells, board-shape and icon checks, the
  translations, the editor's 111 Vitest tests, vue-tsc, the build and the bundle in Git).
- `tools/check.sh --firmware`, now over every board in `tools/profiles.py`, with the ESPHome the add-on ships
  (2026.9.0): CYD 1,622,624 B = 88.4 % of its update slot (0.2.92: 1,633,760 B, 89.0 %; the thermostat overlays left
  the YAML, the Rotation select and its rows came), Guition about 25 %, Waveshare about 28 %.
- `esphome config` of the three checkout entries with the packages' `min_version` (2026.6.2): valid. The only
  entity differences with main are additions: the two diagnostic sensors Screen layout and Screen board on every
  board, and the Rotation select on the CYD (the Guition had it). Nothing is renamed or gone, so Home Assistant
  keeps every id.

## The two first boards render as before

Host builds (`.esphome/readme-render/responsive-lab`, ESPHome 2026.9.0, the frozen demo home of the README
renders) of the branch before this round (38f549d) and after it, twenty renders per board: every page with and
without the page bar, the settings pages, the alert, the busy sheet and the light, colour, thermostat, blind, cover,
robot, media and history cards.

- CYD: 20 of 20 identical.
- Guition: 19 of 20 identical. The colour card differs on purpose: the branch had drawn its three rows 2 px closer
  (`room / 3` forgot the last row has no gap under it); the round puts them back where main draws them, the last
  row ending at 454 px again.
- The blind with tilt on a square panel keeps its one column (the two-column form is for glass wider than 3:2);
  the demo's blinds have no tilt, so that case rests on the arithmetic in `overlay_card::columns`, not on a render.

## Live, on the home Home Assistant

The local add-on (`local_esp_screen_manager`, this round's code) with the three bench screens paired: the Guition
Wallbox (480 × 480), the CYD (320 × 240) and the Waveshare (800 × 480).

- The inventory serves every screen its shape (`480x480 2x3 170dpi standard`, `320x240 2x3 143dpi compact`,
  `800x480 3x3 217dpi standard`), whether it draws pictures (Guition and Waveshare yes, CYD no) and its tile limit
  (48, 48, 63).
- A layout saved on the Waveshare with a wide tile at slot 3 (second row), a full tile at slot 9 (page 2) and a tile
  at slot 62 (the last cell of page 7) is accepted, reaches the screen (inbox Synced within a second) and reads back
  in the layout sensor as `columns 3, rows 3, max_pages 7`, the wide tile on page 1 row 2 column 1, the full tile on
  page 2. The same positions offered to the CYD are refused with "A double-width tile starts in the left column".
- A screen whose sensors are unavailable is known by the board package its profile YAML builds from
  (`Manager.grid_of`), so an offline Waveshare still saves on three columns (`tests/test_grid.py`).
- Firmware 0.2.80 was flashed over the air from this checkout to the Guition Wallbox, the CYD and the Waveshare.
  After the update each reports its shape with density and look, which the add-on prefers over `boards.json`.

## Turning, on the three screens

Every board turns since firmware 0.2.80: a half turn on any glass, the quarter turns as well on a square one. Checked
through the local add-on, whose settings view now says which angles a screen takes:

- The CYD (320 × 240) is offered 0° and 180°; its new `select.<screen>_rotation` has those two options. A change to
  180° through the add-on is accepted and the select follows; 90° is refused with "Only a square screen can turn a
  quarter; this one turns upside down (180°)"; 0° again puts it back.
- The Guition (480 × 480) keeps all four angles: 90° through the add-on, the select follows, 0° again.
- The Waveshare (800 × 480): 0° and 180° offered, its select has those two; 180° accepted and followed, 90° refused
  with the same sentence, 0° again puts it back. All three screens report firmware 0.2.80 and Synced afterwards.
- The settings page on the screen shows the row of two angles on the CYD and the Waveshare and the row of four on
  the Guition (`tests/test_settings_screen.cpp`); the board files' selects are checked against their shape by
  `tools/check_packages.py`.

## The real-world test on the three screens (2026-09-21)

Before the release, every card type on every bench screen at once, saved through the local add-on: the Guition 36
tiles on 8 pages, the CYD 30 tiles on 7 pages, the Waveshare 45 tiles on 7 pages (single, wide and full tiles;
lights, colour lights, WLED effects, thermostats, blinds, covers, robots, media with covers, history graphs, camera
and live camera tiles where the board draws pictures, weather, scripts, scenes, settings tiles). Max pages through
them by hand; his own layouts are restored afterwards.

- The Guition and the CYD take theirs and hold them: Synced within seconds, the free heap steps down once when the
  layout arrives (Guition 88 to 78 KB, CYD 144 to 110 KB) and is flat afterwards.
- The Waveshare hung. Twice, within two minutes of a large layout, with and without the picture tiles: no ping, no
  API, no serial output. Its log before the hang: "Time request dropped, TCP buffer full", "Buffer full, ping
  queued", then "Home Assistant ... is unresponsive; disconnecting". The cause was the memory inside the chip: the
  board ran on 15.6 KB free with Max's eleven tiles and 26 KB after a reset, because LVGL's draw buffer (a quarter
  of 800 × 480 is 192 KB) lives there, next to the panel's bounce buffers, and the earlier round had cut the Wi-Fi
  and TCP buffers to make room. A large layout filled what was left, the API stalled, and the board stopped.
- The fix is the option that round wrote down and did not take: the draw buffer at 12 % of the glass (the CYD's
  share, 92 KB) and ESP-IDF's own Wi-Fi and TCP buffers again. The board boots with 112 KB free, takes the 45 tiles
  with two live cameras and holds them: twenty minutes Synced and answering, the free heap moving between 67 and
  83 KB as the live strip comes and goes, the lowest 56 KB. It was flashed over USB; it went into the board file
  and the firmware stays 0.2.80 (nothing shipped in between).

## The weather card (2026-09-21)

Found while reading the round back: the card a tap opens on a weather tile stacked its two blocks without ever
asking whether they fit. On the Waveshare that left the coming days 51 px tall with five rows drawn over each
other; the CYD and the Guition were right, which is why it had not shown - both are tall enough for the design
the blocks were drawn at. It is a computed card now (`components/smart_display/weather_card.h`), like the
thermostat and the media card.

- `tests/test_weather_card.cpp` (the 22nd C++ test) walks 240 x 320 to 1024 x 600 in the two looks, with zero to
  eight hours and one to five days, and asserts that nothing leaves the glass, no block covers another, a day row
  is never thinner than three quarters of its line, the columns of a row stay in order, and every day is on a page.
- Host renders of the three boards, before and after (the frozen demo home): the CYD and the Guition are identical
  to the byte. The Waveshare keeps its hour strip, gives up the rain under it and the "Coming days" heading, and
  shows all five days on rows of their own.
- A 4.3 inch of 480 x 272 (lab-g43) gives up its hour strip instead and keeps the heading: the state a bigger
  concession makes unnecessary is taken back, which is what the state list in `weather_card::layout` is for.
- The pager appears on no board that ships: the cascade finds the room first on all three, which is the point of
  the order. Its geometry is what the test covers (a shape that cannot hold five days however the card gives);
  its drawing is the settings page's pager, the same objects in the same place. It has not been on glass.

## Not covered here

- Whether the picture on the glass turns with the select was not looked at from here: the boards are in the house,
  and a turn shows on the panel, not in Home Assistant. The Guition's turn is the same code path as before.
- `tests/test_scaling` logs "Publishing the layout of Office 1 failed (AttributeError)" ten times: its fake Home
  Assistant has no `set_state`. It did so before this round as well.
