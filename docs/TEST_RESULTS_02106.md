# Test results 0.2.106 (firmware 0.2.91)

A board says whether its screen can go dark at all (`CAN_STANDBY`), and the Waveshare says no.

## What was found first (2026-09-21, the bench Waveshare on USB and on a 2 A adapter)

- Every wake from a dark standby (standby brightness 0 → EXIO2 low) reset the board. Serial, last wake:
  `[I][standby] Wake from Home Assistant` → `E BOD: Brownout detector was triggered` → `rst:0x3 (RTC_SW_SYS_RST)`.
  EXIO2 also enables the MP3302 boost behind the backlight LEDs; its inrush pulls the 3.3 V rail under the
  brownout level.
- A wake the ESP survived left the bus dead: `ch422g: write failed for register 0x23, error 2` and
  `touchscreen: Communication failed`, the screen lit and nothing to tap until a restart.
- Tried and measured, none of it helps: drawing the page before lighting the panel (`lv_refr_now`), a 400 ms or a
  250 ms wait, the panel idle. Thirty idle flips of the line "survived" over the API - and so did every scripted
  cycle after the first wake, because the expander was dead and the line never moved again. A count over the API
  is not a result; an eye on the glass is.
- The community's answer for a Waveshare that must go dark: a 470 µF capacitor on the 3V3 pins of the Sensor/AD
  connector (medium.com/@pvginkel, myseringan/esp32-s3-touch-lcd-backlight-fix, openHASP #602).

## What 0.2.106 does, verified

| Check | Result |
| --- | --- |
| `tools/check.sh` (Python tests, every `tests/*.cpp`, package check, boards.json `--check`, editor tests, types, build, bundle) | 13 passed, 0 warned, 0 failed after updating the one 0.2.105 assertion that a Waveshare still had `standby_brightness` |
| `tests/test_settings_screen.cpp`: with `can_standby` false the standby rows, the Night page and Also on standby are hidden, Brightness/Dark mode/Back to page 1 stay; with `dimmable` false as well neither the percentage nor the switch shows | PASS |
| `tests/test_screen_owned_settings.py`: `core.can_standby()` per board; a Waveshare's settings view has none of `STANDBY_KEYS`, keeps dark_mode, auto_home, swipe_pages, page_buttons | PASS |
| `tests/test_easy_package.py`: every board file says `CAN_STANDBY`; the one that says false `!extend`s the ten standby entities `internal: true`, the others none; boards.json carries the same answer | PASS |
| `esphome config waveshare-esp32s3-43.yaml`: the ten `!extend` entries resolve, 11 × `internal: true` in the resolved config (ten plus the internal backlight light) | PASS |
| Waveshare firmware built and flashed over USB (image 2026-09-21 23:51:11, `Screen firmware` 0.2.91) | PASS |
| Entities over the native API after the flash: no Auto standby, Standby after, Standby brightness, Night mode, Night starts, Night ends, Night brightness, Back to page 1 on standby, Sleep, Wake; Normal brightness, Dark mode, Back to page 1 (+ after), Swipe between pages, Page buttons, Rotation, Restart still there | PASS |
| Serial after the flash: no `ch422g` or `touchscreen` warnings, no reset | PASS |
| On the glass (Max): touch works; the settings page shows no Auto standby and no Night page; the screen stays lit past the old standby time | PASS |
| CYD firmware built from the same tree (`home-like-2432s028.yaml`): 1,622,208 bytes, 88.4 % of the update slot, built with the ESPHome this add-on ships (2026.9.0); 112 bytes more than 0.2.105 | PASS |

## On the glass

Max, at the bench Waveshare right after the USB flash (2026-09-21, 23:55): touch works; the settings that should
not be there are gone (no Auto standby on the Brightness page, no Night page in the menu); the screen had not gone
dark by itself since the flash. Before this round the same board reset on every wake from a dark standby.
