# Test results app 0.2.66 / firmware 0.2.57 (2026-09-17)

Camera images on the Guition, asked for by Max: an alert that shows who is at the door, and a camera tile that opens a
refreshed image full screen. Plus the Guition building again with ESPHome 2026.8+. See CHANGELOG 0.2.66, docs/CAMERA.md
and the compatibility note in docs/RELEASING.md. Built on 0.2.65 (`9567ab5`).

## Design decisions, with the measurement behind each

- **The add-on serves the image, the screen holds no token.** Home Assistant's camera access token changes every five
  minutes and a long-lived token on every screen opens all of Home Assistant. The app fetches with its own access and
  serves a random, short-lived link on port 8098 (Max chose a LAN port over a file in `/config/www`).
- **BMP, not JPEG.** On the bench Guition (Wi-Fi -70 dBm) ESPHome 2026.6.2 decoded a 480x270 JPEG in one loop call:
  `online_image took a long time for an operation (623 ms)`, long enough to lose a tap. A 24-bit BMP is decoded per
  4 KB as it downloads; the longest single loop operation over a six-minute soak was 353 ms, typically under 170 ms.
  The full-screen BMP is 388,854 bytes and downloads in 2.7-3.3 s over that Wi-Fi (the same from the Mac and from the
  Home Assistant Yellow, so the screen's Wi-Fi is the limit).
- **A steady rhythm.** Max saw the picture change after 3, 5, 7 s. The EZVIZ camera answered every snapshot in a
  steady 2.4 s (10 fetches: 2.36-2.56 s), so the cause was the app fetching on its own 2.6 s clock next to the screen's
  3.75 s. Now each load starts the next fetch and the screen loads every 4 s start to start: pulling every 4 s against
  the real camera gave camera timestamps 5, 4, 4, 4, 4, 3, 4, 5, 3, 5 s apart (the camera stamps whole seconds).
- **The alert keeps the picture of its moment** (Max's choice); a tap opens the camera now. An alert fetches its own
  snapshot, never one kept from a view in the last 30 s (found in review).
- **Alert design** (Max): the picture across the top of the card, 392x220, title and subtitle under it.
- **CYD: no images.** 320x180 RGB565 needs 115 KB in one piece; the CYD's largest free block is 41-53 KB.

## Automated

- Python (`.venv-portal`, with Pillow 12.2.0 as in the ESPHome image): 337 tests OK (334 before the review fixes).
  New `tests/test_camera.py` (23): the rules (domains, firmware, board, header, alert field), BMP encoding of JPEG, PNG
  with alpha, GIF and portrait snapshots (24 bits, no compression, exact size), links (304, expiry, bounds, stills),
  fetching per load, a failing camera's pause, an alert's own snapshot, the HTTP port, the broadcast order (announce,
  alert, link) with and without an image, requests a screen may and may not make, camera tiles only on a Guition, the
  words firmware and app share, and the profile's `set_url` guard. `test_wake_sleep.py` counts `preview_camera` among
  the writers of `last_use_ms`; `tile_icons` knows the cctv default.
- C++ (`clang++ -std=c++17 -Wall -Wextra -Werror -I.`): 17/17 PASS, new `tests/test_camera_view.cpp` (asking, loading
  rhythm, the gap after a slow load, failures, an empty link).
- `generate_packages.py --check` green. No token or Home Assistant URL in tracked files.
- Code review (high) found three issues, all fixed with tests: the alert picture could be a snapshot kept from a view
  up to 30 s earlier; a busy camera port stopped the whole app; `set_url()` on every load threw away the ETag, so an
  unchanged picture was downloaded again instead of answering 304.

## Builds

Easy profiles with the generated packages and local components (ESPHome 2026.6.2, the app's own):

| Profile | RAM | Flash | vs 0.2.65 |
|---|---|---|---|
| CYD | 23.8% (77,940 B) | 77.6% (1,423,815 B) | +368 B RAM, +2.6 KB flash |
| Guition | 25.5% (83,516 B) | 20.5% (1,664,915 B) | +2.6 KB RAM, +140 KB flash (http_request, online_image, BMP decoder) |

(0.2.65's numbers came from check profiles with other device names, a few bytes apart.)

ESPHome 2026.8.1 (ESPHome Device Builder on Max's Home Assistant), separate build folders: both packages compile.
Before this release the Guition package failed there: `backlight_fade.h` derived from `esphome::ledc::LEDCOutput`,
which ESPHome 2026.8 made `final`. With Max's agreement the hardware fader is gone and the backlight uses ESPHome's own
transition. Warnings left for later: top-level `online_image:` is removed in ESPHome 2027.1.0 (use
`image: - platform: online_image` once min_version is 2026.7+), and `platformio_options: build_flags` in 2026.12.0.

## Hardware (bench, Max's home Home Assistant, HA 2026.9.2 on a Home Assistant Yellow)

The app ran as a local add-on built from this tree on the Yellow (store add-on stopped meanwhile); both screens OTA.

| Check | Result |
|---|---|
| Guition boots 0.2.57 (compiled 15:22:34), inbox Synced, `sensor.guition_wallbox_screen_firmware` 0.2.57 | yes |
| `diagnostics/run_ui_test.py` on the Guition | PASS: 10 page checks and 50 overlay render cycles |
| Sleep and Wake from Home Assistant twice, ESPHome transition | no long operations logged; Max watched: "dimmen gaat mooi" |
| Alert with `camera: camera.max` through the real event | picture after 1.8-1.9 s (258,774 B, 391x220) |
| An `image.*` entity as the alert camera | loads (392x98: that entity's own 400x100 picture) |
| Alert without camera, Guition and CYD | the old card, unchanged |
| Tap on the alert picture (Max) | camera full screen; Back returns to the alert |
| Camera tile tapped (Max), refresh | seven images, 2.73-2.79 s downloads each, Back closes it |
| Camera open, then an alert with a camera | camera closes (PSRAM back to 6.09 MB), alert picture loads |
| Camera opened over an alert | both images held: PSRAM 5.66 MB free; close keeps the alert |
| Six-minute soak, 91 images | internal heap level at 77.2 KB free, PSRAM unchanged, no errors |
| Five real alerts in a row (Max testing) | all shown, pictures 0.9-1.9 s, no failed download |
| First image after the add-on started | one `ESP_ERR_HTTP_CONNECT`; the alert picture now retries three times |
| Home Assistant Yellow while a camera refreshes | add-on CPU 0-1.5 %, 51 MB |
| CYD boots 0.2.57 (compiled 15:23:49), inbox Synced | yes |
| Alert with a camera on the CYD | shown without the picture, OK closes it |
| Camera tile saved for the CYD | refused: "Camera images need a Guition screen." |

Internal heap from the screens' own sensors, five minutes after the release build booted (the Guition's window held the
UI self test, Sleep/Wake twice, a camera view and an alert with a picture; the CYD's a sync and an alert):

| Screen | Free | Largest block | Lowest since boot | Before (earlier today) |
|---|---|---|---|---|
| Guition 0.2.57 | 80,108 B | 34,816 B | 56,548 B | 0.2.55 after hours of use: 83,172 / 40,960 / 72,472 B |
| CYD 0.2.57 | 114,024 B | 57,344 B | 104,884 B | 0.2.54 after hours of use: 104,424 / 49,152 / 85,600 B |

PSRAM on the Guition stays near 6.1 MB free with nothing open, 5.9 MB with a camera and 5.7 MB with a camera over an
alert. The Guition's lowest point is lower than this morning's after a UI self test plus camera and alert in the same
five minutes; the soak kept the heap level, so it is headroom used at once, not a leak.

Not tested: the Docker route (no Linux Docker host here); a doorbell integration (Max has none, `camera.max` stood in).
