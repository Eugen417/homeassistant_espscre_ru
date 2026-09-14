# Testresultaten app 0.2.38 / firmware 0.2.32 (2026-09-14)

Nieuw: de **bovenbalk** per scherm. Links de naam, rechts tot zes onderdelen: tijd,
analoge klok, datum of een entiteit met icoon (status of "laatst gewijzigd", altijd of
alleen als actief). Zie README "Bovenbalk" en RELEASING "Compatibiliteit 0.2.38".

## Wat veranderde

- App: `core.validate_header` (opslag), `header_bar.py` (Nederlandse opmaak met
  weergaveprecisie uit het entity-register, iconen per device class en stand,
  HA-kleuren, "alleen als actief", suggesties per ruimte), `op: header` direct na de
  indeling voor firmware 0.2.32+, `POST /api/header-preview` voor de editor,
  bovenbalk-entiteiten tellen mee voor statuswijzigingen.
- Editor: blok **Bovenbalk** (naam, kaartjes, slepen, sheet voor toevoegen en
  instellen), de balk op elke pagina van het schermvoorbeeld als SVG met Roboto
  (`bar-roboto-400/500.woff`, gegenereerd door `tools/generate_icons.py`) en de
  plaatsingsregels van de firmware; knoppenbalk en **Klok tonen** weg.
- Firmware: `header_bar.h` (parsen, relatieve tijd, datum, afstanden, plaatsing) en
  `runtime_tiles::draw_header()`; beide profielen zetten de fonts en het tijdlabel als
  referentie; `sublabel_big` kreeg extra glyphs.

## Geautomatiseerd

- Python (`.venv-portal`): 121 tests OK, waarvan 18 nieuw in `test_header_bar.py`
  (validatie, getallen en eenheden, waarden per domein, actief/kleur/icoon, glyphs gelijk
  aan beide profielen en pakketten, woorden en rekensommen gelijk in editor en firmware,
  profielkoppeling, manager: alleen 0.2.32+ krijgt de balk, geen herhaling bij gelijke
  balk, oude editor behoudt de balk, verdwenen entiteit geweigerd, preview-endpoint).
- C++ (clang++ -std=c++17 -Wall -Wextra -Werror -I.): 13/13 PASS; nieuw
  `test_header_bar` (soorten, kleuren, UTF-8, alle drempels van "geleden"/"over",
  datum, afstanden voor Guition en CYD, plaatsing met overloop en lege balk).
- `generate_packages.py --check` en `generate_icons.py --check` groen; bench-kopieën
  `.esphome/easy-*-package.yaml` opnieuw gegenereerd; `node --check app.js` OK.
- ESPHome 2026.6.2 compile: `easy-guition-device.yaml` SUCCESS (RAM 32,4%, flash
  18,6%; was 32,3% / 18,5%), `easy-cyd-device.yaml` SUCCESS (RAM 31,4%, flash 77,3%;
  was 31,3% / 76,8%). Eén bestaande waarschuwing (`runtime_tiles.h:580`,
  enum-conversie), niet uit deze wijziging. De eerste Guition-build faalde op
  `std::max(int, long)` (int32_t is long op ESP32); opgelost met een cast.
- **Echte LVGL-render zonder scherm:** `tools/render_topbar.py` bouwt een ESPHome
  host-project met SDL2 en de fonts uit beide profielen en tekent zeven scenario's via
  `draw_header()` (alleen klok, de mockup met "—" en "Gisteren", deur open + alarm,
  analoge klok + datum, aantal thuis + weer + verbruik, te vol met afgekorte naam,
  personen). Bekeken op 2x/3x: waarden op de basislijn van de naam, iconen op
  cijferhoogte, gelijke afstanden, overloop valt vooraan weg.
- Editor in de browser (dev-server met een alleen-lezen momentopname van de echte HA):
  toevoegen via suggesties en zoeken, instellen (status/laatst gewijzigd, icoon, tonen),
  slepen, overloop- en verborgen markering, opslaan stuurt `layout`, `header`, states
  naar een 0.2.32-scherm en geen `header` naar 0.2.31; CYD-maten en de firmwarehint op
  een CYD; telefoonbreedte zonder horizontaal scrollen.

## Niet getest

- Geen scherm aan USB tijdens deze ronde: de bovenbalk is niet op glas gezien. De render
  gebruikt dezelfde LVGL 9.5-code en fonts, maar niet het paneel (kleuren, backlight).
- Studio 1 draait nog firmware 0.2.31 en app 0.2.37 tot de update.

## Te controleren op Studio 1 na de update

- Add-on bijwerken naar 0.2.38, dan **Bijwerken** bij Studio 1 (firmware 0.2.32).
- Zonder bovenbalk-aanpassing: naam links, tijd rechts in de nieuwe waardeletter.
- Voeg temperatuur, een persoon met "Laatst gewijzigd", een deur of alarm (alleen als
  actief) en de analoge klok toe; kijk of alles op één lijn staat en even ver uit elkaar.
- "5 min geleden" telt per minuut door; een deur die opengaat verschijnt binnen enkele
  seconden in amber; een lange naam krijgt puntjes zoals in de editor.
- HA even loskoppelen: de entiteiten verdwijnen uit de balk, tijd en datum blijven.
