# App 0.2.18 / firmware 0.2.18 — eigen icoon per tegel

## Wijzigingen

- `screen_manager/app/tile_icons.py`: 150 kiesbare MDI-iconen in twaalf groepen
  (20 voor media en muziek) plus 9 vaste firmwareglyphs; codepoints gecontroleerd
  tegen `fonts/materialdesignicons-webfont.ttf`.
- Firmware: `tile_icon.h` (hex → codepoint → UTF-8), `Tile::icon`, `icon_for()`
  gebruikt een gekozen icoon als de font de glyph bevat.
- App: validatie van `options.icon`, `o.icon` met codepoint (gekozen of uit HA),
  entiteiticonen in de inventaris, icoonkiezer met zoeken in het tegelpaneel.

## Tests

- `.venv-portal/bin/python -m unittest discover -s tests`: 69 tests OK, waaronder
  `tests/test_tile_icons.py` (set in alle zes fonts en beide pakketten, elke
  firmwareglyph in de set, editorvoorspelling gelijk aan `icon_for`/`weather_icon`,
  validatie, draadformaat, discovery, oude editor behoudt icoon).
- Alle `tests/*.cpp` OK, waaronder `test_tile_icon.cpp`.
- `tools/generate_packages.py --check` en `tools/generate_icons.py --check`: actueel.
- `esphome compile`: CYD 1.361.735 bytes flash (74,2%, was 1.339.824), RAM 30,2%;
  Guition 1.454.503 bytes (17,9%).
- USB-flash na `esptool chip_id` (`usbserial-130` = ESP32/CYD, `usbserial-210` =
  ESP32-S3/Guition); uploadlog toont de juiste `firmware.bin`. `device_info`:
  beide 0.2.18, gecompileerd 11:10:02 (CYD) en 11:10:52 (Guition).
- Guition-render via `send_layout.py` + `capture_ui.py`: thermometer in de
  grafiekstrook (mini-font), `account-child` automatisch uit HA, koksmuts op de
  timer, lamp op de dubbelbrede grote waarde (watch-font). Max bevestigde dat
  het op de schermen goed werkt.
- Lokale editor met nep-HA: kiezer, zoeken op "muziek", live mockup en kop,
  opslaan stuurt `F04C3`/`F0769`/`F0FCE`; hint bij firmware 0.2.17. Iconen in de
  browser gemeten gecentreerd na de bearing-fix (gemiddeld 0,3 px op 100 px).
