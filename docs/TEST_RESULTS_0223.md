# Testresultaten app 0.2.23 / firmware 0.2.19 (2026-09-13)

Directe bediening op dubbelbrede tegels, weerkaart met uren en dagen, klimaatkaart
met aan/uit-knop en ventilator-/zwenkstanden, "Laatst"-status voor scènes, scripts
en knoppen, bezig-status als spinner, lichter tekenen, donker thema verwijderd.

## Geautomatiseerd

- Python: `.venv-portal/bin/python -m unittest discover -s tests` → 82 tests OK
  (nieuw: `test_tile_controls.py`, `test_rich_cards.py`).
- C++: alle `tests/*.cpp` met `c++ -std=c++17 -Wall -Wextra -pedantic -I.` → 11 PASS
  (nieuw: `test_tile_controls.cpp`; `test_cyd_ui.cpp` dekt `cyd::list_item`).
- `tools/generate_icons.py --check` (177 glyphs) en `tools/generate_packages.py --check` groen.
- ESPHome 2026.6.2: `easy-guition-device.yaml` en `easy-cyd-device.yaml` compileren
  (lokale componenten en fonts); beide via OTA geflasht, `device_info` meldt 0.2.19 (build 14:53).

## Op de borden

- Guition `ui_self_test` met de demo-indeling `send_layout.py --controls` (18 tegels,
  6 pagina's): 10/10 `page_check=PASS`, geen `Panel bounds`, `Tile geometry` of
  `GEOMETRY FAIL`; heap stabiel rond 6,33 MB.
- CYD `ui_self_test` met dezelfde demo: 10/10 PASS, heap 20–26 KB vrij.
- LVGL-snapshots (Guition, `capture_ui.py`) bekeken: gordijnknoppen, temperatuur-pill,
  volumeschuif met greep en dempknop, modusknoppen (actieve in accent), playback,
  stofzuiger (stop/dock grijs in het dock), toggle op pastelkaart, helderheidsschuif,
  −/+ voor getallen, keuze-chevrons, kookwekker, "Activeren"; weerkaart in twee kaarten
  (nu + komende uren, komende dagen met regen en hoog/laag); "Laatst 12:09" en
  "Gisteren 09:17"; spinner-laag op een bezige tegel; moduskiezer met drie chips per rij.
- De bezig-status wordt niet meer elke 250 ms herrenderd; stijlzetters in `render()`
  raken LVGL alleen bij een echte wijziging.

## Bekend / nog te doen

- De productie-add-on (0.2.22) stuurt `controls`, `x.hours`, `x.last`, `fan_modes` enz.
  nog niet; echte tegels tonen de nieuwe onderdelen pas na de add-on-update naar 0.2.23.
- De klimaatkaart-snapshot lukte niet in dezelfde run (de manager herstelde de echte
  indeling); de moduskiezer wel. Fysieke controle door Max: knoppen, −/+ debounce,
  spinner-vloeiendheid, weerkaart op de CYD.
- `capture_ui.py` mist af en toe enkele rijen (snapshot-stream); gewoon opnieuw draaien.
