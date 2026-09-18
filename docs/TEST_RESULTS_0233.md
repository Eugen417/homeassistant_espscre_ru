# Testresultaten app 0.2.33 / firmware 0.2.28 (2026-09-14)

Alleen firmware: op de Guition zou LVGL beslissen of een tik doorgaat (loslaten binnen de
tegel telt; zie de correctie hieronder), de verplaatsingsgrens van de tap-guard staat daar
uit. Max koos dit na een live geweigerde druk van 73 px op Studio 1.

## Wat veranderde

- `TouchGuard::configure(move_limit_px, ...)` met 0 betekent geen grens: `moved_` wordt nooit
  gezet, `accept()` en `accept_repeat()` kijken alleen nog naar duur, dubbel gebruik en
  dender. LVGL's press-lost (vinger verlaat het object) zou bepalen of de klik doorgaat.
- Guition-profiel `TOUCH_MOVE_LIMIT_PX: "0"`; CYD blijft 56 (resistief paneel springt).
- Correctie (2026-09-18): LVGL beslist dat niet. Een tegel houdt LVGL's `LV_OBJ_FLAG_PRESS_LOCK`
  (standaard aan), dus een vinger die van de tegel schuift stuurt nooit `LV_EVENT_PRESS_LOST`, en
  loslaten buiten de tegel telde op de Guition nog steeds als tik. Vanaf firmware 0.2.65 controleert
  de tegel dat zelf (`runtime_tiles::event`): een tik of vasthouden waarbij de vinger buiten de tegel
  loslaat, doet niets. PRESS_LOCK blijft aan.

## Geautomatiseerd

- C++ (clang++ -std=c++17 -Wall -Wextra -Werror -I.): 11/11 PASS; `test_cyd_ui` dekt
  `configure(0, 20)`: 200 px drift blijft een tik, ook voor de −/+ toetsen.
- Python: 91 tests OK (`FIRMWARE_VERSION` 0.2.28 in app, profielen en pakketten).
  `generate_packages.py --check` groen; bench-kopieën opnieuw gegenereerd.
- ESPHome 2026.6.2 compile: `easy-guition-device.yaml` SUCCESS in 40 s (RAM 32,0%, flash
  18,3%), `easy-cyd-device.yaml` SUCCESS in 36 s (RAM 31,0%, flash 76,1%); geen nieuwe
  waarschuwingen.

## Nog te controleren op Studio 1

- Een stevige, schuivende druk op een tegel voert de actie uit zolang je binnen de tegel
  loslaat; over de rand van de tegel loslaten doet niets (zo bedoeld; waar vanaf firmware 0.2.65,
  zie de correctie hierboven).
- De randveeg (0.2.27) na een update zonder eerst een kaart te sluiten, en de eerste veeg
  na een paginawissel; het log meldt `randveeg niet gevuurd: …` als hij te kort was.
