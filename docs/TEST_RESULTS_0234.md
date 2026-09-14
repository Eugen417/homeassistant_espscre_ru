# Testresultaten app 0.2.34 / firmware 0.2.29 (2026-09-14)

Hotfix: de randveeg uit 0.2.32/0.2.33 vuurde nooit. Live op Studio 1 (firmware 0.2.28):
`GT911 press x=5` en `x=477`, daarna niets, ook geen `randveeg genegeerd` of
`randveeg niet gevuurd`. Oorzaak in de meegeleverde LVGL 9.5.0 (`lv_indev.c`,
`send_event()`): het invoerapparaat krijgt PRESSED, RELEASED, CLICKED, LONG_PRESSED en KEY,
maar geen PRESSING, dus de `lv_indev_add_event_cb`-veeg kreeg nooit positie-updates.

## Wat veranderde

- `cyd::EdgeSwipe` werkt weer op de native touchpunten van ESPHome's `on_touch`
  (`begin` met de LVGL-rotatie), `on_update` (`update`, paginawissel met
  `touch_guard.consume()` en `lv_indev_wait_release` op elk invoerapparaat) en
  `on_release` (`end()`, plus de logregel `randveeg niet gevuurd`). Dezelfde route als
  0.2.24, die live werkte zodra de kaartfix (`close_cards`, 0.2.32) erbij zat.
- `end()` bij loslaten voorkomt dat een niet-afgemaakte veeg bij de eerste `on_update` van
  de volgende aanraking (ESPHome vuurt die vóór `on_touch`) alsnog afgaat.
- De indev-callback is verwijderd; 45°-regel en `randveeg genegeerd: <reden>` blijven.

## Geautomatiseerd

- C++ (clang++ -std=c++17 -Wall -Wextra -Werror -I.): 11/11 PASS; `test_cyd_ui` dekt de
  rotaties, `end()` en de logwaarden.
- Python: 91 tests OK (`FIRMWARE_VERSION` 0.2.29 in app, profielen en pakketten);
  `generate_packages.py --check` groen; bench-kopieën opnieuw gegenereerd.
- ESPHome 2026.6.2 compile: `easy-guition-device.yaml` SUCCESS in 39 s (RAM 32,0%, flash
  18,3%), `easy-cyd-device.yaml` SUCCESS in 36 s (RAM 31,0%, flash 76,1%); geen nieuwe
  waarschuwingen.

## Te controleren op Studio 1 na de update

- Vegen vanaf beide randen zonder eerst een kaart te sluiten; na een lichtkaart openen en
  sluiten; en de eerste veeg direct na een paginawissel. Het log meldt per veeg
  `randveeg: pagina …`, `randveeg genegeerd: …` of `randveeg niet gevuurd: …`.
