# One firmware, any board: how the screens fit their glass

Work in progress on the branch `responsive-lab` (September 2026). The plan and the renders behind it:
the report *ESP Screens op elk bord* and the *Responsive lab renders* gallery (Max's artifacts).
Nothing here is released; both real boards render pixel-identical to the 0.2.85 README renders with
every change on this branch, which is the rule for everything that follows.

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

`tools/propose_grid.py` (lab, still under `.esphome/readme-render/responsive-lab/`) proposes the grid
from the resolution and the diagonal: as many cells as hold a standard tile of about 33 × 16 mm, never
smaller than 30 × 12 mm, and dense packing on three or more columns. `make_board.py` writes a board
file for a new panel from the nearest real board, scaling every size by the density ratio.

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
  capped so a screen never holds more than 64 tiles (one dirty bit each), and `runtime_tiles::widgets`
  holds exactly one entry per cell.
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

## What is still open, and the way it becomes durable

- The 157 sizes in `runtime_tiles.h` and the metric tables of the settings, effects, media and light
  cards go through `ui::px()` one by one. The durable form is LVGL's own: flex rows and columns with
  `flex_grow`, `min_width`/`max_width` in `ui::px()`, and `LV_EVENT_SIZE_CHANGED` for a card that
  changes shape with its width. The first component to rebuild that way is the tile row (circle |
  text column | panel); the forecast strip and the clock follow.
- The add-on and the editor still assume two columns of three (`SLOTS_PER_PAGE`, the drop rules, the
  mockup). Every board that ships today is 2 x 3, so nothing is wrong; a board with another grid needs
  them to take the grid from the screen, which the screen can report (`DISPLAY_DPI`, `LOOK`,
  `GRID_COLS`, `GRID_ROWS` are all it takes).
- The climate card is still a table of pixels per board (six layouts in the board file) and the compact
  look still sends extra modes to the old Mode page. It should become a computed card like the settings
  page: one card for every board, with the modes as a row of choices beside the fan and swing rows, and
  no Mode page at all. That is the next card to rebuild, and the reason a CYD still shows the old picker.
- The other overlays that take pixels from the board file (light, alert, touch test) keep those
  substitutions; they become computed cards too.
- A card with two groups (light: brightness and colour; climate: setpoint, modes, fan) could stand in two
  columns on wide glass instead of one capped column. Same components, another flex flow.
- The lab boards (`packages/boards/lab-*.yaml`) are generated and disposable; a real preset gets a
  hardware section checked on glass.
