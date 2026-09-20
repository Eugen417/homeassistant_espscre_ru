# One firmware, any board: how the screens fit their glass

Since app 0.2.93 (firmware 0.2.79) a board says what its glass is and the firmware, the add-on and the editor
follow: the tile grid, the size of everything drawn, the cards a tap opens. The CYD and the Guition were the
first two boards and render as they did before; the Waveshare ESP32-S3-Touch-LCD-4.3 (800 x 480, three by
three) was the first board added this way. The rule for every change here: the two first boards keep their
pixels, and nothing is written that knows a board by its name.

## The idea

A screen hangs on a wall and is used from arm's length whatever its size. So a tile, a letter and a
key keep their **physical size**; a larger or sharper panel does not get bigger tiles, it gets **more
tiles**. Everything the firmware draws is therefore defined in a *look* (millimetres, expressed as the
pixels of a reference board) and scaled to the board's pixel density.

## What a board declares

In its board file under `packages/boards/`, next to its hardware:

| Substitution | Meaning |
|---|---|
| `DISPLAY_W`, `DISPLAY_H` | the logical canvas (after LVGL's rotation) |
| `DISPLAY_DPI` | diagonal pixels / diagonal inches (the CYD 2.8″: 143, the Guition 4.0″: 170) |
| `LOOK` | `standard` (the Guition's sizes) or `compact` (the CYD's, for glass too small for the standard) |
| `GRID_COLS`, `GRID_ROWS` | the tile grid; the shared tree places every cell from these |
| `GRID_MARGIN`, `GRID_GAP_X`, `GRID_GAP_Y` | the side margin and the gaps between cells, in pixels |
| the size table (`TILE_W`, `TILE_ICON_SIZE`, `FONT_*_SIZE`, …) | the look's sizes at this board's density |

`tools/propose_grid.py` proposes the grid from the resolution and the diagonal: as many cells as hold a
standard tile of about 33 × 16 mm, never smaller than 30 × 12 mm. `tools/new_board.py` writes a board file
for a new panel from the nearest real board, scaling every size by the density ratio (docs/ADDING_A_BOARD.md).

## The cards are cells of an LVGL grid

`packages/core.yaml` gives the tile area (`tile_scroll`) an LVGL grid layout; `runtime_tiles::grid_bind` fills in
the board's columns and rows as free units (`lv_obj_set_grid_dsc_array`), so LVGL divides the area over the cells
and keeps the gaps (`pad_row`, `pad_column`) and the side margin (the container's padding). `place_page` says only
which cell a card takes and how many it spans: `lv_obj_set_grid_cell(tile, STRETCH, column, span, STRETCH, row, 1)`,
a wide card two columns, a full card the whole page. No coordinate is computed in C++ any more, and the cards grow
by themselves when the page bar goes (the container then reaches the bottom edge, keeping the side margin).

The cards themselves are LVGL widgets, so they live in YAML, and ESPHome has no loop: `tools/generate_cells.py`
writes one file per number of cells (`packages/cells/6.yaml` for a 2 x 3 board) with the cards and the line that
binds them, and a board includes the file for its own grid. A board therefore carries exactly the cards it can
show: a CYD six, a 4 x 4 board sixteen. `tools/check.sh` fails when a file is out of date.

## What the firmware does with it

- `ui::configure(dpi, look)` at boot (`components/smart_display/ui_scale.h`): one scale for every
  size the C++ decides itself, `ui::px(n)`, where `n` is in the reference look's pixels. On the CYD
  and the Guition the scale is exactly 100.
- `ui::large()`: the class of cards and pages is the look's, never a cell's momentary height. (A class
  that flipped when the rows grew reused a clock's numeral labels as tick lines: the lab's crash.)
- `GRID_COLS`/`GRID_ROWS` reach the C++ as build flags; `SLOTS_PER_PAGE` follows, `MAX_PAGES` is
  capped so a screen never holds more than 64 tiles (one dirty bit each: seven pages of nine, four of
  sixteen), and `runtime_tiles::widgets` holds exactly one entry per cell. The add-on (`core.Grid`) and
  the editor (`setGrid`) count with the same rule, so a page, a slot and a tile limit mean the same in
  all three.
- A cell taller than the look's cell height (`ui::cell_height()`) centres its content on it; a cell at
  least twice as tall stacks the icon above the name and state.
- What does not fit is left out: the forecast shows as many day columns as the width holds (five at
  most, none below two), a single clock card drops its date when it has no room beside the dial, a
  wide card gets a control panel only when the panel, the icon and some name fit.
- The icon circle and its place follow `TILE_ICON_SIZE` and `TILE_ICON_Y`; the forecast's current
  conditions block follows the card's text offset.

## Overlays: one frame for every card

`components/smart_display/overlay_card.h` is the frame a card a tap opens gets, and it holds two rules on every
board:

- **The content is never wider than a hand spans** (`ui::control_max_width`, 110 mm of the reference look). A
  thermostat whose − and + sit at the far edges of a ten-inch panel takes two hands. On a CYD and a Guition the
  glass is narrower than the cap, so nothing changes there.
- **What is capped, is centred**: left to right by `overlay_card::frame`, top to bottom by
  `overlay_card::centre(root, pinned)`, which leaves the first `pinned` children (the back key and the name)
  where they are: the card's top bar stays at the top, the content under it sits in the middle.

A card that is a *picture* asks for `overlay_card::picture` and is not capped: the media card's cover art and a
camera's image are nicer the bigger they are. A full-screen backdrop behind the card keeps the page covered.

Anything a finger must hit keeps at least `ui::touch_min()` (7 mm of glass, from the board's density) as its
touch area, however thin it is drawn: `overlay_card::touchable(object, drawn_thickness)` grows the click area
instead of the drawing, so a blind's slider on a small panel stays usable.

## Designing a card or a page here

Every card is drawn on glass we have never seen: 2.8 to 10 inches, 143 to 294 dpi, landscape, portrait
and everything between. A design that was drawn for one panel and then patched for the next is how the
climate card ended up as six tables of pixels, and how a light's effects page pushed its sliders off a
800 x 480 screen. These are the rules that keep that from happening again. They are about **how to
think about a design**, not about which pixel goes where.

**Build a component, not a screen.** A card is two parts: a header of pure arithmetic that says where
everything goes (`media_card.h`, `climate_card.h`, `effects_page.h`'s `place()`), and a draw step in
`runtime_tiles.h` that hangs LVGL objects on those numbers. The arithmetic knows nothing about LVGL or
ESPHome, so `tests/test_*_card.cpp` can check every shape on a PC in a second. If you cannot test a
layout without a board, it is not a component yet.

**Ask the area, never the board.** The only things a layout may read are the width and height it was
given, the density and the look (`ui::`), and the content itself. There is no `if (board == guition)`,
no `if (width == 480)`, and no substitution in a board file that positions something the shared tree
draws. A board file says what the hardware is and how dense it is; everything else is derived.

**Sizes are physical.** `ui::px(n)` is a design size in the reference look's pixels, scaled to this
panel. `ui::mm(n)` is for anything the human body decides: `ui::touch_min()` (7 mm) for what a finger
must hit, `ui::column_gap()` between two columns, `ui::control_max_width()` (110 mm) for a row of
controls that must stay inside one hand's reach. A number that is neither a design size nor a physical
one is usually a mistake.

**Say what may give, and in what order.** A stack that must fit calls `ui::shrink({...}, over)` with
its blocks, each with the least it can be and how much of the stack one of its pixels is worth. The
*order is the design decision*, and every card makes its own: a robot gives up its portrait before its
keys, a thermostat its status word before its modes, the effects page its rows before its sliders.
Write the order down in a comment with the reason. Nothing may ever be drawn past the glass: if the
cascade runs out, the last resort is scrolling or leaving content out, never overflow.

**Let the shape choose the form.** Screens differ more in *proportion* than in size. A card that is one
column on a square panel should stand in two on wide glass, because the height it lacks is width it
has: the cover card's sliders, the effects page's speed and intensity, the media card's art beside its
texts. Decide on the ratio of the area (`width * 2 >= height * 3`), never on a pixel count or a board
name, and keep one code path that both forms come out of.

**Prefer LVGL's own layout where it fits.** The tile grid is an LVGL grid (`lv_obj_set_grid_dsc_array`),
so LVGL divides the page and we only say which cell a card takes. Flex rows with `flex_grow` and
`min_width`/`max_width` in `ui::px()` do the same for a row inside a card. Computed coordinates are for
what LVGL cannot express, not for what is easier to write today. Watch the cost, though: a widget that
clips its children to a rounded corner makes LVGL allocate a layer of tens of kilobytes on every
redraw, which a board without PSRAM cannot pay.

**Reuse the frame.** Anything a tap opens goes through `overlay_card`: the padding, the cap, the
centring, the two-column split and `touchable()` are there so that a new card inherits the rules
instead of restating them. A fix that belongs to all cards belongs in that file, once.

**Text is not a fixed width.** The firmware speaks nine languages, and a German or Polish label is
often half again as long as its English original ("Standby" against "Bereitschaftsmodus"). So: never
size a column to an English word, ask the font for the line height instead of assuming one
(`lv_font_get_line_height`), give every label a long mode (dots or scroll) and enough room that the
dots are rare, and prefer a layout that can take a longer word over one that looks perfect in English.
Check a card in English and in one long language before calling it done. The words themselves come
from `screen_text` (`docs/TRANSLATING.md`); state words, units and entity names come from Home
Assistant and are never composed in the firmware.

**Prove it before you flash it.** A new card ships with a test that walks every shape it can land on
(zero to six rows, one to three columns, 240 x 320 to 1024 x 600) and asserts that nothing leaves the
area and nothing a finger needs falls under `ui::touch_min()`. Then render it on the lab boards
(`.esphome/readme-render/responsive-lab/lab_render.py`) and look at it. A test says it fits; only the
render says it is worth looking at.

## What the screen tells the add-on

A screen reports two diagnostic sensors (firmware 0.2.79+): **Screen layout**, `800x480 3x3 217dpi standard`
(the canvas after rotation, the grid, the density, the look), and **Screen board**, the key of its board file.
The add-on (`core.shape_of`, `core.grid_of`) takes what the screen says first, then the board the screen's
profile YAML builds from (`screen_manager/app/boards.json`, written from the board files by
`tools/generate_board_shapes.py`), and the smallest screen there is when it knows nothing. Firmware from
before the sensors says nothing, and every screen that ran it is a two by three board. The editor draws the
mockup at that aspect with that grid and the top bar at that density, and places tiles on it; a save, a tile
event and the layout sensor count rows, columns and pages the same way.

## What is still open, and the way it becomes durable

- The sizes in `runtime_tiles.h` and the metric tables of the settings, effects, media and light cards go
  through `ui::px()` one by one. The durable form is LVGL's own: flex rows and columns with `flex_grow`,
  `min_width`/`max_width` in `ui::px()`, and `LV_EVENT_SIZE_CHANGED` for a card that changes shape with
  its width. The first component to rebuild that way is the tile row (circle | text column | panel); the
  forecast strip and the clock follow.
- The overlays that take pixels from the board file (light, alert, touch test) keep those substitutions;
  they become computed cards like the thermostat, the blind and the robot.
- A card with two groups (light: brightness and colour; climate: setpoint, modes, fan) could stand in two
  columns on wide glass instead of one capped column. Same components, another flex flow.
- Turning follows the shape (firmware 0.2.79+): a half turn keeps width, height, the grid and the whole size
  table, so every board offers it; a quarter turn only a square screen (`settings_screen::quarter_turns`,
  set from `DISPLAY_W == DISPLAY_H` at boot). The shared tree applies the angle on top of the board's own
  `LVGL_ROTATION` (a CYD starts at 90), and each board's Rotation select offers the angles its glass allows.
- The lab boards (`packages/boards/lab-*.yaml`) are generated and disposable; a real board gets a hardware
  section checked on glass and an entry in `tools/profiles.py`.
