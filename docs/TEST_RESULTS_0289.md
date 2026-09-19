# Test results app 0.2.89 / firmware 0.2.75 (2026-09-19)

Max asked whether ESPHome's defaults leave room for faster builds and smaller firmware, and after the report
(flash and build time, 2026-09-19) said: "ga het doen, maak het toekomstbestendig". This round implements it: ESPHome
2026.9.0 in the add-on, the add-on's build memory out of the ESPHome folder, and three settings that make the CYD's
firmware smaller. Everything stays on ESPHome's and ESP-IDF's own options; no lwIP, Wi-Fi or LVGL settings.

## What changed

| | Before (0.2.88) | Now (0.2.89) |
|---|---|---|
| ESPHome in the add-on | 2026.6.2 (PlatformIO) | 2026.9.0 (ESP-IDF itself, ccache) |
| The packages' `min_version` | 2026.6.2 | 2026.6.2, on purpose (below) |
| ESPHome's build memory (storage JSON, fetched packages) | `/homeassistant/esphome/.esphome` | `/data/esphome` |
| ESP-IDF, its tools, the compiler cache | PlatformIO in `/data/platformio` | `/data/idf`, ccache at most 1 GB |
| ESP-IDF assert messages | in the firmware | left out (`assertion_level: SILENT`, every board) |
| Log level of the screen | DEBUG | INFO |
| Timer and sun times | `sscanf` | `clock_parts()` in `runtime_model.h` |
| A running timer's countdown | could read one more than its duration | at most its duration |

Three items of the report were left out or put off on purpose:

- **`min_version` 2026.9.0**, with the Guition's `image: - platform: online_image` and `ota: encryption:` for new
  screens. It was in the branch and on both bench screens; before the release Max asked whether everyone can update
  smoothly, and this is where they couldn't. Screens build from the packages on main whatever ESPHome builds them: an
  add-on not yet updated has 2026.6.2, Max's own ESPHome Device Builder had 2026.8.1. ESPHome 2026.6.2 knows neither
  the `image:` form of `online_image` nor `ota: encryption:` (checked in its source), so every such build, the nightly
  round included, would have failed until its owner updated. The packages now use nothing their `min_version` lacks;
  raising it is a release of its own, before ESPHome 2027.1 drops the top-level `online_image:`. CI builds both
  boards with the add-on's ESPHome and with `min_version` from now on.

- **ESP-IDF's error names** (`CONFIG_ESP_ERR_TO_NAME_LOOKUP: n`, 7.2 KB). The report said error codes would then be
  logged as a number. Checked in ESP-IDF 5.5.5 (`esp_err_to_name.c`): without the table `esp_err_to_name()` returns
  "UNKNOWN ERROR", and ESPHome's own error lines print only that name (30 of them in its Wi-Fi code, none with the
  number). A failing Wi-Fi start would then say nothing useful.
- **`esphome: build_flags`** instead of `platformio_options: build_flags`. The move made the CYD fail to compile:
  ESPHome 2026.9's LVGL reads only `platformio_options` when it writes `lv_conf.h` and wrote `#define LV_USE_SWITCH 0`
  over the flag (`'lv_switch_create' was not declared`). The dev branch of ESPHome still does; no issue or pull request
  about it was found. The old form works until ESPHome 2026.12 and prints a deprecation warning.

## A running timer read one second too many

