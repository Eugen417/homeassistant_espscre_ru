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

## What the firmware does with it

- `ui::configure(dpi, look)` at boot (`components/smart_display/ui_scale.h`): one scale for every
  size the C++ decides itself, `ui::px(n)`, where `n` is in the reference look's pixels. On the CYD
  and the Guition the scale is exactly 100.
- `ui::large()`: the class of cards and pages is the look's, never a cell's momentary height. (A class
  that flipped when the rows grew reused a clock's numeral labels as tick lines: the lab's crash.)
- `GRID_COLS`/`GRID_ROWS` reach the C++ as build flags; `SLOTS_PER_PAGE` follows, `MAX_PAGES` is
  capped so a screen never holds more than 64 tiles (one dirty bit each). `runtime_tiles::place_page`
  computes every cell's place and size from the margin, the gaps and the grid; a wide card spans two
  cells, a full card the page. The tile widgets in `packages/core.yaml` (twenty today) are the pool a
  board's grid draws from.
- A cell taller than the look's cell height (`ui::cell_height()`) centres its content on it; a cell at
  least twice as tall stacks the icon above the name and state.
- What does not fit is left out: the forecast shows as many day columns as the width holds (five at
  most, none below two), a single clock card drops its date when it has no room beside the dial, a
  wide card gets a control panel only when the panel, the icon and some name fit.
- The icon circle and its place follow `TILE_ICON_SIZE` and `TILE_ICON_Y`; the forecast's current
  conditions block follows the card's text offset.

## What is still the lab's, and the way it becomes durable

- The 157 sizes in `runtime_tiles.h` and the metric tables of the settings, effects, media and light
  cards are wrapped in `ui::px()` one by one. The durable form is LVGL's own: flex rows and columns
  with `flex_grow`, `min_width`/`max_width` in `ui::px()`, and `LV_EVENT_SIZE_CHANGED` for a card that
  changes shape with its width. The first component to rebuild that way is the tile row (circle |
  text column | panel); the forecast strip and the clock follow.
- The overlays that still take pixels from the board file (light, climate, alert, mode picker, touch
  test) keep those substitutions; a lower canvas than the look's needs the board generator to fit
  them. They become computed cards like settings and effects.
- Control overlays are to be capped at about 110 mm of width and centred; media and camera fill.
- The lab boards (`packages/boards/lab-*.yaml`) are generated and disposable; a real preset gets a
  hardware section checked on glass.
