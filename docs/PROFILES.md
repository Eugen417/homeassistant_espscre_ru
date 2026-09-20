# One screen, three files: the shared core and the board files

Since app 0.2.84 a screen's ESPHome configuration is one shared file plus one file per board, put together by
ESPHome's own [packages](https://esphome.io/components/packages/) mechanism. Before, each board was a 5,000-line
profile of which 87 % was the same text as the other board's; every change had to be made twice and the two had
already drifted apart in small ways.

| File | What it holds |
|---|---|
| `packages/core.yaml` | Everything every board shares: the LVGL tree (tiles, cards, overlays, the touch test page), the scripts, the API actions, the entities, the fonts, the globals, and Wi-Fi that never dozes (`power_save_mode: none`, firmware 0.2.74+; the network itself stays in the screen's own YAML). It contains no number that depends on the board: every such value is a `${NAME}` the board file defines. |
| `packages/boards/cyd-2432s028.yaml` | The CYD: its hardware (ILI9341 over SPI, XPT2046 resistive touch, backlight, LED), its sizes and font sizes, the code the shared lambdas take from it (the *hooks*, below), and what only a CYD has: the calibration wizard, a page swipe by LVGL's gesture, `ui_self_test`, the two scripts that drive the backlight as a light action, its climate card layout. |
| `packages/boards/guition-4848s040.yaml` | The Guition: its hardware (ST7701S RGB, GT911, PSRAM), its sizes, its hooks, and what only a Guition has: camera images (`online_image`, the alert's picture frame), the Rotation entity, the page swipe from the glass edge, the snapshot diagnostics, the climate card with its fan and swing card, the wider tile labels (`!extend` on each tile). |
| `packages/cyd.yaml`, `packages/guition.yaml` | What a screen installed from ESP Screens builds from over GitHub, unchanged in name and place: ESP Screen Manager writes every screen's YAML with `files: [packages/<board>.yaml]`. Each is a few lines: where the fonts are, the components from GitHub, and `packages: {core: !include core.yaml, board: !include boards/<board>.yaml}`. |
| `home-like-2432s028.yaml`, `guition-4848s040.yaml` | The same two files for a build from a checkout (the bench, the manual route of README_EXTENDED.md): the components of the checkout, the fonts from `fonts/`, and the secrets from `secrets.yaml`. |

ESPHome merges them in this order: the core, then the board file on top of it, then the entry file on top of both.
A mapping merges key by key, a list of components with ids merges by id (`!extend` adds to an existing widget or
script, `!remove` takes one away), any other list is the core's items followed by the board's, and a plain value of
the board file replaces the core's. `esphome config home-like-2432s028.yaml` shows the result.

## The three ways a board differs from the core

1. **A size, a font size, a name.** `${TILE_ICON_SIZE}`, `${FONT_HEADLINE_SIZE}`, `${TOUCH_TEST_TITLE}`: the core
   writes the placeholder, the board file's `substitutions:` table gives the value. `tools/check_packages.py` fails
   when a board leaves one out, so a new board cannot forget a size.
2. **A stretch of C++ inside a shared lambda.** The Guition sets `runtime_tiles::rotation_supported`, closes its
   camera when the cards close, prepares the alert's picture frame; the CYD sets its calibration up and lays the
   climate card out for a portrait panel. Each such stretch is a *hook*: a substitution whose value is that board's
   exact lines, written where the old profile had them (`${BOOT_ENABLED_HOOK}`, `${CLOSE_CARDS_HOOK}`,
   `${APPLY_ROTATION}`, ...). A board that adds nothing there names the hook with an empty value. The hooks sit at
   the end of the board file's `substitutions:` table, each with a line that says where it lands.
3. **A whole section or script.** Hardware, `online_image`, the Rotation select, `capture_ui_snapshot`: these are
   the board file's own sections, merged next to the core's. `alert_dismiss`, `alert_flash` and `wake_display` are
   in both board files because their *steps* differ (a `light.turn_on` action on the CYD, the `set_backlight` script
   on the Guition); so are `ui_self_test` and `climate_card_refresh`, whose layouts differ by design.

## Adding a board

1. Copy the board file of the panel that resembles the new one most (a small SPI panel: the CYD; a big RGB panel
   with capacitive touch: the Guition) to `packages/boards/<board>.yaml` and change its hardware sections. Keep
   `assertion_level: SILENT` in its `esp32:` block (firmware 0.2.75+): every board builds its firmware the same way,
   and `tests/test_easy_package.py` checks it.
2. Fill the sizes table for the new resolution, and go through the hooks: keep, change or empty each one.
3. Add `packages/<board>.yaml` and `<board>.yaml` after the existing entries, `<board>` to `BOARDS` and `ENTRIES`
   in `tools/profiles.py` and to `REFS` in `screen_manager/app/core.py` (the boards `installation_yaml()` writes a
   profile for), and the board to the editor's New screen. docs/ADDING_A_BOARD.md is the whole recipe, the grid
   included: since app 0.2.93 a board declares its grid, its density and its look, and the shared tree, the add-on
   and the editor follow (docs/RESPONSIVE.md).
4. `python3 tools/check_packages.py`, `esphome config <board>.yaml`, `tools/check.sh --firmware`.

## How the split was verified

The split was made by a script that replaced every size in the old CYD profile and asserted the value it replaced
in both old profiles (195 places, 84 names), and took every hook's text from the old profiles by line. Then the
result of `esphome config` and the generated `main.cpp` of both boards, through the checkout entry and through the
GitHub package route, were compared with the same of the old profiles:

- every scalar, every lambda body and every component is the same, apart from whitespace and `#line` directives;
- on the CYD, the calibration setup runs in the same lambda as the effects page's hooks instead of a lambda of
  its own (the same statements, in the same order: `${BOOT_CARDS_TAIL}`);
- on the Guition, the alert's picture frame is created after the OK button instead of before it (they never
  overlap), and the hidden `camera_image_seed` after the labels;
- the order in which ESPHome declares and registers fonts, entities, scripts, API actions, styles, includes and
  build flags follows the merge (the core's first, the board's after), which changes the numbers in ESPHome's
  component tables and nothing that runs in sequence.

Every other generated source file of both builds was identical, byte for byte.