Max, testing the Guition: the idle tile of `timer.ok_nabu_timer` (duration `0:00:03`) read 0:03, but a tap started it
at 0:04, and it never came below 0:02 before Home Assistant said it had finished. Not a change of this round (the
countdown and ESPHome's time sync are the same in 0.2.74 and ESPHome 2026.6.2): the tile subtracts the screen's clock
from Home Assistant's end time (`finishes_at`, whole seconds from the add-on), and the screen sets its clock from Home
Assistant's `epoch_seconds` every minute, so it runs up to a second behind; a timer started early in a second then
reads one more than it lasts, and Home Assistant's finish comes while the tile still shows 0:01 or 0:02.
`timer_left()` (`runtime_model.h`) now caps the countdown at the timer's duration; the end stays up to a second early,
since the screen gets no finer time from Home Assistant. `tests/test_runtime_model.cpp` holds the cases.

## Firmware size

`tools/check.sh --firmware --baseline 1678816` (ESPHome 2026.9.0, the check profiles: fallback hotspot, captive
portal, OTA password):

| | 0.2.88 on 2026.6.2 | 0.2.89 on 2026.9.0 | |
|---|---|---|---|
| CYD image | 1,678,816 B, 91.5 % | 1,585,136 B, 86.4 % | −93,680 B |
| CYD, OTA with the api key (the next step) | | 1,584,208 B, 86.3 % | −912 B more |
| Guition image | 2,055,280 B, 25.3 % | 1,945,152 B, 23.9 % | −110,128 B |

The firmware of this release with each ESPHome a screen may be built with (`tools/check.sh --firmware` per version,
the same check profiles):

| ESPHome | Who builds with it | CYD | Guition |
|---|---|---|---|
| 2026.6.2 (`min_version`) | an ESP Screens not yet updated | 1,609,856 B, 87.7 % (−68,960 B) | 1,982,896 B, 24.4 % |
| 2026.8.1 | an ESPHome Device Builder like Max's | 1,620,640 B, 88.3 % (−58,176 B) | 1,998,432 B, 24.6 % |
| 2026.9.0 | ESP Screens 0.2.89 | 1,585,136 B, 86.4 % (−93,680 B) | 1,945,152 B, 23.9 % |

Of the CYD's 93.7 KB, the report's single measurements attribute 49.0 KB to the silent asserts, 8.9 KB to INFO and
9.5 KB to `sscanf`; ESPHome 2026.9 itself saves about 24 KB (its API encryption and the ESP-IDF parts it leaves out).
The European letter set of the language plan (+65.8 KB) now fits at about 90 %.

## Bench screens (over the air from the Mac)

Copies of the Device Builder YAMLs (`guition-wallbox.yaml`, `cyd-2432s028.yaml`: keys, Wi-Fi and the CYD's
calibration unchanged, compared value by value with the root `secrets.yaml`) with the packages, components and fonts of
this branch, built with ESPHome 2026.9.0:

| | Guition Wallbox .136 | CYD .134 |
|---|---|---|
| Before | 0.2.74, ESPHome 2026.6.2 | 0.2.71, ESPHome 2026.6.2 |
| Build on the Mac | 127 s | 100 s |
| Image | 1,945,088 B | 1,489,216 B (its YAML has no hotspot) |
| OTA | 13.0 s, password (plaintext fallback) | 15.4 s, password (plaintext fallback) |
| `device_info` after | 0.2.75, ESPHome 2026.9.0, 10:29:09 | 0.2.75, ESPHome 2026.9.0, 10:31:51 |
| Home Assistant | `Synced`, online | `Synced`, online |
| Free heap 5 min after boot | 86,940 B (fresh boots on 0.2.50: 85-86 KB) | 114,724 B (was 101,712 B) |
| Largest free block | 45,056 B | 57,344 B (was 49,152 B) |

This first flash had the Guition's `image:` form of `online_image`. The released firmware (the top-level form back,
the timer cap added) went onto both the same way at 12:32 and 12:34, now over an encrypted connection with the api
key, since their 2026.9 firmware offered it: Guition 1,945,152 B, CYD 1,489,232 B, both 0.2.75 on ESPHome 2026.9.0 in
`device_info` and `Synced` in Home Assistant.

Max tested both screens by hand for half an hour on the first flash while their logs (over the API, INFO) and Home
Assistant entities were watched from the Mac:

- 0 errors in about 450 log lines; one restart, Max's own from the settings page ("restart from the screen"), back
  in 9 s. No lost connection otherwise; both stayed `Synced`.
- 29 taps confirmed by Home Assistant in 256-486 ms (median 343 ms); the light effects page (select and number
  entities) changed them in Home Assistant; a docked robot's return to base had nothing to confirm.
- Sonos covers and 22 camera pictures in a row loaded without a failure; the first camera picture 5 s after the tap
  (EZVIZ snapshot included, 4.5 s in 0.2.87).
- Free heap steady: Guition about 80 KB (largest block about 34 KB), CYD 98-115 KB (51-56 KB), as before the update.
- ESPHome's "took a long time" warnings at boot, on opening a card (CYD LVGL 356 ms) and while pictures came in; one
  picture of the camera that took 4 s instead of 2 held the loop 0.3-1.2 s in `online_image`, whose read loop is the
  same as in 2026.6.2 (a slow download, not this release). A full redraw Max saw on the CYD was Back to page 1 after
  120 s without a touch.

OTA paths, all on the Guition with the same 0.2.75 image:

- ESPHome 2026.9 CLI, profile with a password, device on 2026.6.2 firmware: "The device did not offer OTA encryption;
  continuing in plaintext", success.
- The same again, now that the device runs 2026.9 firmware: "Encrypted connection established" with the api key,
  success (14.5 s; the upload itself takes 14 s encrypted against 8 s in plaintext).
