## Easy Setup and releases

The preferred route for new users is docs/EASY_SETUP.md: ESP Screen Manager
plus remote ESPHome packages. No token or blueprint needed. Tiles live in the
persistent add-on data; Wi-Fi/API/OTA stay in the device's own ESPHome YAML. Read
docs/RELEASING.md before publishing updates. Main distributes both boards.
Every push to GitHub is a release: always also bump the add-on version in
screen_manager/config.yaml (with a CHANGELOG line), otherwise HA won't see an update.
Generate packages with tools/generate_packages.py; don't edit them by hand.
Runtime mode supports all cards in all twenty positions. The position constraints
below apply only to the old manual profiles.
Preserve the data schema, protocol compatibility, unique keys, and CYD preferences.
Test updates against existing data. Don't publish an unknown storage version without
a migration. Production Ingress needs no long-lived token or public port.

## Guition board

The Guition 4848S040 has a separate profile `guition-4848s040.yaml` with
480×480, ST7701S RGB, and GT911. Read docs/GUITION.md. Use its own local
`guition-device.yaml`; don't carry over the CYD layout or XPT2046 calibration.
Verify touch with tools/verify_gt911.py and keep the CYD regressions green.
Don't configure wallbox relays as part of display support.

# Working instructions for LLMs and developers

This project drives an ESP32-2432S028 with ILI9341 + XPT2046 (320×240,
LVGL 90°). Read README.md and docs/ before installing. The owner can
physically tap; an agent cannot replace that with software coordinates.

## Installing a new screen

1. Identify the USB port and board variant. Use ESPHome 2026.6.2 with Python
   3.11–3.14. First check whether local configuration already exists.
2. In a fresh copy, use `tools/new_device.py` to create your own `device.yaml`,
   `calibration.yaml`, `secrets.yaml`. Never overwrite a working profile.
   Let the owner fill in Wi-Fi locally. Don't show keys in logs/chat.
3. Flash `device.yaml` with the CLI substitution `CALIBRATION_ON_BOOT=true`.
   The isolated screen shows five crosshairs and works without HA.
4. Follow docs/CALIBRATING.md: guided USB capture, fit, flash, new capture,
   independent verification. Ask for physical taps per target. Wait for confirmation
   that the measurement screen is actually visible; a successful build is not a flash.
5. Flash without that override. Pair with the owner's own HA via the ESPHome integration.
   Read real entity IDs and supported attributes; don't make up entities.
6. Configure every tile you use according to docs/TILES.md. The vacuum card
   sits at position 6. Positions 8/10 don't have a full slider/climate binding.
   Don't test real device actions without the owner's permission.
7. Go through docs/ACCEPTANCE.md and report the tests actually carried out,
   limitations, and the observed stability duration.

## Code and regressions

- Keep base hardware and UI in `home-like-2432s028.yaml`; personal data belongs
  in the gitignored local profiles.
- Preserve fixed pages, hidden navigation at six tiles or fewer, and a
  minimum default standby of 600 seconds. Don't reintroduce free scrolling
  without physically testing for touch/navigation regressions.
- Preserve the touch filter and event guard before actions; calibration processes
  filtered physical ADC values. Don't apply the affine correction twice, in both the driver and the UI.
- The calibration wizard assumes swap_xy=false, mirror_x=true, mirror_y=false,
  and LVGL 90°. A changed orientation also requires a new projection/tests.
- Run the Python tests, both C++ test suites, and ESPHome validation/build on code changes.
  Firmware tests and hardware acceptance are different checks.
- `diagnostics/run_ui_test.py` renders without HA actions; don't touch the screen
  during that test. Use `--name` for the expected device identity.
  `diagnostics/send_layout.py` pushes a demo layout with every card type to a
  screen via the API inbox (no HA actions); the manager restores the real
  layout within ~25 s. Guition: `capture_ui.py` saves the LVGL render as a PNG.
  Without a screen: `tools/render_topbar.py` renders the real top-bar code for both
  boards via the ESPHome host + SDL2 to `.esphome/render-topbar/out/sheet.png`.
- Share via `tools/export_bundle.py` or Git. Don't stage secrets, measurements,
  binaries, logs, build caches, or local device profiles.
- No automatic firmware upload to an arbitrary connected port.
  With multiple boards, first determine the intended port.
- Profiles with the same `DEVICE_NAME` (Easy Setup and manual) share
  `.esphome/build/<name>`. Never compile or upload them concurrently; check
  the `firmware.bin` path in the upload log, and then the compile time via
  `device_info`. A wrong profile knocks the screen out of ESP Screen Manager.

Historical diagnostics are background context, not proof that a new panel
works correctly. During onboarding, don't make claims about physical tests that weren't actually performed.
