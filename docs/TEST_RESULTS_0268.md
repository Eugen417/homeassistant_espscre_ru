# Test results app 0.2.68 / firmware 0.2.58 (2026-09-17)

GitHub issue #8 asked for a firmware download, because Home Assistant runs on a Proxmox server and plugging the
screen into it is awkward. ESP Screens already builds a profile's firmware with its own ESPHome CLI and flashes it over
USB; this release adds a third choice: build, download the file, and put it on the screen from your own computer with
ESPHome Web. Max's calls: the three routes are "build and flash on the Home Assistant machine", "build and flash in the
browser" (later) and "build and download" (this release); USB on the Home Assistant machine must always stay visible,
also before a board is plugged in, "or people think it isn't possible at all". App only. Built on 0.2.67 (`6809d3b`).

## ESPHome, read first

- ESPHome 2026.6.2 `components/esp32/__init__.py` `get_download_types`: the "Factory format (Previously Modern)" is
  `firmware.factory.bin` next to `firmware.bin`, "For use with ESPHome Web and other tools"; the dashboard's download
  handler serves it from `firmware_bin_path.parent`. `core/config.py`: `ESPHOME_BUILD_PATH` becomes
  `<build path>/<node>`, so the app's `data/build/<profile>` holds `<node>/.pioenvs/<node>/firmware.factory.bin`
  (PlatformIO) or `<node>/build/firmware.factory.bin` (`espidf/toolchain.py`, native ESP-IDF). Both are searched; the
  newest wins and links are skipped.
- ESPHome's own install dialog (`esphome_dashboard` source maps, `install-choose-dialog.ts`) offers Wirelessly, Plug
  into this computer, Plug into the computer running ESPHome Device Builder and Manual download. Browser flashing needs
  `"serial" in navigator` and `window.isSecureContext` (HTTPS); otherwise it falls back to the download plus
  `https://web.esphome.io/?dashboard_install`. ESP Screens links to that same address.
- ESPHome Web's words (`esphome/device-builder-frontend` `src/translations/en.json`): "Connect your device below, then
  click Install to flash your downloaded project", Connect, Install, "Select the factory binary you want to install on
  your device", and holding BOOT when it fails to initialise. The steps in the window and the docs use those words.

## Automated

- Python (`.venv-portal`): 386 tests OK (377 on 0.2.67).
  - New `tests/test_firmware_download.py` (9), with a stand-in ESPHome CLI that writes the factory image where ESPHome
    2026.6.2 does: the Download target is checked (CLI, busy slot, unknown targets) before anything is written; the
    download job compiles without uploading and offers `<profile>.factory.bin`; a build without a factory image fails
    instead of offering nothing; the native ESP-IDF layout is found; a new job withdraws the old file until it succeeds,
    so a failed rebuild offers nothing; Build only offers the file and Check does not; only a file built by this process
    is served (not one left from before a restart, not another name, not a link); the newest image wins after a node
    rename; and the HTTP route: CSRF on the create, 400 while building, `application/octet-stream`,
    `attachment; filename="kitchen.factory.bin"`, `no-store`, identical bytes, and the *not yet in Home Assistant* card
    turning to "downloaded".
- `node --check screen_manager/app/static/app.js` clean.

## Real ESPHome builds (2026.6.2, Mac)

Through `Firmware.install(target='download')` itself, with the app's environment (`ESPHOME_BUILD_PATH` per profile,
`PLATFORMIO_CORE_DIR` under data), a profile made by `installation_yaml` and the remote packages from `main`:

- **CYD** (`download-check-cyd`): success after 132 s, `Succeeded: download`, the image found at
  `data/build/download-check-cyd/download-check-cyd/.pioenvs/download-check-cyd/firmware.factory.bin`, 1,602,272 bytes.
  Layout checked: padding to 0x1000, bootloader magic 0xE9 at 0x1000, partition table at 0x8000 (otadata, phy_init,
  app0 at 0x10000, app1 at 0x1D0000, nvs at 0x390000), app at 0x10000: `esptool image-info` checksum valid, validation
  hash valid, project `download-check-cyd`, ESP-IDF 5.5.4; the image contains firmware version 0.2.58. The file ends
  far below NVS, so flashing it keeps a CYD's touch calibration, as a USB install does.