- ESPHome 2026.6.2's uploader (`espota2`) with the password, as the add-on before 0.2.89 and the Device Builder 2026.8
  upload: success. A screen on 0.2.75 stays reachable for older tools that know its password.

## On the Yellow (local add-on)

This branch's `screen_manager/` as a local add-on (`local_esp_screen_manager` 0.2.89, SMB share `local_apps` and the
Supervisor), next to the store add-on 0.2.85: its camera port set to none, no layouts, so it sent nothing to the
screens. Its image built in 44 s from `ghcr.io/esphome/esphome:2026.9.0` (aarch64). It built the Device Builder
profiles through ESP Screens' own firmware jobs (build only, no upload), so with the packages on GitHub's main
(firmware 0.2.74); the owner's ESPHome folder stays read-only, and this branch's packages were compiled on the Mac.
Uninstalled afterwards, and the copy on the share removed.

| Build on the Yellow | 2026.6.2 with PlatformIO (before) | 2026.9.0 in this add-on |
|---|---|---|
| Guition, first full build | 920 s (2026-09-12) | 1,379 s, of which the first ESP-IDF install into `/data/idf` |
| CYD, first full build | 1,154 s (2026-09-18 23:35) | 1,037 s, ESP-IDF already there |
| Guition again, nothing changed | not measured | 41 s |

- The build went where `Firmware.build_env` says: `/data/build/guition-wallbox/guition-wallbox/build/`, ESPHome's
  packages in `/data/esphome/packages/`, ESP-IDF 5.5.5 in `/data/idf` ("ccache will be used for faster
  recompilation"). The Download of the built firmware gave `guition-wallbox.factory.bin`, 2,083,728 B, starting with the
  ESP image byte 0xE9.
- A first build is about as long as before: a cold ccache saves nothing, and ESP-IDF builds more files than
  PlatformIO did (1,699 steps for the Guition). The gain is in what follows. The build folder and ESPHome's memory of
  it now survive a restart of the ESPHome Device Builder, which emptied `/homeassistant/esphome/.esphome` and made the
  next update of every screen a full build again (the CYD's 18-minute build of 2026-09-18 was one). A new firmware
  version still recompiles ESPHome's own sources and `main.cpp`, because the version is a define in `defines.h`: 59 s
  on the Mac against 46 s with PlatformIO (the report's measurement), so about ten minutes a screen on a Yellow,
  estimated with the Mac-to-Yellow ratio of these builds (107 s against 1,037 s for the CYD).
- The Supervisor showed the add-on at 94 % CPU and 1.2 GB of memory while it built, with ninja's default of cores + 2
  compilers. The released add-on starts as many as the machine has cores (`ESPHOME_DEFAULT_COMPILE_PROCESS_LIMIT`), as
  PlatformIO did, so a small Raspberry Pi builds with about as much memory as before (not measured on one). The Yellow's disk: 60 GB used of
  917 GB.
- ESP-IDF's GCC warns `array subscript 6 is above array bounds of 'Key* [6]'` in the Guition's climate card lambda
  (`packages/boards/guition-4848s040.yaml`, `visible[room - 1] = visible[i]`): on the Guition `room` is 6 and there
  are six keys, so that loop never runs. Left as it is.

## Automated

- `tools/check.sh` (Python 3.13.14, `.esphome/venv313`): Python 478 tests OK (new: `tests/test_build_env.py`, the
  OTA line in `test_portal`, every board's `assertion_level` in `test_easy_package`), C++ 19/19 (new: clock texts in
  `test_runtime_model.cpp`), packages, icons, editor tests, types, build and bundle PASS.
- `tools/check.sh --firmware --baseline 1678816` with ESPHome 2026.6.2, 2026.8.1 and 2026.9.0: PASS (table above).
- The Mac's ccache after this round and the report's builds: 27,986 compiler calls (about seventeen full builds of
  both boards), 0.6 GB. The add-on caps it at 1 GB (`Firmware.CCACHE_SIZE`); past that ccache drops the oldest entries.
- `clock_parts()`: identical answers to the `sscanf` forms for two million random strings of digits, colons, spaces,
  tabs, dots and letters.

## Not tested

- Studio 1: not touched. It gets 0.2.75 with its next **Update** once its ESP Screens has 0.2.89.
- `tools/render_topbar.py` does not build, on ESPHome 2026.6.2 either: its host project lacks the roller the effects
  page (firmware 0.2.70) uses. Left for a separate round.
