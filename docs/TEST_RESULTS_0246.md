# Test results app 0.2.46 / firmware 0.2.39 (2026-09-15)

Cleaning mode, suction and water on the vacuum card, the vacuum card redesign on both boards, the
new CYD climate card and the cleared climate edit flag. See CHANGELOG 0.2.46 and RELEASING
"Compatibility 0.2.46".

## Automated

- Python (`.venv-portal`): 170 tests OK.
  - New: `tests/test_vacuum_card.py` covers the extras on the owner's Roborock data (Dutch entity
    ids), a robot without a mode select, a plain vacuum, entity id endings, disabled entities and
    unknown modes (an Ecovacs-style `work_mode`), bounded options, and that a select change marks
    the vacuum dirty and resends only its message.
- C++ (`clang++ -std=c++17 -Wall -Wextra -Werror -I . -I components/smart_display`): 13/13 PASS;
  `test_tile_controls.cpp` adds `mode_color`, `vacuum_rows`, `vacuum_role`, `shown_value`,
  `settle_suction` and `choice_action`.
- `generate_packages.py --check` green, `generate_icons.py --check` verified (150 pickable icons,
  179 glyphs).
- ESPHome 2026.6.2 compile SUCCESS, one after another: `easy-cyd-device.yaml` (RAM 31.9%, flash
  78.5%), `easy-guition-device.yaml` (RAM 32.8%, flash 18.8%), `guition-device.yaml` (RAM 32.9%,
  flash 19.8%), `device.yaml` (RAM 32.0%, flash 83.2%). Packages carry `SCREEN_FIRMWARE_VERSION` 0.2.39.

## Home Assistant facts (owner's HA 2026.9.1, read only)

- Roborock S8 (`roborock.vacuum.a51`): `vacuum.s8` has no `battery_level`; its device holds
  `select.woonkamer_s8_schoonmaakmodus` (translation key `cleaning_mode`: vacuum, vac_and_mop,
  mop, custom), `select.s8_intensiteit_van_dweilen` (`mop_intensity`: off, mild, standard, intense,
  custom), `sensor.s8_batterij` (battery), `binary_sensor.s8_opladen` (battery_charging) and
  `sensor.s8_huidige_kamer` (`current_room`). Entity ids follow the UI language, so matching goes
  by translation key.
- The Roborock cleaning mode select exists in HA 2026.8.0 and 2026.9.1 (`RoborockCleaningModeSelectEntity`).
- `core.extras` on this data: mode vacuum / vac_and_mop / mop (roles `vbm`), water mild / standard /
  intense, suction quiet … max_plus, battery, charging and room; the message is 612 bytes.

## Bench boards (OTA)

- Both Easy Setup builds uploaded over the network; `device_info` reports project 0.2.39, compiled
  2026-09-15 15:11:48 (CYD) and 15:12:20 (Guition), matching the build logs.
- The real `vacuum.s8` state with the new extras, built by the app code from live HA data, was sent
  through `screen_message` and the card opened with `preview_runtime_card` on both boards. The
  Guition capture (`capture_ui.py`) shows Docked, 83 % with the charging bolt, Start cleaning and
  Dock, Vac & mop, Max and Intense. The owner looked at both screens.
- The owner tapped Vacuum on the CYD: Home Assistant switched `select.woonkamer_s8_schoonmaakmodus`
  to `vacuum` (15:16:12) and the robot set its water to off and suction to balanced. The production
  app was still 0.2.45 and resent the vacuum without the new extras, so the card fell back to
  suction only, as described for mixed versions in the CHANGELOG.

## Renders (ESPHome host + SDL, both boards, this release's code)

- Vacuum card with the owner's data in ten states: docked and charging, cleaning with the room,
  mop only, vacuum only, Custom (note instead of rows), returning, error, paused at 12 % (red
  battery), a vacuum without selects (small hero on the CYD) and an Ecovacs-style robot with long
  labels (Vac, then mop; Ultra high). No label is cut off; the mode row keeps its place.
- CYD climate card: heating with humidity, an air conditioner with five modes plus ···, and off
  (grey setpoint). The README images `guition-vacuum.png`, `cyd-vacuum.png` and `cyd-climate.png`
  come from these renders.

## Not tested / for the owner

- The vacuum rows with app 0.2.46 on the Yellow after the store update (live updates of mode,
  water, battery and room while the robot cleans).
- The CYD climate card by hand with the real air conditioner (− / + taps, holding, mode keys, ···).
- Other brands only through the tests with their documented entity names; no Dreame or Ecovacs
  robot was available.
