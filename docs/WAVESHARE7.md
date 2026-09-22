# Waveshare ESP32-S3-Touch-LCD-7, experimental

Added in ESP Screen Manager 0.2.110, firmware 0.2.94, for [issue #22](https://github.com/MaxGramser/homeassistant_espscreen/issues/22).
This is the **800 x 480 ESP32-S3-Touch-LCD-7**, with GT911 capacitive touch, 8 MB octal PSRAM and 8 or 16 MB flash.
It is not the 7B, 7C or the version without touch. Physical acceptance has not been performed.

## Install

Update ESP Screen Manager and choose **Waveshare 7 inch (experimental)** in **New screen**.
Follow [Easy setup](EASY_SETUP.md) to create a profile with its own name, Wi-Fi references and unique API/OTA keys.
Choose the correct USB port, or download the firmware to flash with ESPHome Web from your own computer.
Keep an existing working profile if the board is already installed. Pair it through the ESPHome integration before adding tiles.

The remote package is `packages/waveshare7.yaml`; the checkout entry is `waveshare-esp32s3-7.yaml`.
Both combine `packages/core.yaml` with `packages/boards/waveshare-esp32s3-7.yaml`.
The 8 MB flash layout also fits a 16 MB board and leaves its additional flash unused.
Use the native USB connector for USB Serial/JTAG logs; the separate USB-to-UART connector is a different logging route.

## Layout and capabilities

- Landscape: 800 x 480, four columns of four cells. Portrait: 480 x 800, two columns of seven cells.
- Standard look at 133 dpi, following the existing physical size rules. The panel's non-square pixels are not compensated.
- Camera tiles, full-screen snapshots, live tile pictures, camera alerts and media artwork use the shared PSRAM implementation.
  Their operation on this board still needs physical verification. They are periodically refreshed images, not video playback.
- The unmodified backlight only switches on or off. The experimental profile keeps it on: no dimming, standby, night mode or alert flashes.
  The 4.3-inch board had brownouts on wake; that does not establish the same fault on this model. Standby remains disabled until tested.
- GT911 reports pixel coordinates, with no resistive calibration. The existing touch filter and action guard remain in use.
- LVGL uses a 12% draw buffer to leave internal RAM for networking and tile widgets.

## Hardware references

- [Waveshare specifications and pinout](https://docs.waveshare.com/ESP32-S3-Touch-LCD-7).
- [ESPHome MIPI RGB models](https://esphome.io/components/display/mipi_rgb/), model `ESP32-S3-TOUCH-LCD-7-800X480`.
- I2C: SDA GPIO8, SCL GPIO9. GT911 reset: CH422G EXIO1. LCD reset: EXIO3.
- EXIO2 supplies the backlight enable; EXIO6 enables the panel supply. The display owns EXIO6 and the backlight output owns EXIO2.
  The 7-inch model's own timings are used without the 4.3-inch overrides.

## What to report while testing

1. Board revision and flash size, successful boot, pairing and appearance in ESP Screens.
2. Correct colours and a stable picture across several page changes and cold starts.
3. Physical taps near each corner, slider drags and edge swipes, in the selected orientation.
4. A full page of tiles, opening and closing the settings and detail cards, and the half-turn setting.
5. Camera tile pictures, full-screen camera images and camera alerts, including repeated opens and closes.
6. Free internal heap with a full layout, any reset or I2C errors, and how long the screen stayed running.

Repeat the relevant checks after building the other orientation. Do not publish Wi-Fi passwords or API/OTA keys with logs.
The experimental release does not exercise standby or send commands to real Home Assistant devices as part of automated validation.
