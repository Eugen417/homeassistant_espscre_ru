# Test results app 0.2.84 / firmware 0.2.70 (2026-09-18)

Max asked whether the two board profiles carried much double code ("hebben we veel dubbele code voor de beide
schermen?"), wanted one shared package with only the board differences apart ("zodat we ook makkelijker een scherm
kunnen toevoegen later"), and set the bar for this round: a refactor that can be verified afterwards, one to one, with
the same end result ("Ik wil echt dat je fully 100% zeker bent dat het precies hetzelfde eindresultaat is"). The rows
per page stay as they are (his call: step 1 only).

## What changed

- `packages/core.yaml`: everything the two profiles shared (4,283 lines). No number in it depends on the board.
- `packages/boards/cyd-2432s028.yaml` (738 lines) and `packages/boards/guition-4848s040.yaml` (1,015 lines): the
  hardware, the table of 84 sizes and font sizes, the 18 hooks (the lines of C++ a board adds inside a shared lambda,
  as substitutions whose values are the old profile's exact text), and the board's own sections and scripts.
- `packages/cyd.yaml`, `packages/guition.yaml`, `home-like-2432s028.yaml`, `guition-4848s040.yaml`: entry files of
  15-45 lines that include the core and the board file. The two under `packages/` keep their names, so every screen's
  own YAML (`files: [packages/<board>.yaml]`) builds as before.
- `tools/check_packages.py` replaces `tools/generate_packages.py` in `tools/check.sh` and CI; `tools/profiles.py` gives
  the tests and tools the three files as one text (`text`), with the substitutions filled in (`resolved`), or without
  the substitution blocks (`merged`). Twenty test files read the profiles through it; `tools/generate_icons.py` writes
  the glyph lists into the core only; `tools/render_topbar.py` takes the fonts from the core at each board's sizes;
  `tools/check.sh --firmware` links `packages/` next to the check copies of the profiles.
- Docs: docs/PROFILES.md (new), pointers in AGENTS.md, RELEASING.md, GUITION.md, THEME.md, SETTINGS.md, CAMERA.md.

## How the split was made

A script (kept out of the repository) took the old CYD profile as the base of the core and replaced every size by its
`${NAME}`, asserting the value it replaced in *both* old profiles: 195 places, 84 names. Every hook's text was cut from
the old profiles by line. The Guition's tile labels (`height`, `width: 130`, `long_mode: DOT` on twenty labels) are
`!extend` blocks in its board file; its alert picture frame, `camera_image_seed` and `climate_settings_card` likewise.

## The one-to-one check

Before the split, `esphome config` and `esphome compile --only-generate` (ESPHome 2026.6.2) were run for both boards
along two routes: the checkout profile, and a screen YAML as `core.installation_yaml()` writes it, with
`packages: display: {url: file://<checkout>, ref: <commit>, files: [packages/<board>.yaml]}`: the GitHub route the
add-on uses, against the old commit. After the split the same four builds were run against the new commit, and
compared:

- `esphome config`, read as data: every scalar, every lambda body and every component is the same, apart from
  whitespace inside a lambda where an empty hook leaves a blank line;
- `main.cpp`, with `#line` directives, blank lines, leading whitespace and ESPHome's numbered ids taken out, compared
  as a sorted multiset of lines: the Guition's is identical except the lines that carry ESPHome's component-table
  indices and the numbers of two LVGL event triggers (the picture frame and the OK button swapped places); the CYD's
  differs by those indices and by one `StatelessLambdaAction` fewer (the boot step that set the calibration wizard up
  is now the tail of the boot step that sets the cards' hooks up: `${BOOT_CARDS_TAIL}`, the same statements in the
  same order, one lambda instead of two consecutive ones);
- every other generated source file (361 for the CYD, 401 for the Guition; `build_info_data.cpp` with its build
  timestamp left out): identical, byte for byte;
- `platformio.ini`: identical on the CYD; the Guition's two `build_flags` in the other order.

What the merge changes is the order in which ESPHome declares and registers fonts, entities, scripts, API actions,
styles, includes and build flags (the core's first, the board's after). That order is what the component tables
number; nothing that runs in sequence moved. The two z-order changes on the Guition (the picture frame after the OK
button, the hidden image seed after the labels) concern widgets that never overlap or are never shown.

So the firmware version stays 0.2.70, and no screen is offered an update.

## Automated

- `tools/check.sh`: Python 466 tests (`tests/test_easy_package.py` rewritten for the new layout), C++ 19/19, the package
  check, the icon check, Vitest, vue-tsc, build and the bundle: 10 passed, 0 failed.
- `tools/check.sh --firmware` (ESPHome 2026.6.2): CYD 1,676,912 B = 91.4 % of the 1,835,008 B slot (158,096 B free),
  Guition 2,054,080 B = 25.3 %. Against the old profiles built the same way on the same computer (CYD 1,675,472 B,
  Guition 2,052,544 B; the 0.2.83 release noted 1,675,424 B for the CYD from another build): +1,440 B and +1,536 B.
- Where those bytes are (`xtensa-esp32-elf-size -A`, `nm -S` on both ELFs): every section is the same size except
  `.flash.text` (+1,432 B on the CYD, +1,528 B on the Guition; `.dram0.bss` 16 B smaller on the CYD: the lambda object
  that went). Of the symbols that changed size, ESPHome's generated `setup()` accounts for the growth (+1,412 B on the
  CYD): the components are registered in the merge's order, which changes its literal pool. The lambdas are the same
  functions under other numbers (`{lambda()#75}` +1,259 B where `{lambda()#56}` −1,259 B), net +19 B on the CYD and
  −18 B on the Guition from the line numbers ESPHome's log macro bakes in; the font arrays and every string are the
  same size, renamed.

## Not done

- No hardware was flashed: the check above is the proof that a screen would run the same code; Max's screens keep
  0.2.70 as they are.
- Left for a later round, each a change of what the CYD runs and therefore not part of this one: the CYD could drive
  its backlight through the Guition's `set_backlight` script (then `alert_dismiss`, `alert_flash` and `wake_display`
  become shared); the camera hooks `camera_close()`, `camera_visible()`, `alert_prepare()`, `alert_clear()` and
  `camera_tick()` exist as no-ops on a board without images, so the core could call them on every board and five
  hooks would go; the portrait layout in `OPEN_VALUE_OVERLAY_TAIL` could move into the CYD's `climate_card_refresh`.
  The tiles per page (`SLOTS_PER_PAGE`, two columns of three) stay fixed in the firmware, the add-on and the editor.
