# Test results app 0.2.41 / firmware 0.2.35 (2026-09-15)

Screenshots in the README and on the App store page, and firmware 0.2.35 so the owner can
test an update from the app. See CHANGELOG 0.2.41 and RELEASING "Compatibility 0.2.41".

## What changed

- `README.md`: the owner's two photos of a Guition at home and the ESP Screens page at the
  top; both boards under **Supported screens**; a card gallery under **What you can
  configure**; the alert next to the Alerts cheatsheet; New screen and choosing tiles under
  **Step by step**; the Heating tile's settings next to that tile on the screen; the top bar
  sheet next to the top bar. 21 images in `docs/images/` (2.4 MB). The photos carry no
  location or camera metadata (EXIF holds only the image size).
- `screen_manager/README.md` (the App store page): a photo and the editor through absolute
  `raw.githubusercontent.com` URLs, since the App store does not resolve relative paths.
- Firmware 0.2.35: only `SCREEN_FIRMWARE_VERSION` in both profiles, packages regenerated,
  `core.FIRMWARE_VERSION`.

## How the images were made

- **Screens:** each board's generated package (`packages/guition.yaml`, `packages/cyd.yaml`)
  became an ESPHome host project with SDL display and touch at the panel's native size.
  The CYD stays 240 × 320 and LVGL's 90° rotation makes it landscape, like the real screen.
  The project also got template outputs for the backlight and LED, host time
  (Europe/Amsterdam) and a stand-in for the LEDC backlight fader. What the screen draws is the
  package's own LVGL tree, fonts and `runtime_tiles`. The firmware's own `screen_message` and
  `preview_runtime_card` actions fed it the messages and opened the detail cards;
  host-only actions switched pages and saved `lv_snapshot_take` of the screen with the top
  layer composited, so the alert card is included. Time fixed at Tuesday 15 September 2026, 10:08.
- **Messages:** the app's own code (`server.Manager`, `core`, `header_bar`) on a demo home
  with made-up English entities, no data from a real home: a Guition with 17 tiles on 4 pages
  and a CYD with 9 tiles on 2 pages.
- **ESP Screens page:** that demo app in headless Chrome (DevTools protocol, device scale 2,
  the same frozen time). For the capture session only, the page's CSP was bypassed to swap
  the dialog backdrop blur for a flat color.
- Post-processing: rounded corners on the screen renders, crops of the editor captures.

## Automated

- Python (`.venv-portal`): 139 tests OK.
- C++ (clang++ -std=c++17 -Wall -Wextra -Werror -I.): 13/13 PASS.
- `generate_packages.py --check` green; `node --check app.js` clean.
- ESPHome 2026.6.2 compile SUCCESS, one after another: `easy-guition-device.yaml` (RAM 32.5%,
  flash 18.6%), `easy-cyd-device.yaml` (RAM 31.5%, flash 77.3%), manual `guition-device.yaml`
  (RAM 32.6%, flash 19.7%) and `device.yaml` (RAM 31.5%, flash 82.1%); `main.cpp` of both
  builds carries 0.2.35. Not uploaded: both bench boards stay on 0.2.34 so the owner can
  test the update.
- Profiles written by `core.installation_yaml` for both boards (remote package from `main`,
  `refresh: 0s`) pass `esphome config` in an empty folder: package, external components and
  fonts all download from GitHub.

## README check

- Rendered through GitHub's markdown API (`POST /markdown`, gfm, repository context) inside
  GitHub's markdown CSS and captured at 1280 px and 400 px: all 23 image references load.
  Every row stays on one line, with matched heights where a sheet sits next to a render.
  Nothing overflows at phone width.

## Not tested / for the owner

- The update from 0.2.34 to 0.2.35 through the app (**Update** or the nightly round).
- How Home Assistant's App store draws the images on the app page (not opened in the
  Home Assistant frontend).
