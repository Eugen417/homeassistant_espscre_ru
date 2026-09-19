# Troubleshooting

| Symptom | Check first |
|---|---|
| No USB port | Data cable, a different USB port, board power; compare `python -m serial.tools.list_ports` before/after connecting. Install the USB chip vendor's driver if needed. |
| Permission denied on Linux | Access to the serial device group (often `dialout`); log in again after a group change. |
| Port busy | Stop ESPHome logs, the IDE monitor, and other serial readers. One reader at a time. |
| Upload keeps connecting | Check the correct board/port. Hold BOOT while connecting and release afterward if the board requires it. |
| ESPHome Web shows no port or can't connect (Download) | Chrome or Edge on a computer; Safari, Firefox and phones can't reach USB. A data cable, and the driver for the board's USB chip if the computer asks for one (CH340 on most CYDs). Hold BOOT while choosing the port if it keeps failing to connect. |
| Build fails | ESPHome 2026.6.2 or newer with a Python it supports (the app's 2026.9.0 needs 3.12–3.14); the full unpacked folder, local fonts/components present, internet, and disk space. Save the first real error from the build log. |
| White/black or wrongly rotated image | Check the ILI9341 + XPT2046 variant. Don't blindly copy the pins, rotation, or SPI settings from a different CYD model. |
| No measurement screen | The upload must be complete, not just the build. Flash with `-s CALIBRATION_ON_BOOT true`. Look for `USB calibration ready` in the USB logs; a normal boot shows no crosshairs. |
| No measurements | Five crosshairs visible, other log reader closed, correct port. Press Enter in the wizard first, then tap the requested point three times, releasing in between. |
| Tap jumps to a different spot | Check the independent `verify`, your own `calibration.yaml`, and a fixed orientation. Don't use the same taps to both fit and verify. |
| Fit reports spread/error | Measure again following the prompts. Don't fix unstable ADC readings by raising the tolerance; check power, cable, and touch hardware. |
| Duplicate HA action | Check both the touch logs and your HA automations: an automation that reacts to the same entity can repeat or undo what the tile did. |
| Wi-Fi doesn't connect | 2.4GHz, correct local secrets, network range. USB calibration doesn't need HA. |
| HA doesn't see the board | Manually use the IP from the logs; port 6053 reachable, correct encryption key, no guest network isolation. |
| Status works but action doesn't | Check the HA option for allowed device actions, the real entity ID, and a supported action. |
| Vacuum/climate partly usable | Supported modes/attributes differ per integration; test the same action in Home Assistant first. |
| Wrong pagination | Tiles keep the slot you gave them in ESP Screens: six per page, up to eight pages, and a Save & send applies it without a reflash. The page buttons hide on a screen with one page, and **Page buttons** off hides them everywhere; then only swiping and Go to page tiles change the page, and the editor warns about a page nothing leads to. Firmware older than 0.2.26 packs the tiles in order instead. |
| Freeze/reset | Keep the USB log, check the reset reason/power, and run the render test. Note the action and time. |

For a USB log (stop other readers):

```sh
python diagnostics/capture_serial.py --port <USB_PORT> --seconds 120
```

The `docs/TEST_RESULTS_*.md` files describe tests on the owner's own boards. Their
measurements are not calibration for a new board. Start at [README.md](../README.md), keep a working
local configuration, and change one cause at a time.
