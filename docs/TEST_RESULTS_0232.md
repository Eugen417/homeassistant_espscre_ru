# Testresultaten app 0.2.32 / firmware 0.2.27 (2026-09-14)

Alleen firmware: de randveeg die op een echte Guition (Studio 1, firmware 0.2.24) nooit
pakte, plus de veeg op LVGL's eigen pointer-events. De app verandert alleen de doelversie.

## Live diagnose op Studio 1 (ESPHome-log via de native API)

- Elke veeg begon netjes in de band (`GT911 press x=0…13` links, `x=470…478` rechts), maar er
  volgde nooit `randveeg`. Instelling stond aan (add-on: `swipe_pages: true`, 10 tegels,
  rotatie 0), `ui_state` meldde niet gedimd, geen kalibratie, geen runtime-kaart.
- Na `preview_runtime_card -1` (roept `runtime_tiles::dismiss()` aan) werkte elke veeg
  meteen: `randveeg: pagina 0 -> 1`. Oorzaak: de kruisjes en achtergrondtikken van de
  licht-, klimaat- en stofzuigerkaart en het wakker worden uit standby verbergen de kaart
  zonder `active_entity` te wissen; de veeg-guard leest die en meldde stil "kaart open".
  Dezelfde gate zat in de oude LVGL-gesture en in het CYD-profiel.
- Daarna: ongeveer één op de twee eerste vegen na een paginawissel vuurde niet, de
  tweede wel (bijvoorbeeld 12:57:28 en :29 vanaf rechts zonder wissel, :29 wel). De
  0.2.24-firmware logde daarvoor niets; 0.2.27 meldt nu `randveeg niet gevuurd: N px naar
  binnen, M px verticaal` en de richtingseis is 45° in plaats van "twee keer zo horizontaal".
- Eén tik werd terecht geweigerd: `tik op script.studio_1_startup_sequence genegeerd:
  verplaatst 73 px (grens 67)`; de grens op de Guition is een open vraag aan Max.

## Wat veranderde

- `cyd::EdgeSwipe` op LVGL-pointercoördinaten (`begin(x, y, width)` met de horizontale
  resolutie van LVGL, dus rotatie is al verwerkt), 45°-regel, `inward()`/`sideways()`.
- Guition: één `lv_indev_add_event_cb` in `on_boot` (PRESSED → begin, PRESSING → update en
  paginawissel met `lv_indev_wait_release`, RELEASED/PRESS_LOST → logregel als de veeg
  niet vuurde); geblokkeerde vegen loggen de reden. De touchscreen-triggers voeden alleen
  nog de tap-guard.
- Beide borden: script `close_cards` (via `runtime_tiles::dismiss()`, anders overlays
  verbergen en `active_entity` wissen) voor de zes sluitknoppen en `wake_display`.

## Geautomatiseerd

- C++ (clang++ -std=c++17 -Wall -Wextra -Werror -I.): 11/11 PASS; `test_cyd_ui` dekt de
  nieuwe `EdgeSwipe`-API, de 45°-regel, de logwaarden en een smallere (gedraaide) breedte.
- Python: 91 tests OK (`FIRMWARE_VERSION` 0.2.27 in app, profielen en pakketten).
  `generate_packages.py --check` groen.
- ESPHome 2026.6.2 compile (verse worktree, geen cache): `easy-guition-device.yaml` SUCCESS in
  112 s (RAM 32,0%, flash 18,3%), `easy-cyd-device.yaml` SUCCESS in 86 s (RAM 31,0%, flash
  76,1%); geen nieuwe waarschuwingen.

## Nog te controleren op Studio 1

- Werkt de randveeg direct na een update zonder eerst een kaart te openen en te sluiten, en
  pakt de eerste veeg na een paginawissel nu ook; het log (`touch`) laat het zien.
- Na het openen en sluiten van een lichtkaart (kruisje, tik ernaast, standby) blijft vegen werken.
