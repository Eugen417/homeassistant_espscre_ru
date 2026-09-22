# Adding a board

The recipe, in the order the work actually goes: the hardware is looked up, the layout is computed, and only the
grid is a choice. `docs/RESPONSIVE.md` says why the layout works the way it does.

## 1. The board's details

Its name and SKU, its resolution, its diagonal in inches, its chip (ESP32, S3, P4), how much flash and PSRAM it
has, and what its touch panel is. The shop page or `devices.esphome.io` has all of it.

## 2. Look it up in ESPHome

ESPHome supports many panels by name, and then one line sets the pins, the timings and the dimensions:

| Driver | For | Example models |
|---|---|---|
| `mipi_spi` | small SPI panels | `ESP32-2432S028` (the CYD), `JC4827W543`, `WT32-SC01-PLUS`, `T-DISPLAY-S3` |
| `mipi_rgb` | 16-bit parallel panels | `GUITION-4848S040`, `ESP32-S3-TOUCH-LCD-4.3`, `ESP32-S3-TOUCH-LCD-7-800X480`, `ESP32-8048S070`, `WAVESHARE-5-1024X600` |
| `mipi_dsi` | ESP32-P4 panels | `WAVESHARE-P4-86-PANEL`, `M5STACK-TAB5`, `JC8012P4A1` |

A board that is not in those lists needs its pins, its init sequence and its timings from the manufacturer's
example, which is the one genuinely difficult part of a new board.

## 3. A base configuration from ESPHome

Start from the board of the same family (`packages/boards/cyd-2432s028.yaml` for a small SPI panel,
`guition-4848s040.yaml` for a big parallel one) and replace its hardware sections: `display`, `touchscreen`,
`output`/`light` for the backlight, `i2c`/`spi`, `esp32` (variant, flash size) and `psram`. Everything else in a
board file is layout, and step 4 computes it.

One number to look at on a parallel (RGB) panel: `LVGL_BUFFER_SIZE`. The picture lives in PSRAM, but LVGL's draw
buffer lives in the memory inside the chip, next to Wi-Fi, the API and the panel's bounce buffers, and a quarter of
800 × 480 is 192 KB of it. The Waveshare 4.3 ran on 15 KB free that way and hung under a large layout; at 12 % it
boots with 112 KB. Read `sensor.<screen>_heap_free` after the first boot with a full layout: under 40 KB is too
little.

## 4. The layout: two grids and a table that follows from them

```
python3 tools/propose_grid.py             # what grid the resolution and the diagonal ask for, each way up
python3 tools/new_board.py <name> --from guition --width 800 --height 480 --inch 4.3 [--cols 3 --rows 2] \
                                  [--portrait-cols 1 --portrait-rows 4] [--rotation 0|90|180|270]
python3 tools/generate_cells.py           # the cards of that grid
```

`--width` and `--height` are the canvas of the screen lying down and `--rotation` the LVGL angle that lays the
panel out that way, so the board file states the panel's own pixels (`PANEL_W`, `PANEL_H`) and that angle
(`ROTATION_LANDSCAPE`). It then states its density and its look (`DISPLAY_DPI`, `LOOK`), its grid the two ways the
screen can hang (`GRID_COLS`, `GRID_ROWS`, `GRID_COLS_PORTRAIT`, `GRID_ROWS_PORTRAIT`, `GRID_MARGIN`, `GRID_GAP_X`,
`GRID_GAP_Y`) and the size table, every value scaled from the reference board by the density ratio, so a tile, a
letter and a key keep their size in millimetres.

The grid is the one real choice: the proposal keeps a tile at about 33 × 16 mm and never smaller than 30 × 12 mm,
but a board of the same size can be read as "more tiles" or "bigger tiles". Render both and look. It is two
choices on glass that is not square, because a card keeps its size in millimetres: a screen that holds three
columns lying down may hold one standing up, and ESP Screens offers the owner both when the screen is built.

Two things to weigh for the standing grid. `tools/generate_cells.py` gives a board the cards of whichever of its
two grids is larger, so a standing grid with more cells than the lying one costs every screen of that board those
extra cards, whichever way it hangs; the four boards that ship are all at the larger of their two, so none of them
pays for the second way round. And a screen holds 64 tiles in all, one dirty bit each, so a page of 32 cells leaves
room for two pages and a page of 44 for only one. That is why the 10.1-inch stands at four columns of five rather
than the four of eleven the millimetres would allow.

State no number in the board file that follows from the canvas. The tile area, the cells, the page keys, the strip
that opens the settings, the crosses of the touch test and the card of an alert are all measured at boot from the
canvas LVGL hands the screen. That is what lets one firmware serve the board either way round, and a board file
that states such a number would be right one way and wrong the other.

## 5. Look at it before it ever reaches the glass

The host renders the real firmware into an SDL window: every page, the settings, every card, the alert, and the
firmware's own geometry test (`ui_self_test`) with the page bar on and off. That catches a cramped forecast, a
clipped name or a card that falls outside its area without a board on the desk.

## 6. Then the board itself

Flash it once: touch (the corners and a drag), the backlight, the colour order, the rotation, and a page switch.
What a render cannot show is exactly what the hardware check is for. Then flash it standing up, which is the same
build with `LVGL_ROTATION` a quarter further than `ROTATION_LANDSCAPE`, and walk the same list again.

## 7. Write it down

Say what the backlight can do, in two flags every board file carries: `BACKLIGHT_DIMMABLE` (a PWM pin takes levels;
a line on an expander is lit or dark, and the percentages become switches) and `CAN_STANDBY` (the screen can go dark
at all). A board that cannot go dark - the Waveshare browns out when its backlight boost switches on again - has no
standby and no night: the firmware hides those rows, the board file keeps their entities internal with `!extend`
(copy the block at the end of `packages/boards/waveshare-esp32s3-43.yaml`), `tools/generate_board_shapes.py` writes
both flags into boards.json for the add-on, and `tests/test_easy_package.py` keeps the flag and the list together.

A screen also says what it can do while it runs, in its **Screen features** sensor (firmware 0.2.99): one word per
ability, so the add-on follows the screen itself and only falls back to boards.json for firmware from before that
sensor and for a screen that is offline. That matters for a board that was changed after it was built: a Waveshare
whose backlight was rewired to a PWM pin (`docs/WAVESHARE7.md`) really does dim, and a table per board would keep
saying it cannot. An ability is one row on each side: a line in the `features` list of the sensor in
`packages/core.yaml`, and a row in `FEATURES` in `screen_manager/app/core.py` naming the boards.json key it falls
back to. A word the add-on does not know is skipped, so new firmware may report one an older add-on never heard of.

Give the board file its own `BOARD_ID` (the screen reports it) and a `Rotation` select with the angles its glass
allows (the half turn; the quarter turns as well when it is square; copy the block of the nearest board,
`tools/check_packages.py` checks it), and add the board to `packages/<board>.yaml` and
`<board>.yaml` (the two entries), to `BOARDS` and `ENTRIES` in `tools/profiles.py`, to `REFS` in
`screen_manager/app/core.py`, and to the editor's New screen (`web/src/components/InstallerView.vue` and the
`editor.installer.board_<id>` text in every translation). Then `tools/generate_cells.py` (the cards of its grid,
which go into Git with it), `tools/generate_board_shapes.py` (what the add-on and the editor know of it) and
`tools/check.sh --all`, which from then on checks and compiles it with the others.
