> Historical investigation on a single panel. For a new screen, see [README.md](README.md)
> and [the five-point calibration](docs/CALIBRATING.md); don't reuse these measurements.

# ESP32-2432S028: stable operation

Use **home-like-2432s028.yaml** for the connected 2.8-inch CYD with
ILI9341 and XPT2046. The Wi-Fi, API, and OTA secrets stay in `secrets.yaml`.

## Operation

- Set `TILE_COUNT` to the number of tiles in use (1–10). With six tiles or
  fewer, both page buttons and the page indicator disappear; higher
  tile numbers stay hidden.
- Page 1 holds tiles 1–6; page 2 holds 7–10. Use the **Previous**
  and **Next** buttons at the bottom. Swiping is disabled.
- A short tap runs the tile's existing action. A long press opens the
  existing light, climate, or vacuum card, or the configured long-press action.
- Standby starts after ten minutes without touch (`AUTO_DIM_TIMEOUT: "600"`).
- The first tap on the dimmed screen wakes the screen.
- Contacts shorter than `TOUCH_MIN_PRESS_MS` (60 ms on the CYD, 20 ms on the
  Guition) are ignored. One touch can produce at most one tile action;
  a movement greater than `TOUCH_MOVE_LIMIT_PX` (56 px, about 1 cm, on the CYD;
  0 on the Guition: no limit, LVGL decides and releasing inside the tile counts)
  relative to the settled first contact point cancels the tap; only the first
  contact counts; the same tile is protected against contact bounce for 600 ms.
  A rejected tap is logged with its reason (tag `touch`).
- The thermostat sends the rounded, clamped temperature it displays.
  A local temperature or mode choice gets three seconds to sync with HA.
  An active control press prevents automatic dimming.
- Pippa uses the real `vacuum.s8` status, so docking, cleaning,
  pausing, and returning are distinguished. Missing HA states
  are shown as `Unavailable`.

## Technical choices

ESPHome 2026.6.2 with ESP-IDF; no Arduino components are needed.
The display bus runs at 40 MHz, like ESPHome's standard CYD profile.
XPT2046 is polled every 20 ms without the GPIO36 interrupt. The pressure
threshold is 800, after a suspect corner reading at pressure 422. A local XPT2046
driver requires three consistent ADC samples for a new touch, ignores isolated
outliers, and waits for two pressure-free samples before release. This adds about
40–60 ms on touch and prevents an unstable first reading from being immediately
interpreted as a different button. The driver lives under `components/`. The stock
interrupt driver tries to enable an unavailable internal pull-up on that
input-only pin; the old firmware logged a GPIO error for this.

LVGL gets a 12% draw buffer. No scroll animations and no extra flash pulse after
each tap. Tile backgrounds are opaque. Updates within 100 ms are
coalesced; unchanged tile labels and colors are not redrawn.
UI initialization only runs after LVGL has started. The clock refreshes immediately
on time sync. Roboto is bundled locally in `fonts/`, including the OFL license.

The active layout is 320×240. The old, commented-out portrait presets
are not adapted page layouts yet: with a different orientation, also reserve
34 pixels at the bottom for navigation and check the tile positions.

## Building and flashing

```sh
esphome compile home-like-2432s028.yaml
esphome upload home-like-2432s028.yaml --device /dev/cu.usbserial-130
esphome logs home-like-2432s028.yaml --device /dev/cu.usbserial-130
```

Close any running serial log reader before flashing. The previous configuration
is stored locally in `diagnostics/before-2432s028.yaml.bak`; the corresponding previous
factory binary is in `diagnostics/before-2432s028.factory.bin`. These local backups
are not included in Git. You can roll back by restoring the backup as the configuration
and rebuilding/flashing; restore the old Arduino toolchain
if you build the old configuration.

## Tests

```sh
clang++ -std=c++17 -Wall -Wextra -Werror tests/test_cyd_ui.cpp -o /tmp/test_cyd_ui
/tmp/test_cyd_ui
python3 tests/test_layout.py
clang++ -std=c++17 -Wall -Wextra -Werror tests/test_touch_filter.cpp -o /tmp/test_touch_filter
/tmp/test_touch_filter
```

The C++ test checks contact bounce, short noise pulses, consecutive tiles,
repetition within a single touch, `millis()` overflow, and temperature rounding.
The layout test checks overlap, screen bounds, space for navigation,
and the filter ahead of both handlers for all ten tiles.

With ESPHome's Python environment (with aioesphomeapi, PyYAML, and pyserial):

```sh
python diagnostics/capture_serial.py --reset --seconds 120 > diagnostics/runtime.log
python diagnostics/run_ui_test.py --host cyd-2432s028.local
```

The API test reads the encryption key internally from `secrets.yaml` and doesn't
print it. It switches pages ten times and draws the five overlays on each
round. It doesn't call any Home Assistant actions. Don't touch the screen during
these roughly 11 seconds. This is a rendering/memory test; it doesn't replace
a physical check of the touch calibration or a test of HA service effects.

Diagnostics logs free heap and the largest free memory block every 30 seconds.
`DEBUG` is useful temporarily for touch and connection diagnostics. For everyday
use, `logger.level` can go back to `INFO` once physical operation is confirmed good.

## Touch diagnostics

`python diagnostics/control_ui.py touch_diagnostics` opens a separate screen
with four crosshairs. During this measurement, touches can't trigger a tile
action. Tap accurately, in order: top-left, top-right,
bottom-right, and bottom-left. The raw and normalized coordinates appear in
the serial logs with tag `touch`. Exit with
`python diagnostics/control_ui.py end_touch_diagnostics`. The crosshairs sit
at (20,20), (299,20), (299,219), and (20,219) for 320×240 landscape.

The current calibration (X 225–3800, Y 274–3609) is derived from the three
repeated measurements at top-right and bottom-left on this physical board. The earlier
top-left measurements were unstable and were deliberately not used, to avoid
shifting the entire screen to the wrong position.

On top of that, `set_raw_correction(...)` in `on_boot` corrects the measured
skew. The fit uses the filtered corner medians (830,556), (544,3461),
(3410,3391), and (3492,372). After correction, the four medians land within five
pixels of their measurement target in the regression test. These are board-specific
coefficients; don't carry them over blindly to another panel. The `raw=` logs
still show the physical, filtered ADC readings.
