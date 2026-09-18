# Test results app 0.2.75 / firmware 0.2.63 (2026-09-18)

Max: "dit icon ziet er overal gek uit op de CYD", with a photo of the mdi:power glyph on a grey key: a lumpy,
uneven ring. This release draws the icon fonts with four bits per pixel on both boards. See CHANGELOG 0.2.75.
Built on 0.2.74 (full-page tiles, `bb25353`), which itself follows 0.2.73 (the Vue editor, `1ed42dc`).

## What happened before

- Every Material Design Icons `font:` block of both boards (28/18/12 px on the CYD, 42/26/18 px on the Guition, plus
  the 40/64 px font of a full-page tile since 0.2.74) had no `bpp`. ESPHome's default is 1 bit per pixel: it loads
  each glyph with `FT_LOAD_TARGET_MONO`, so every pixel is either fully on or off. The Roboto text fonts had `bpp: 4`
  since the first release; nothing in the history or the docs says the icons were left at 1 on purpose.
- A thin stroke suffers most. The ring of mdi:power at 18 px (the climate power key, the settings page) comes out
  between one and two pixels wide around its circumference; at 28 px (a tile) the ring still wobbles. Filled icons
  such as the lightbulb or the cog only get a ragged outline. Rendering the real glyphs through freetype the way
  ESPHome does, side by side at 1 and 4 bpp, showed exactly the ring of Max's photo.

## What changed

- `bpp: 4` on every MDI font block of `home-like-2432s028.yaml` and `guition-4848s040.yaml`; packages regenerated.
- `tools/generate_icons.py` (FONT_BLOCK) and `tests/test_tile_icons.py` expect the `bpp: 4` line between `size`
  and `glyphs`, so the icon generator keeps it on every rewrite.
- No change to the tile protocol, the layouts or the app; the app raises `FIRMWARE_VERSION` so the manager offers
  the update.

## Flash on the CYD

Measured by compiling the same profile twice at the same commit (0.2.72, `9f5694b`), only the `bpp` lines differ:

| Build | firmware.bin | of the 1.835 MB app slot |
|---|---|---|
| 1 bpp (before) | 1 538 032 bytes | 83.8 % |
| 4 bpp (this release) | 1 632 736 bytes | 89.0 % |
| difference | +94 704 bytes | +5.2 points |

On 0.2.74 (`bb25353`, which added the 40 px full-page font and the 48-tile code) the release build is 1 655 568 bytes,
90.2 % of the slot, static RAM 21.7 % (71 188 of 327 680 bytes); about 180 KB stay free for later rounds.

The glyph bitmaps alone grow by 108 KB (309 icons at three sizes plus the big font, counted with freetype); the
linker's alignment takes a little of that back. Fonts live in flash, so RAM does not move (24.1 % static in both
builds). 2 bpp would have cost 38 KB but still shows four grey steps on a thin ring; the Guition has 16 MB flash and
gets the same 4 bpp so both brands look alike.

## Automated

- Python (`.venv-portal`, `unittest discover -s tests`): 409 tests OK on 0.2.74 plus this change (386 before the
  rebase, at 0.2.72; the icon-font test now insists on `bpp: 4` in all four blocks).
- C++ (`clang++ -std=c++17 -Wall -Wextra -Werror -I.`): 17/17 PASS, before and after the rebase.
- `tools/generate_packages.py --check` and `tools/generate_icons.py --check` green.
- `esphome compile home-like-2432s028.yaml` and `esphome compile guition-4848s040.yaml` from a worktree
  (`icon-bpp`), sequentially.

## Hardware

- Pending: the bench CYD flashed with this firmware, then Max looks at the power key, a light tile and the
  settings page.