- **Guition** (`download-check-guition`): success after 126 s, 1,841,760 bytes at the same place. Bootloader magic
  0xE9 at 0x0 (ESP32-S3), partition table at 0x8000 (app0 at 0x10000 and app1 at 0x7D0000, 0x7C0000 each; nvs at
  0xF90000), app at 0x10000 with a valid checksum and validation hash, project `download-check-guition`, firmware
  0.2.58 inside.

## Editor, clicked through

Demo home with the real app code (fake Home Assistant, stand-in CLI with ESPHome's log lines, USB ports from a file),
desktop 1280×900 and phone 375×812:

- New screen without a board: **Install via** lists `USB · no board found on the Home Assistant machine yet` first and
  selects it, **Install** waits (disabled) and the hint says where to plug in or to choose Download.
- A board plugged in: the list shows its port within a poll and selects it; **Install** is enabled. A second board:
  "More than one board connected". Unplugged while chosen: back to the waiting USB entry.
- **Download** or **Later** chosen: kept while boards come and go. Button: **Build & download** / **Save profile**.
- **Build & download**: `POST firmware/profiles` with `target: download`, "Building firmware…" with the spinner;
  closed and reopened during the build, the window shows the running build again and ends on the download. Done:
  "Ready to download.", **Download garage-screen.factory.bin** (`api/firmware/profiles/garage-screen.yaml/download`,
  fetched from the page: 200, `application/octet-stream`, attachment name, `no-store`, 1,504,097 bytes; not clicked,
  so nothing landed in Downloads), the ESPHome Web link to `?dashboard_install` with `target="_blank"`, the CYD
  calibration note, the API key and the pairing steps.
- A failing build: "That didn't work." / "Build failed" with the error line; **Retry** posts `action: download` without
  a target. **Later**: "Profile saved." with the new text that points to Firmware & USB and Download.
- The card under My screens: "Firmware downloaded, not yet in Home Assistant…".
- Firmware & USB: Wi-Fi / OTA, the USB entry (waiting or the ports, refreshed while open), Download; with Download the
  host field hides and the button says **Build & download**; a profile never built shows no link; after the build,
  **Download kitchen-screen.factory.bin** appears; back to Wi-Fi / OTA: **Build & install** with the host field.
- Phone width: the page does not scroll sideways; the download box wraps.
- Found on the way: the device name field's `pattern="[a-z][a-z0-9-]{0,29}"` is invalid under the `v` flag that current
  Chrome uses for `pattern`, so Chrome skipped the check and logged an error. Now `[a-z][a-z0-9\-]{0,29}`: "Bad Name",
  "9lives" and 31 characters are flagged while typing, "living-room" and "ok-2" pass.

## Home Assistant ingress (read only)

The Yellow runs the store add-on 0.2.66. Its existing zip download (`api/claude-skill.zip`, the same kind of GET with
`Content-Disposition`) fetched through `/api/hassio_ingress/…` with an ingress session: 200, `application/zip`,
`attachment; filename="esp-screens.zip"`, `no-store`, `Content-Length` kept and a valid zip. The firmware file takes
the same path (below Home Assistant's 4 MB limit for a buffered response).

## Not tested / for the owner

- Nothing was flashed. A real flash with ESPHome Web in Chrome, with a file downloaded from the add-on in Home
  Assistant, is still to do: New screen (or Firmware & USB → Download) → Build & download → Download → web.esphome.io
  → Connect → Install → select the file. A board keeps its identity when the file comes from its own profile.
- The first build on the Yellow takes minutes, as with USB.
