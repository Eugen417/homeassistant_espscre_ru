# Test results app 0.2.127 / firmware 0.2.102 (2026-09-23)

A screen's files are built from shared parts: the core, one look, the features a board has, hardware several boards
share, and a short board file (docs/PROFILES.md). The rule for this release: every board builds the firmware it built
before. This is what was checked, and how.

## The same firmware, through every way a screen is built

For every board that ships, the files of app 0.2.126 (f92ba68) and of this release were each put through ESPHome, and
the results compared by a script:

- **Routes.** The checkout entry (`<board>.yaml` in the root, with placeholder secrets, as `tools/check.sh` builds it);
  the YAML ESP Screen Manager writes for a screen (`core.installation_yaml`), with the package fetched from a Git
  clone of each version, as a screen fetches it from GitHub; the same standing up, for every board whose glass is not
  square; and that YAML with each override from `tests/fixtures/overrides/` as its `local_overrides`. 23 routes.
- **What was compared.** The validated configuration of `esphome config`, as data: every component, every lambda
  (lines that are only a C++ comment left out) and every resolved substitution. And the C++ that
  `esphome compile --only-generate` writes: `main.cpp` as the set of its statements, and every other generated file by
  its hash.
- **Result, ESPHome 2026.9.0 (what the add-on ships):** 0 of 23 routes differ. Every size the looks work out came out
  at the number the board file used to state, and `main.cpp` holds the same statements on every route.
- **Result, ESPHome 2026.6.2 (the packages' `min_version`), configuration:** 0 of 23 routes differ. The 10.1-inch
  Guition refuses 2026.6.2 in both versions, as before: its board file asks for 2026.8.0.

What differs, and why that is not a change of firmware:

- The order in which ESPHome registers components, scripts and API actions follows the order of the files, so the
  numbers ESPHome gives what it names itself (`automation_id_22`, its tables of component sources and icons, the
  index of an API action's strings) are handed out in another order. The comparison reads them as the same.
- Five substitutions nothing read are gone (`STANDBY_DIM_MS`, `ALERT_FLASH_FADE_MS`, `ALERT_FLASH_DARK_MS`,
  `ALERT_FLASH_LIT_MS`, `PAGE_DOTS_LARGE`), with the always-empty hook `BOOT_ENABLED_HOOK`; the paint hook
  `BOARD_PAINT_FILL` is two now, one per feature that brings a paint (`CAMERA_PAINT_FILL`, `CALIBRATION_PAINT_FILL`).
- The 10.1-inch Guition and the Waveshare 7 state their density with its decimals (`DISPLAY_DPI`), which is what their
  sizes were computed from; the firmware gets the same whole number as before (149 and 133).

## Gates

- `tools/check.sh --all` with ESPHome 2026.9.0: 23 of 23. 601 Python tests, 24 C++ tests, the package check (now over
  the whole chain of every board), the cells, board-shape and icon checks, the translations, the editor's tests, types,
  build and bundle, the nine overrides from GitHub issues, and every board compiled:

  | Board | Firmware image | Share of its update slot |
  |---|---|---|
  | CYD | 1,627,776 B | 88.7 % |
  | Guition 4-inch | 2,044,464 B | 25.2 % |
  | Waveshare 4.3 | 2,301,344 B | 28.3 % |
  | Guition 10.1-inch | 2,014,704 B | 24.8 % |
  | Waveshare 7 | 1,830,288 B | 46.5 % |
  | Waveshare 4B | 2,029,184 B | 25.0 % |

- The overrides people shared in GitHub issues (GitHub #5, #6, #15, #22), nine cases on all six boards, each read by
  ESPHome on its board in `tools/check.sh --firmware` ("Overrides from GitHub issues"), and their ids and substitutions
  kept by `tests/test_override_contract.py`.

## On the bench

The two bench screens (a CYD and a 4-inch Guition) build from their own YAML in a checkout, with their own name, keys
and, on the CYD, the calibration measured on its glass. For both, `esphome config` and the generated C++ of 0.2.126 and
of this release were compared the same way: the same configuration and the same statements.

Both were flashed over the air with `esphome run` from the release's worktree, one after the other:

- CYD: the firmware it reports went from the build of 03:00 (the nightly update from main) to 07:40, the build of this
  release. The UI self test (`diagnostics/run_ui_test.py`): 10 page checks and 50 overlay cycles, PASS, about 102 KB of
  heap free throughout. One "lvgl took a long time (183 ms)" warning during the stress test.
- Guition: from the build of 2026-09-22 17:32 to 07:43. The UI self test with its geometry check: PASS, 28 tiles.
  `diagnostics/capture_ui.py` saved the page the screen draws: the top bar with the home key, the room and the
  temperature, and the tiles in their cells.
- Home Assistant had both back within a minute: 76 entities, none unavailable, the same addresses, and the CYD reporting
  its shape as `320x240 2x3 143dpi compact`, as before.
- On the glass, checked by hand on both screens: the tiles where they were, a lamp tile switched on and off where it was
  tapped (the CYD's calibration), a page swipe forward and a swipe up back to page 1 (from the edge on the Guition), and
  the settings page opened by holding the top bar. All as before.
