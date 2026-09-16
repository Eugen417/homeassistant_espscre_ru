# Test results app 0.2.53 / firmware 0.2.45 (2026-09-16)

Wake and Sleep buttons for Home Assistant automations. Firmware and board profiles change; the app only
raises `FIRMWARE_VERSION` and extends the Claude skill. Storage, tile protocol, preferences and keys are
unchanged. See CHANGELOG 0.2.53 and the compatibility note in docs/RELEASING.md.

## Automated

- Python (`.venv-portal`): 206 tests OK. New `tests/test_wake_sleep.py` (6 tests, run against both
  board profiles and both generated packages): the Wake and Sleep buttons exist as template buttons
  without an entity category and not internal; Wake runs `wake_display` on a dimmed screen (the same
  script the tap on `dim_wake_overlay` runs) and only restarts `last_touch_ms` otherwise; Sleep is
  ignored during a touch calibration and otherwise sets `sleep_requested`, closes an alert
  (`remote`) and runs `dim_display`; `dim_display` and `apply_screen_settings` honour the flag;
  `wake_display` clears it; the flag is a non-restored bool; the Auto standby switch clears it only when
  it really turns standby off. `tests/test_claude_skill.py` checks both buttons, the minimum firmware
  and the new YAML example (nine blocks). `tests/test_settings_page.py` now asks for firmware 0.2.44
  or newer instead of exactly 0.2.44.
- C++ (`clang++ -std=c++17 -Wall -Wextra -Werror -I.`): 14/14 PASS (no C++ changed).
- `generate_packages.py --check` green. `generate_icons.py --check` not run: no glyphs changed, and
  fontTools is not installed in this environment.

## Builds (ESPHome 2026.6.2)

Both Easy Setup profiles, compiled one after the other from packages generated out of the board profiles:

| Profile | RAM | Flash | vs 0.2.52 |
|---|---|---|---|
| easy-cyd-device.yaml (package, local components) | 23.1% (75,612 B) | 76.1% (1,396,427 B) | +336 B RAM, +1,284 B flash |
| easy-guition-device.yaml (package, local components) | 24.1% (78,844 B) | 18.4% (1,496,071 B) | +336 B RAM, +1,996 B flash |

## Behaviour on the real profile logic (host build)

The generated packages were built for the Mac with ESPHome's host platform (SDL display, template
backlight output, host time; the readme-render tooling) under their own device names, and driven over
the native API: button presses, the Auto standby switch, the profile's own actions (`show_alert`,
`dismiss_alert`, `touch_diagnostics`), plus three test-only actions that log `display_dimmed`,
`sleep_requested`, the wake overlay, the backlight target and the idle time, click the wake overlay,
and run `apply_screen_settings`. Result: 24/24 checks passed on the Guition package and 24/24 on the CYD package.

Checked on both boards:

1. Boot: awake, Auto standby on, full brightness. Buttons listed as `wake` / `sleep` with
   `mdi:gesture-tap` / `mdi:power-sleep`, entity category none.
2. The standby timer still dims after the standby time, and a tap on the overlay wakes.
3. Sleep with Auto standby on dims at once to the standby level, overlay up, `Sleep from Home
   Assistant` in the log; Wake brings full brightness back and clears the flag.
4. Wake on a screen that is on restarts the idle count (300 s -> under 3 s) and changes nothing else.
5. With Auto standby off, Sleep dims; an explicit `apply_screen_settings` and a real minute tick
   (`on_time`) both keep it dimmed. Turning the already-off switch off again keeps it dimmed; a tap on
   the overlay wakes.
6. Sleep with Auto standby on, then switching Auto standby off: the screen wakes.
7. Sleep during an alert closes the alert (`alert dismissed: remote` in the log) and dims; a new alert
   wakes the screen out of Sleep.
8. During `touch_diagnostics` (calibration active) Sleep is ignored with a log line.

## Not tested / for the owner

- Hardware. The bench boards (192.168.146.134 / .136) were not reachable, and the only screen online in
  Home Assistant, Studio 1 (Guition, firmware 0.2.43), is in daily use and gets its firmware through
  ESP Screens; nothing was flashed. Still to see on real glass: the backlight fade into Sleep (1.5 s on
  the Guition's LEDC fader), and pressing `button.<screen>_wake` / `_sleep` from Home Assistant after
  the update.
- The `esphome.screen_alert` event with `remote` after Sleep closes an alert, as Home Assistant
  receives it (the host run shows the dismiss path and its log line only).
