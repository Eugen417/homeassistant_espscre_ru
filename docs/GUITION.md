# Guition ESP32-S3 4-inch wallbox — 480 × 480

This branch adds a separate profile for the **Guition 4848S040** with an
ST7701S RGB display and **GT911 capacitive touch**, ESP32-S3, 16 MB flash, and
8 MB octal PSRAM. The name/2mm wall plate also describes the enclosure; always
check the electronics. This profile doesn't configure any relay.

Hardware source: the [ESPHome Guition board writeup](https://devices.esphome.io/devices/guition-esp32-s3-4848s040/)
and the built-in [ST7701S driver](https://esphome.io/components/display/st7701s/).
The panel configuration follows the original manufacturer demo, found physically clean;
see [the comparison](GUITION_FACTORY_REFERENCE.md). No local RGB driver
or periodic recovery script is needed. The GT911 uses unmirrored coordinates.

## Files

- `packages/boards/guition-4848s040.yaml`: the board file (hardware, the 480×480 sizes, the Guition's own parts); the
  interface itself is `packages/core.yaml`, shared with the CYD (docs/PROFILES.md).
- `packages/guition.yaml`: what a screen builds from over GitHub; `guition-4848s040.yaml` in the root is the same
  for a build from a checkout.
- `<your-screen>.yaml`: the screen's own profile, written by ESP Screens, outside Git.
- `secrets.yaml`: local Wi-Fi/API/OTA settings, outside Git.
- `tools/verify_gt911.py`: physical pixel check; no resistive ADC calibration.

The 2.8-inch CYD profile stays separate. Don't flash that to the Guition, and don't
carry over `calibration.yaml` or the XPT2046 correction.

## Interface

### Mounting and screen rotation (runtime 0.2.9+)

Install Guition firmware 0.2.9 and ESP Screen Manager 0.2.9. Open the screen
in the management page, open the **Screen settings** tab, and choose 0°, 90°,
180°, or 270° (clockwise) under **Rotation**. It applies at once; there is
nothing to save. After that, a firmware flash is no longer needed to change the
angle. The setting is kept in the app data and on the screen after a restart.

This uses native ESPHome/LVGL rotation for both display and touch, with no changes
to the panel initialization or GT911 mirroring. After mounting, physically check the
four corners and navigation; a render test doesn't test touches. The option is
only visible for the Guition once its new firmware has been discovered by HA.
The CYD stays on its existing orientation with its own calibration.

Six tiles of **218 × 108 pixels** per page, 12px spacing, and a
separate navigation strip. In ESP Screen Manager, up to 48 tiles fit across up to
eight pages (firmware 0.2.62+; twenty over four pages before); navigation disappears
at six or fewer. The current interface has a light gray background, white cards,
and colored domain icons. From 0.2.10, you can choose a pastel background with
dark text per tile. Standby starts after ten minutes without touch by default and
is adjustable in the management page.
The backlight dims through ESPHome's own light transition (1.5 s to standby, 80 ms to
wake). Firmware 0.2.13-0.2.56 used the LEDC hardware fader (`backlight_fade.h`) instead;
0.2.57 went back to ESPHome's transition, because that fader reached into ESPHome's LEDC
output, which ESPHome 2026.8 no longer allows.

The existing tile actions, climate control, and vacuum card are preserved.
Every card type works in all 48 positions.

## New installation

Install the screen from ESP Screens ([EASY_SETUP.md](EASY_SETUP.md)): it writes the
screen's own profile with its name, Wi-Fi reference and unique keys, and flashes it
over USB. Choose the tiles afterwards in the app, not in the YAML.

Check the serial port and chip before uploading. On the tested board,
higher serial speeds proved unreliable when reading. Use 115200 baud:

```sh
python -m esphome config <your-screen>.yaml
python -m esphome compile <your-screen>.yaml
```

Flash via `esphome run` if the USB connection is reliable. For explicit
115200 baud, you can use the combined factory image. Replace both the
port and the device name in the path with your own values:

```sh
python -m esptool --chip esp32s3 --port <USB_PORT> --baud 115200 write-flash 0 .esphome/build/wallbox-kitchen/.pioenvs/wallbox-kitchen/firmware.factory.bin
```

The factory image belongs to this S3 profile and starts at address **0**, unlike
some classic ESP32 images. Keep a recovery copy before replacing
existing firmware if you might need it later. Keep binaries and logs local.

## Testing GT911 and orientation

The GT911 reports pixels directly. Usually no calibration is needed; a wrong
rotation/transform should not be papered over with the CYD affine wizard.

With a working Wi-Fi/API connection:

```sh
python diagnostics/control_ui.py touch_diagnostics --host wallbox-kitchen.local --name wallbox-kitchen
python tools/verify_gt911.py --port <USB_PORT> --output measurements-guition.json
python diagnostics/control_ui.py end_touch_diagnostics --host wallbox-kitchen.local --name wallbox-kitchen
```

Five crosshairs appear. The wizard asks for three separate taps per
point; press Enter first, then touch only the named crosshair. Error
at most 16px, spread at most 18px. The test doesn't send any HA actions.

Without Wi-Fi, you can first build with `-s CALIBRATION_ON_BOOT true`,
which shows the GT911 measurement screen right after the USB upload. After
checking, rebuild/reflash without that override. This compatibility name only
opens a pixel test; it doesn't perform any ADC calibration.

On the tested panel, both GT911 axes are mirrored: `TOUCH_MIRROR_X` and
`TOUCH_MIRROR_Y` are set to `true`. The default is `LVGL_ROTATION: "0"`. For a different mounting, set the
desired LVGL rotation and give the wizard the same value, for example
`--rotation 90`. Check every corner; only change the GT911 `TOUCH_SWAP_XY` /
`TOUCH_MIRROR_X` / `TOUCH_MIRROR_Y` if the physical measurement requires it.

## HA and acceptance

Add the new device via the ESPHome integration (name/IP, port 6053,
API key from secrets), then choose its tiles in ESP Screens.

```sh
python diagnostics/run_ui_test.py --host wallbox-kitchen.local --name wallbox-kitchen
python -m unittest discover -s tests -p 'test_*.py'
```

Also go through the physical checks, using the GT911 wizard instead of the XPT2046
calibration. Check the display, correct touches, pagination, long press,
climate and vacuum cards, real HA feedback,
standby, and recovery after a restart. A compile/render test doesn't replace
that physical check. The original CYD board's test outcome
says nothing about this new board.

## Inspecting the rendered interface

```sh
python diagnostics/capture_ui.py --host wallbox-kitchen.local --name wallbox-kitchen --output diagnostics/guition-home.png
```

This only takes an LVGL snapshot on request and sends a 240×240 preview
over the encrypted API. The temporary buffer is released after about 24 seconds.
The image verifies the renderer; it doesn't prove the physical
RGB signals, screen colors, or mounting orientation are correct. The render test
also checks that visible click areas fall within the 480×480 screen.

## Recovery copy of this test board

The original firmware in use, including bootloader, partition table,
and NVS, is stored locally as `diagnostics/guition-original-recovery.bin`.
The lengths of the five application segments determined the needed 4,788,224
bytes; this is not a copy of the full 16 MB flash. The stub verified the MD5
of every block read. SHA-256 of the complete recovery file:

```text
a656c793c422848b5ad72bc16545fb92c844b7dfba029f4e4399d22444a9e046
```

This copy is private, belongs only to the tested board, and is not
committed or exported. Recovery happens at address 0, with the same
115200-baud command as the factory image, but with this recovery file.

## RGB memory settings

The pixel clock is set to 16 MHz: on the test board, this removed the faint
flicker seen at the 12MHz setting. For the RGB bounce buffer, code and
constant data run from octal PSRAM, with a 64KB data cache and 64-byte
cache lines; RGB stream recovery happens on VSYNC. These settings
follow [Espressif's RGB LCD recommendations](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/peripherals/lcd/rgb_lcd.html).
Physically check for occasional glitches under Wi-Fi/render load; a
software snapshot can't prove a disturbance in the panel signal.
