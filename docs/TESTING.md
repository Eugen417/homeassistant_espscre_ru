# Testing a change to the screens

A test counts when it walks the path the product walks and looks at the moment a bug would live in. A function called
directly, or a screen looked at once it has settled, can pass while the screen on the wall shows the bug for a second
after every page change (GitHub #27 was exactly that). So before calling a change tested, say which entry point was
driven (a finger, a layout message from the add-on, an action from Home Assistant) and which moment was looked at.

There are five levels, from fast to real. Each is a different check: a green level does not stand in for the next.

## 1. The fast checks: `tools/check.sh`

One command, the same on a laptop and in CI (`.github/workflows/ci.yml` runs it on every push and pull request to
main). On a development machine run it with `PYTHON=.venv-portal/bin/python`. It needs aiohttp, PyYAML, Pillow,
fontTools and jinja2, and stops when one is missing, because a skipped test proves nothing. In order:

- **Python tests** (`python -m unittest discover -s tests`): the add-on, the tools, the catalogue, the release rules
  (`tests/test_release_lint.py`), the font set (`tests/test_font_set.py`), the saved layouts of 0.4.31
  (`tests/test_compat_0431.py`), and `tests/test_layout_audit.py`, which lays out every card kind, size and board
  through the firmware preview and checks where every object and text ended up, without drawing a picture.
- **C++ tests**: every `tests/*.cpp`, compiled with `clang++ -std=c++17 -Wall -Wextra -Werror -I.` and run. They test
  the firmware's logic and layout arithmetic (cards, settings, theme, touch filter, protocol) on this computer.
- **Generated files are current**: `tools/check_packages.py` (the boards define every name the core uses, and no board
  file repeats what it would get anyway), and the `--check` of `tools/generate_cells.py`, `generate_board_shapes.py`,
  `generate_entries.py`, `generate_issue_templates.py`, `generate_icons.py` and `generate_catalogue.py`.
- **Firmware numbers** (`tools/affected_boards.py --verify`): a change that reaches a board raises that board's
  number (docs/BOARD_RELEASES.md).
- **Home Assistant's facts**: `catalogue/_ha.json` and `catalogue/_remote_commands.json` are snapshots read from Home
  Assistant's source. With `HA_CORE` naming a checkout of home-assistant/core, the check reads them again and fails
  when Home Assistant changed (docs/CATALOGUE.md, "When Home Assistant changes"). Without it the snapshots are taken
  as they are.
- **Translations** (`tools/i18n.py check`, `header --check` and `lint`): every language against English, the key
  header the firmware builds against, and no English left in the firmware's code.
- **The editor**: `npm ci`, the Vitest suite, the type check and the build, and whether the bundle in Git equals that
  build.
- **The firmware preview**, the shared firmware compiled to WebAssembly for the editor: whether it is older than the
  firmware sources, and its tests (`web/wasm/test_*.mjs`) at three glass shapes. `.github/workflows/preview.yml`
  rebuilds and commits it when the firmware sources move; locally, `sh web/wasm/build.sh` with Emscripten does the
  same.

## 2. Firmware builds

`tools/check.sh --firmware` compiles boards the way their owners build them: from the checkout entries, with
placeholder secrets, and with the Wi-Fi fallback hotspot only where users get it. It has ESPHome read every override
from `tests/fixtures/overrides/` on its board, and applies the CYD's flash budget (docs/RELEASING.md step 2).

- `--affected` builds the boards the change reaches (`tools/affected_boards.py --build-keys`); when that is every
  board, it builds the sample of four in `tools/profiles.py` `SAMPLE` instead (the CYD and the Guition always, and two
  that differ in chip, flash or glass). `--affected --every-board` builds them all.
- `--sample` builds the sample directly, and `--board <key>` (repeatable) one board.
- CI's firmware job builds with the ESPHome the add-on ships and with the packages' `min_version`, on the boards the
  push reaches, and every board every night and when started by hand.

A build that passes says the YAML and the C++ compile and fit their slot. It says nothing about what the glass shows.

## 3. Every board on this computer: renders, by hand

`tools/render/run.py` builds the real firmware of a board as a program for this computer: the same C++ and LVGL as on
the glass, with an SDL surface in place of the panel and the touch chip. Lying down, and standing up where the glass is
not square. `tools/check.sh --render` does it for every board, and `tools/check.sh --render --sample` for the three of
`RENDER_SAMPLE` in `tools/profiles.py` (the smallest, a middle and the largest glass). It needs SDL2 and ESPHome's
Python and takes long, so it runs by hand when a change reaches what a screen draws. CI does not run it. For each
variant it:

- sends the demo layout through the screen's inbox, as the add-on does, with a title per page;
- runs the firmware's own self test (`ui_self_test`): every page and overlay, each page's cards and bar, and the check
  that nothing falls outside its area. A FAIL fails the run;
- changes page with a finger: from the edge of the glass across the tiles and back, through ESPHome's own touchscreen
  (its transform, the touch guard, the edge swipe or LVGL's gesture), read at the rate the board reads its touch panel;
- reads back, every 50 ms from the moment the finger lifts, the page title as its label shows it: a title that stands
  in dots for a moment and then whole is a failure;
- shows the alerts and one with a camera picture, and reads the card back as it opens, before the picture, and right
  after the picture arrives: every part inside the card, nothing over anything else;
- saves every page, the alerts and Dark mode as PNGs under `.esphome/render/out`, with a sheet of all of them. To
  compare with an earlier commit, render a checkout of it with `--tree <checkout> --out .esphome/render/base` and run
  `tools/compare_renders.py` on the two folders.

What it cannot show: colours and timing of a real panel, a real finger on real glass, memory and heat, and anything a
real Home Assistant or camera adds.

## 4. One screen on the glass

`diagnostics/send_layout.py` pushes the same demo layout to a screen over its API; `diagnostics/run_ui_test.py` runs
its self test (don't touch the screen meanwhile); `diagnostics/capture_ui.py` saves what LVGL draws as a picture. A
new board is flashed and walked through by hand: touch in the corners, a drag, the backlight, the colour order, a page
swipe from each edge, and the same standing up (docs/ADDING_A_BOARD.md). A resistive panel's calibration is tapped by
a person (docs/CALIBRATING.md); a GT911 panel is checked with `tools/verify_gt911.py`.

Only the glass shows: touch accuracy under a real finger, the panel's colours, flicker and stripes, the backlight,
standby and the wake from dark (a brownout shows only there), free heap after hours with a full layout, and Wi-Fi under
load. A count read over the API is not such a result; an eye on the glass is.

## 5. The whole chain

The release a user gets, from end to end, on a screen of its own (not one a household depends on):

1. Update ESP Screen Manager in Home Assistant to the release.
2. Add the screen with **New screen**. With the board on the Home Assistant machine, flash it there; otherwise choose
   **Download** and flash the file from your own computer.
3. Pair it: Home Assistant finds the screen, and with ESPHome Device Builder installed it takes the key from the
   screen's YAML by itself.
4. Give it tiles in ESP Screens, with real entities, and look at the glass.
5. Send an alert the way an automation does, `esp_screens_show_alert` with a `camera`: every screen gets the picture at
   the size of its own frame, and older firmware its old frame. It goes to every screen, so choose a moment for it.
6. Swipe, tap the alert's button, open a camera. Leave tiles that switch real things alone unless that is the test.

A release says in its CHANGELOG entry which of these levels were run, on which boards, and what they showed.
