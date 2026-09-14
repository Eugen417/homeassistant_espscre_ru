# Testresultaten app 0.2.31 / firmware 0.2.26 (2026-09-14)

Vrij slepen in de editor met vaste plekken, lege plekken en lege pagina's;
firmware 0.2.26 tekent de tegels op die plekken.

## Wat veranderde

- `core.py`: `SLOTS_PER_PAGE`/`MAX_PAGES`/`MAX_SLOTS`, `pack_slots`, `has_gaps`;
  `validate_layout` geeft elke tegel een `slot` (oude volgorde-packing zonder plekken,
  anders validatie op bereik, even plek voor dubbelbreed en overlap, gesorteerd) en
  accepteert `pages` (1–8). `server.py`: `slots` en `pages` in het layoutbericht;
  `save()` pakt opnieuw na het terugzetten van opgeslagen opties en valideert nogmaals.
- `runtime_model.h`: `Model.slots`/`explicit_slots`/`pages`, `set_layout(..., positions,
  moved)`, `place()`; `runtime_tiles.h`: parser leest `slots`/`pages`, `page_count()` en
  `place_page()` gebruiken `place()`. Versie 0.2.26 in beide profielen en pakketten.
- `app.js`: grid-model (`arrange`/`commit`/`placeTile`, `nearestFree`, `firstFree`),
  hover-preview tijdens slepen (`setTarget` rendert het resultaat, `slotAt` kiest de
  cel op afstand), pointer capture vanaf pointerdown en op `documentElement` tijdens de
  sleep (de mockup wordt herrenderd), lege cellen als knoppen (`insertAt`), pagina's
  toevoegen/weghalen, pijltjestoetsen, hint voor firmware < 0.2.26. `style.css`:
  `user-select: none` op editor en kiezer, `.preview-cell`, `.placeholder`, `.new-page`.

## Geautomatiseerd

- Python: 91 tests OK (nieuw `tests/test_positions.py`: packing zonder plekken, expliciete
  plekken met gaten en sortering, geweigerde overlap/oneven/bereik/gemengd, `pages`,
  `slots`/`pages` op de draad, oude opslag en oude editor). `generate_packages.py --check` groen.
- C++ (clang++ -std=c++17 -Wall -Wextra -Werror -I.): alle `tests/*.cpp` PASS;
  `test_runtime_model` dekt validatie van posities, `moved` zonder statusverlies,
  even plek voor dubbelbreed, lege tussenpagina, `pages`, terugval zonder plekken.
- ESPHome compile (bench-pakketten met lokale componenten): `easy-guition-device.yaml`
  SUCCESS (RAM 31,9%, flash 18,3%), `easy-cyd-device.yaml` SUCCESS (RAM 30,9%, flash 76,1%).
- Lokale add-on (devserver met nep-HA, echte pointer-events in de browser): twee enkele
  tegels, tweede dubbelbreed → blijft op rij twee, gat op plek 2; entiteit uit de kiezer
  in het gat gesleept → plek 2, geen tekstselectie; hover-preview toont de nieuwe pagina
  met placeholder; enkele tegel op dubbelbrede gedropt → wissel; pagina toevoegen/weghalen;
  pijltjestoetsen 6 → 5 → 6 met focus; lege plek klikken + kiezer → tegel op die plek;
  opslaan verstuurt `slots [0,2,3,6]` en state-indexen in plekvolgorde.

## Nog niet geverifieerd

- Op echte hardware: de indeling met gaten en een lege pagina op CYD en Guition na de
  firmware-update (Max test via GitHub-release → add-on → firmware bouwen in ESP Screens).
- Touch-slepen (hold 260 ms) op een telefoon in de HA-app.
