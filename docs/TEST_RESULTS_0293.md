# Test results app 0.2.93 / firmware 0.2.79 (2026-09-20)

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
- Firmware 0.2.79 was flashed over the air from this checkout to the Guition Wallbox, the CYD and the Waveshare.
  After the update each reports its shape with density and look, which the add-on prefers over `boards.json`.

## Turning, on the three screens

Every board turns since firmware 0.2.79: a half turn on any glass, the quarter turns as well on a square one. Checked
through the local add-on, whose settings view now says which angles a screen takes:

- The CYD (320 × 240) is offered 0° and 180°; its new `select.<screen>_rotation` has those two options. A change to
  180° through the add-on is accepted and the select follows; 90° is refused with "Only a square screen can turn a
  quarter; this one turns upside down (180°)"; 0° again puts it back.
- The Guition (480 × 480) keeps all four angles: 90° through the add-on, the select follows, 0° again.
- The Waveshare (800 × 480): 0° and 180° offered, its select has those two; 180° accepted and followed, 90° refused
  with the same sentence, 0° again puts it back. All three screens report firmware 0.2.79 and Synced afterwards.
- The settings page on the screen shows the row of two angles on the CYD and the Waveshare and the row of four on
  the Guition (`tests/test_settings_screen.cpp`); the board files' selects are checked against their shape by
  `tools/check_packages.py`.

## Not covered here

- Whether the picture on the glass turns with the select was not looked at from here: the boards are in the house,
  and a turn shows on the panel, not in Home Assistant. The Guition's turn is the same code path as before.
- `tests/test_scaling` logs "Publishing the layout of Office 1 failed (AttributeError)" ten times: its fake Home
  Assistant has no `set_state`. It did so before this round as well.
