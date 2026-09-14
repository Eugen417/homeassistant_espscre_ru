# Testresultaten app 0.2.29 / firmware 0.2.24 (2026-09-14)

Vegen vanaf de zijrand op de Guition vervangt de LVGL-gesture over het hele scherm;
de CYD houdt zijn snelle veeg. In de app alleen de omschrijving van de instelling.

## Wat veranderde

- `cyd::EdgeSwipe` in `cyd_ui.h`: een aanraking die binnen `EDGE_SWIPE_BAND_PX` (32) van de
  linker- of rechterrand begint en `EDGE_SWIPE_TRAVEL_PX` (40) naar binnen aflegt, duidelijk
  meer zijwaarts dan verticaal (|dx| ≥ 2·|dy|), wisselt één keer per aanraking van pagina.
  Geen snelheidseis. De rotatie volgt ESPHome's LVGL-mapping (90°: x=y, y=W−x−1; 270°:
  x=H−y−1, y=x; 180°: beide gespiegeld), dus "linkerrand" is altijd de rand die de gebruiker ziet.
- Guition-profiel: `configure()` in `on_boot`, `begin()` in `on_touch`, `update()` in
  `on_update` voor het eerste contact; bij een treffer `touch_guard.consume()`,
  `lv_indev_wait_release()` en `show_tile_page` (die de pagina begrenst). De
  `LV_EVENT_GESTURE`-handler op `home_page` is verwijderd. Dezelfde blokkades als voorheen:
  instelling uit, gedimd, kalibratie, open kaart.

## Geautomatiseerd

- C++ (clang++ -std=c++17 -Wall -Wextra -Werror -I.): 11/11 PASS. `test_cyd_ui` dekt:
  42 px vanaf links = vorige, vanaf rechts = volgende, één keer per aanraking, start in het
  midden doet niets, diagonaal telt niet en naar buiten bewegen niet, later rechtgetrokken telt
  wel, en de drie rotaties.
- Python: 84 tests OK (`FIRMWARE_VERSION` 0.2.24 in app, profielen en pakketten).
  `generate_packages.py --check` groen; bench-kopieën opnieuw gegenereerd.
- ESPHome 2026.6.2 compile: `easy-guition-device.yaml` SUCCESS in 41 s (RAM 31,9%, flash
  18,3%), `easy-cyd-device.yaml` SUCCESS in 40 s (RAM 30,9%, flash 76,0%); geen nieuwe
  waarschuwingen.

## Nog niet geverifieerd

- Fysiek vegen vanaf de rand van de Guition (frame van de wallbox, eerste GT911-meting
  bij x≈0–10), inclusief een gedraaid scherm; en of de CYD-veeg onveranderd werkt na de
  herbouw. De bench-borden hingen niet aan USB en reageerden niet op de API. Max werkt bij
  via **Bijwerken** en kijkt in het log naar `randveeg: pagina 0 -> 1`.
