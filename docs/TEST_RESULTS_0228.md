# Testresultaten app 0.2.28 / firmware 0.2.23 (2026-09-14)

Alleen firmware: tikken die verloren gingen doordat de vinger tijdens het drukken
iets verschoof. De app verandert alleen de doelversie.

## Diagnose vooraf (waarom tikken wegvielen)

- `cyd::TouchGuard::update()` markeerde een aanraking als "verplaatst" zodra x of y
  meer dan 18 px van het allereerste contactpunt afweek (Guition ≈ 2,7 mm, CYD ≈ 3,2 mm);
  `accept()` weigerde daarna stil, zonder logregel. LVGL 9.5 zelf annuleert een klik
  alleen bij scrollen (alle containers staan op `scrollable: false`) of bij een ander
  object onder de vinger (press lost); een gesture (50 px, snelheid 3 px/meting)
  onderdrukt geen klik. De blokkade zat dus in de eigen guard, ver onder de veegdrempel
  en ook met "Vegen tussen pagina's" uit.
- Bijzaken: elk contactpunt (ook een tweede vinger) voedde de guard; de 60 ms
  minimumduur voor contactdender gold ook op de capacitieve Guition.

## Wat veranderde

- Per bord `TOUCH_MOVE_LIMIT_PX` (Guition 67, CYD 56: circa 1 cm) en
  `TOUCH_MIN_PRESS_MS` (CYD 60, Guition 20), via `configure()` in `on_boot`.
- Verplaatsing is nu de afstand tot een referentiepunt dat over de eerste vier
  metingen (≈ 60 ms bij 20 ms polling) settelt; een snelle veeg wordt daardoor pas
  op de vijfde meting herkend, een langzame drift onder 1 cm blijft een tik.
- Alleen het contact-id waarmee de aanraking begon telt (`touch.id`).
- `reason()` en `runtime_tiles::allowed()`: elke geweigerde tik op tegel, kaartknop of
  bediening staat op INFO in het log met tag `touch`.

## Geautomatiseerd

- C++ (clang++ -std=c++17 -Wall -Wextra -Werror): 11/11 PASS. `test_cyd_ui` dekt de
  nieuwe grenzen: 55 px drift geaccepteerd, veeg van 180 px geweigerd met reden
  `verplaatst`, tweede vinger genegeerd, 10 ms te kort op de Guition, `al verwerkt`,
  dendertijd, CYD-sprong van 30 px geaccepteerd en 70 px geweigerd; de oude
  standaardwaarden (18 px, 60 ms) gelden nog zonder `configure()`.
- Python: 84 tests OK (`FIRMWARE_VERSION` 0.2.23 gelijk aan beide bordprofielen en
  pakketten). `tools/generate_packages.py --check` groen; de bench-kopieën in
  `.esphome/easy-*-package.yaml` opnieuw gegenereerd.
- ESPHome 2026.6.2 compile: `easy-guition-device.yaml` SUCCESS in 43 s (RAM 31,9%,
  flash 18,3%), `easy-cyd-device.yaml` SUCCESS in 41 s (RAM 30,9%, flash 76,0%);
  geen nieuwe waarschuwingen in `cyd_ui.h` of `runtime_tiles.h`.

## Nog niet geverifieerd

- Fysiek tikken met een bewegende vinger op beide borden; de bench-borden hingen
  tijdens deze ronde niet aan USB en reageerden niet op de API, dus geen flash en
  geen logcontrole van de nieuwe `touch`-regels. Max werkt de schermen bij via
  **Bijwerken** en kijkt in het ESPHome-log of geweigerde tikken nu een reden geven.
