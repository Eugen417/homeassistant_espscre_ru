# App 0.2.13 / firmware 0.2.15 — zonnebaan, gladde grafiek, editor-UX

## Wijzigingen

- **Grafiek**: Catmull-Rom-curve door dezelfde 24 historiepunten (vier
  tussenpunten per segment) met een zachte vulling eronder. De vulling wordt
  door een `LV_EVENT_DRAW_MAIN`-tekenevent van de kaartcontainer als
  driehoeken getekend; er is geen canvasbuffer nodig, dus ook de CYD kan het.
- **Zonnebaan** (`sun.sun`, `display: sunpath`, altijd dubbelbreed): horizon,
  boog van opkomst naar ondergang, de zon op de huidige positie met een gloed
  en een vulling van het afgelegde deel; ’s nachts loopt de zon onder de
  horizon in indigo. De positie wordt uit de HH:MM-tijden van de manager en
  de lokale tijd berekend en ververst elke minuut.
- **Navigatie**: één onderbalk over de volle breedte met een doorlopende
  bovenlijn; linkerhelft = vorige, rechterhelft = volgende, paginanummer in
  het midden. Een uitgeschakelde helft dimt zijn tekst.
- **Paginawissel**: `show_page` zet in hetzelfde frame de kaartkaders van de
  nieuwe pagina neer (juiste breedtes via `pack`, inhoud verborgen, lichte
  skeletonstijl) en een eenmalige LVGL-timer van 60 ms vult daarna de inhoud
  synchroon in. Een eerdere fade-variant voelde traag en is vervangen.
  Keepalives en herpakken op dezelfde pagina passen direct toe.
- **CYD-kleuren**: het Guition-schema is overgenomen — vlakke lichtgrijze
  achtergrond (0xE7E7E7), witte kaarten met 0xDDDDDD-rand, donkere tekst,
  lichtblauwe iconcirkels — inclusief alle bedieningsoverlays (75 kleurwaarden
  één-op-één gelijk aan de Guition) en `light_theme` voor de runtimekaarten.
- **Navigatie** zonder scheidingslijn: alleen de teksten "<  Vorige",
  "Volgende  >" en het paginanummer.
- **Editor**: pointer-gebaseerd slepen (muis en touch met kort vasthouden)
  om tegels in de mockup te ordenen en entiteiten uit de lijst op een plek
  te laten vallen, met auto-scroll bij de schermranden; sticky opslaan-balk;
  nieuwe kop met drie uitgelegde acties en een stappenuitleg; verbeterde
  firmwaredialoog. Zon-, weer- en kloktegels starten dubbelbreed in hun
  mooiste weergave.

## Tests

- 55 Python-tests en negen C++-testprogramma's geslaagd; `--check` van de
  pakketgenerator; beide Easy Setup-profielen gebouwd (CYD-flash 72,9 %).
- Editor in de browser met een gemockte inventory gecontroleerd: kop, stappen,
  sticky opslaan-balk, dubbelbrede mockup. Slepen getest met gesimuleerde
  pointer-events: tegel 4 → positie 1 en een entiteit uit de lijst → positie 2
  komen op de juiste plek; een sleep telt niet als klik.
- Beide borden via USB geflasht; `device_info` meldt 0.2.15. Guition-snapshots
  van de demo-indeling: zonnebaan met zon op de boog en tijden, gladde
  gevulde grafiek enkel- en dubbelbreed, nieuwe onderbalk. Demo-renderdiagnose
  op de Guition 10/10 geslaagd (`count=9`).
- De eigenaar bevestigde het uiterlijk op de Guition, meldde tussenframes bij
  paginawissels (foto met een achtergebleven sparkline) en vond de fade te
  traag; daarop de skeletonwissel. Renderdiagnose op de echte indelingen van
  beide borden na de laatste flash: zie hieronder.
- Laatste flash (Guition 08:59:18, CYD 09:03:32, beide 0.2.15): renderdiagnose
  op de echte indelingen geslaagd — Guition 10/10 (`count=6`, heap ~6,46 MB),
  CYD 10/10 (`count=7`, ~52,5 kB vrij). Beide handmatige profielen compileren.
  De skeletonwissel en de CYD-kleuren zijn door de eigenaar visueel te
  beoordelen; er is geen langdurige stabiliteitsproef uitgevoerd.

## App 0.2.14 (zelfde firmware)

Tegelinstellingen openen nu in een sheet boven de mockup (mobiel: onderaan)
met knoppen voor naam, weergave, breedte, tikgedrag, slider, geschiedenis en
kleur; elke keuze werkt live door in de mockup erachter. Het kruisje op een
kaart verwijdert direct, met **Ongedaan maken** in de melding; de tegellijst
onder de mockup is vervallen. In de browser met de gemockte inventory
gecontroleerd: sheet-layout, weergave "Grote waarde" direct zichtbaar in de
mockup, verwijderen en ongedaan maken herstellen de volgorde, het kruisje
start geen sleep. 55 Python-tests geslaagd; geen firmwarewijziging.
