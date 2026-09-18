# Test results app 0.2.76 / firmware 0.2.63 (2026-09-18)

Max sent a screenshot of a CYD page in the editor ("het gaat over CYD in de EDITOR die niet 100% goed eruit
ziet"): on every single tile the second line (*7,2 W*, *26,0 °C*, *Cleaning*, *Off*) was cut in half at the bottom
edge. See CHANGELOG 0.2.76. Built on 0.2.75 (`261fa82`).

## What happened before

- Both boards draw a normal tile the same way (`runtime_tiles.h`, `TILE_TITLE_X`/`TILE_VALUE_X` in both profiles):
  the icon in its circle on the left, the name and the value beside it, vertically centred. A small slider runs
  under that row; a watch card has the icon and the name on top and the big value below.
- The Vue editor (`TileCard.vue`, since 0.2.73) stacked the icon, the name and the value in one column. On a CYD
  mockup (292 × 219 px, the screen at 0.91) a tile is 47 px high; the stack needed 58 px, a watch card 63 px, and
  `.tile { overflow: hidden }` cut off the rest. The Guition's 73 px tiles had room, so it only showed on the CYD.
- The wide tiles already had the screen's row layout and were fine.

## What changed

- `TileCard.vue`: a single tile draws `.head` (icon + `.tx` with name and value) as a row, like the screen. A watch
  card has `.head.top` with the big value under it; the small slider sits under the row.
- `app.css`: `.tile .head` is the row, `.tile .tx` (was only for wide tiles) is the name/value column. On a CYD a
  tile with a small slider uses the screen's own numbers: 4 px padding above and below, an 8 px slider.
- `web/tests/components.spec.ts`: a test for the three shapes (plain, watch, small slider).

## Checks

- Demo home (`.esphome/readme-render/demo_home.py`) with the editor on Vite, kitchen screen (CYD): every tile
  measured with `scrollHeight` against `clientHeight` in the browser.

  | Tile | 0.2.75 needs / has | 0.2.76 needs / has |
  | --- | --- | --- |
  | Pasta · Active | 58 / 47 px | 47 / 47 px |
  | Coffee machine · On | 58 / 47 px | 47 / 47 px |
  | Kitchen lamp · On | 58 / 47 px | 47 / 47 px |
  | Power · 1249 W (watch) | 63 / 47 px | 47 / 47 px |
  | Sam · Home | 58 / 47 px | 47 / 47 px |
  | Kitchen lamp with a small slider (added in the DOM) | not measured | 47 / 47 px |

- Living room screen (Guition): all 8 tiles fit; the single tiles now show the icon left as on the screen's render
  (`docs/images/guition-home.png`).
- `npm test` 47/47 (one new), `npm run check` clean, `npm run build`; Python `unittest discover -s tests` 409 OK.
- No firmware change, so no ESPHome build and no C++ tests; `FIRMWARE_VERSION` stays 0.2.63.
- Still for Max: open a CYD in the editor after updating the app and look at the tiles.
