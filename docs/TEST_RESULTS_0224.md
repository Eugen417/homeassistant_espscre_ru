# Testresultaten app 0.2.24 / firmware 0.2.20 (2026-09-13)

Vervolg op 0.2.23: doorlichting van de rendering, minder geheugen per tegel, zichtbare
greep op de mini-slider, CYD-kaarten in het lichte palet, wifi zonder modem-slaap.

## Doorlichting (wat is gemeten, wat is veranderd, wat bewust niet)

- LVGL 9.5 via ESPHome 2026.6.2 tekent al partieel (`LV_DEF_REFR_PERIOD 16`, kernloop 16 ms,
  alleen ongeldig gemaakte gebieden gaan naar het paneel; `auto_clear_enabled: false`).
  `lv_obj_set_x/y/width/height` slaan een ongewijzigde waarde over, maar elke
  `lv_obj_set_style_*` maakt het object ongeldig. Daarom (0.2.23) alleen nog zetten bij een
  echte wijziging en geen herrender meer elke 250 ms tijdens *bezig*.
- `Tile` was 2016 bytes × 20 = 40 KB statisch; de dag- en uurvoorspelling zitten nu in
  vectoren die alleen weertegels vullen: 1216 bytes × 20 = 24 KB. CYD vrije heap tijdens de
  selftest met de 18-tegels demo: 20–26 KB (0.2.19) → 34–37 KB (0.2.20). Guition 6,33 → 6,35 MB.
- Niet veranderd, bewust: de paginawissel tekent twee keer (skelet, dan inhoud; gewenste
  directe feedback); GT911 op 20 ms; `buffer_size` 25% (PSRAM) en 12% (CYD).
- `power_save_mode: none` in wizard-YAML en bordprofielen (hangt aan het lichtnet).

## Geautomatiseerd

- Python: 82 tests OK. C++: 11 tests PASS. `generate_icons.py --check`, `generate_packages.py --check` groen.
- Beide profielen compileren; OTA naar beide borden; `device_info` meldt 0.2.20.

## Op de borden

- Guition en CYD `ui_self_test` met `send_layout.py --controls` (18 tegels): 10/10 PASS,
  geen geometrie-, paneel- of paletfouten.
- Nog fysiek te bekijken door Max: greep op de mini-slider, CYD-lichtkaart in het lichte palet.
