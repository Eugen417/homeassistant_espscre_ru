# Upgrading an existing CYD panel to runtime 0.2.7

Board: ESP32-2432S028, ESP32-D0WD-V3; MAC b0:cb:d8:e7:bd:7c.
Kept the existing identity CYD 2.8in Display / cyd-2432s028. The USB port was
identified beforehand with esptool; the Guition was on a different USB port.

Own profile: gitignored easy-cyd-device.yaml, with the existing local
calibration.yaml and API/OTA/Wi-Fi secret references. A backup of the first
64 KiB (partition/configuration space) is stored locally. A full fast
read gave a serial error; the full slow read was aborted.
So this is not a complete old-firmware backup. USB upload at 115200 succeeded;
further correction via the verified OTA host.

The existing affine calibration was written once into preference key 0x43594403.
After that, the final profile uses the standard boot and existing NVS.

## Found during the upgrade

A temporary on_boot list in the local migration profile replaced the package's
on_boot object. As a result, the normal UI initialization was missing and
the start of the render diagnostic crashed. The temporary import was removed after the
calibration had been saved. In the final generated C++, runtime-bind,
light_controls::setup, and screen_calibration::setup are present again. The full
render diagnostic then passes (ten page checks and fifty overlays).
Don't use this temporary migration profile for future flashes.

HA discovered the new Tegelinstellingen entity without a new API key. The old
ten entities were carried over; the user then changed the layout.
That current layout is preserved. Swipe is on, standby 600 seconds. With six
tiles or fewer, pagination stays hidden.

Final build: ESPHome 2026.6.2, 2026-09-12 17:50:49 +0200,
config_hash=0x0a15a59f, source commit a14a66f.

## Verification

- Final build and OTA upload succeeded; identity and firmware version checked
  via the encrypted API.
- Render diagnostic: ten successful page checks, fifty overlay cycles, and
  `Light sliders: PASS`. The diagnostic used the two tiles configured at the time;
  this is not a physical swipe check of four pages.
- Home Assistant and ESP Screen Manager report the panel online with version 0.2.7.
  The user's changes to the tile layout are preserved.
- Existing calibration reused; no new independent physical calibration
  measurement was performed during this upgrade.

The 0.2.7 observation ran for fifteen minutes complete, with a live API response every
thirty seconds, with no restart or connection loss. Free memory around
111–119 kB. Because of physical use, ten minutes of uninterrupted inactivity
was not reached; standby/wake was not confirmed in this observation.

## Follow-up: text layout 0.2.8

The owner then reported titles running off the edge, subtitles truncated too early,
and an overlapping mini-slider. So stability alone doesn't confirm the
layout's usability. The titles had no explicit width, and the
subtitle inherited the title's content width. The compact card also had
insufficient vertical space for the extra slider.

Runtime 0.2.8 bounds both lines to the available content width, expands that
when an icon is hidden, and reserves separate space for the slider on the CYD.
The render diagnostic now checks the real LVGL coordinates for clipping outside the
box and overlap between text lines/slider. With the current four tiles, including
a light with a mini-slider: ten page checks and fifty overlay cycles passed.
The slider test also passes. Physical confirmation of the new layout is still open.
The fifteen minutes above apply to 0.2.7, not to this follow-up build.
