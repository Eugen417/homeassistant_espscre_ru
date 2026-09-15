# Test results app 0.2.48 / firmware 0.2.41 (2026-09-15)

The Auto standby switch for Home Assistant automations and the standby section of the Claude skill.
See CHANGELOG 0.2.48 and RELEASING "Compatibility 0.2.48".

## Automated

- Python (`.venv-portal`): 170 tests OK; `tests/test_claude_skill.py` checks the new section (the
  switch, the four numbers, the minimum firmware, the intro link) and parses the new example
  automation (five YAML blocks), including that it triggers on every entity its condition reads.
- C++ (`clang++ -std=c++17 -Wall -Wextra -Werror -I . -I components/smart_display`): 13/13 PASS.
- `generate_packages.py --check` green, `generate_icons.py --check` verified.
- ESPHome 2026.6.2 compile SUCCESS, one after another: `easy-cyd-device.yaml` (RAM 32.0%, flash
  78.6%), `easy-guition-device.yaml` (RAM 33.0%, flash 19.0%), `guition-device.yaml` (RAM 33.1%, flash 20.0%)
  and `device.yaml` (RAM 32.1%, flash 83.3%). Packages carry `SCREEN_FIRMWARE_VERSION` 0.2.41.

## Bench boards (OTA) and Home Assistant

Both boards run 0.2.41. The switch was driven only through Home Assistant's REST API, the way an
automation does it, and watched through the backlight light entity and the app's inventory.

- CYD (standby after 60 s): the screen had dimmed to 55 %. `switch.turn_off` switched the entity off and
  brought the backlight to 100 % within 1.1 s; the app's stored settings showed `standby_enabled: false`.
  After 80 s without a touch the screen was still at 100 %. `switch.turn_on` switched it on and the screen
  dimmed again after 61 s. The stored setting returned to `true`.
- Guition: `switch.turn_off` switched the entity off within 1.2 s and the screen stayed bright for 30 s;
  `switch.turn_on` switched it back on; the stored setting returned to `true`.

## Observed on the CYD, not caused by this release

- The first OTA of 0.2.41 crashed within its first minute and the bootloader rolled back to 0.2.40 by
  itself ("OTA rollback detected"). A second OTA of the same build booted, was marked successful and
  passed the test above without a restart.
- Around the crash the CYD logged LVGL failing to allocate 13.8 KB layer buffers and a task watchdog
  reset with a backtrace in the API connection code. The CYD also restarted twice on 0.2.40 while a
  diagnostic script opened API connections every few seconds. Heap after the boot sync is about 70 KB.
  This points to the CYD's memory headroom under bursts (boot sync, several API clients, object
  opacity layers), not to the switch; a separate check of the CYD's LVGL layer use is due.

## Not tested / for the owner

- An automation of the owner's own that switches Auto standby over a day.
- Installing the updated skill from Settings → Claude and letting Claude write the automation.
