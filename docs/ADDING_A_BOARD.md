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
| `mipi_rgb` | 16-bit parallel panels | `GUITION-4848S040`, `ESP32-S3-TOUCH-LCD-4.3`, `ESP32-8048S070`, `WAVESHARE-5-1024X600` |
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

## 4. The layout: two numbers and a table that follows from them

```
python3 tools/propose_grid.py             # what grid the resolution and the diagonal ask for
python3 tools/new_board.py <name> --from guition --width 800 --height 480 --inch 4.3 [--cols 3 --rows 2]
python3 tools/generate_cells.py           # the cards of that grid
```

The board file then states its density and its look (`DISPLAY_DPI`, `LOOK`), its grid (`GRID_COLS`, `GRID_ROWS`,
`GRID_MARGIN`, `GRID_GAP_X`, `GRID_GAP_Y`) and the size table, every value scaled from the reference board by the
density ratio, so a tile, a letter and a key keep their size in millimetres.

The grid is the one real choice: the proposal keeps a tile at about 33 × 16 mm and never smaller than 30 × 12 mm,
but a board of the same size can be read as "more tiles" or "bigger tiles". Render both and look.

## 5. Look at it before it ever reaches the glass

The host renders the real firmware into an SDL window: every page, the settings, every card, the alert, and the
firmware's own geometry test (`ui_self_test`) with the page bar on and off. That catches a cramped forecast, a
clipped name or a card that falls outside its area without a board on the desk.

## 6. Then the board itself

Flash it once: touch (the corners and a drag), the backlight, the colour order, the rotation, and a page switch.
What a render cannot show is exactly what the hardware check is for.

## 7. Write it down

Say what the backlight can do, in two flags every board file carries: `BACKLIGHT_DIMMABLE` (a PWM pin takes levels;
a line on an expander is lit or dark, and the percentages become switches) and `CAN_STANDBY` (the screen can go dark
at all). A board that cannot go dark - the Waveshare browns out when its backlight boost switches on again - has no
standby and no night: the firmware hides those rows, the board file keeps their entities internal with `!extend`
(copy the block at the end of `packages/boards/waveshare-esp32s3-43.yaml`), `tools/generate_board_shapes.py` writes
both flags into boards.json for the add-on, and `tests/test_easy_package.py` keeps the flag and the list together.

Give the board file its own `BOARD_ID` (the screen reports it) and a `Rotation` select with the angles its glass
allows (the half turn; the quarter turns as well when it is square; copy the block of the nearest board,
`tools/check_packages.py` checks it), and add the board to `packages/<board>.yaml` and
`<board>.yaml` (the two entries), to `BOARDS` and `ENTRIES` in `tools/profiles.py`, to `REFS` in
`screen_manager/app/core.py`, and to the editor's New screen (`web/src/components/InstallerView.vue` and the
`editor.installer.board_<id>` text in every translation). Then `tools/generate_cells.py` (the cards of its grid,
which go into Git with it), `tools/generate_board_shapes.py` (what the add-on and the editor know of it) and
`tools/check.sh --all`, which from then on checks and compiles it with the others.
