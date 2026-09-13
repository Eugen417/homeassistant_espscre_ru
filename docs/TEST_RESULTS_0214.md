# App 0.2.12 / firmware 0.2.14 — speciale kaarten en dubbelbrede tegels

## Wijzigingen

- **Dubbelbreed** (`options.size: wide`) voor elke tegel. `runtime_model.h`
  bevat het pak-algoritme (`pack`): een dubbelbrede tegel begint links en telt
  voor twee vakjes; een lege rechterplek ervoor blijft leeg. De app gebruikt
  dezelfde regels voor het schermvoorbeeld en de paginagrenzen.
- **Klok** (`screen.clock`, ingebouwd, geen HA-entity): digitaal (grote tijd +
  datum) of analoog (wijzerplaat met stippen, uur- en minuutwijzer; dubbelbreed
  met de digitale tijd ernaast). Eigen cijferfont (60 px Guition, 28 px CYD).
- **Weersvoorspelling** (`display: forecast`, altijd dubbelbreed): actueel
  icoon, temperatuur en omschrijving links; vijf dagen met dagnaam, icoon en
  hoog/laag rechts. De manager haalt `weather.get_forecasts` (dagelijks) op en
  cachet dat 30 minuten. De gewone weerkaart toont nu ook het conditie-icoon.
- **Grafiek** (`display: graph`, sensoren): sparkline van de bestaande
  24-punts historie; enkelbreed in de sliderstrook, dubbelbreed rechts naast
  de tekst.
- **Zon** (`sun.sun`): opkomst en ondergang in de HA-tijdzone; **kookwekker**
  (`timer.*`): live aftellen op het scherm, tikken start/pauzeert, lang
  indrukken opent een kaart met annuleren; **aanwezigheid** (`person.*`):
  Thuis/Weg met groen accent.
- Statusberichten krijgen een optioneel object `x` met berekende waarden.
  Protocol, opslagversie en preferences ongewijzigd; oudere firmware krijgt
  indelingen met nieuwe domeinen pas na een update (`min_firmware`).

## Tests

- 55 Python-tests geslaagd (nieuw: opties/domeinen, `extras` met tijdzone,
  voorspellingstrimmen, ingebouwde klokstatus, firmwareguard in de manager).
  Alle negen C++-testprogramma's geslaagd; `test_runtime_model.cpp` test de
  nieuwe domeinen en het pak-algoritme inclusief paginagrenzen.
- Beide Easy Setup-profielen compileren met ESPHome 2026.6.2 (CYD-flash 72,7 %).
  Voor `lv_line` is een verborgen `line`-widget aan beide profielen toegevoegd,
  omdat ESPHome LVGL-widgets alleen inschakelt die in de YAML voorkomen.
- USB-flash op beide vooraf geïdentificeerde borden; `device_info` meldt 0.2.14
  met de verwachte compileertijden (Guition 02:00:44, CYD 02:01:01).
- Nieuw: `diagnostics/send_layout.py` zet een demo-indeling met alle kaarttypen
  (analoge klok dubbelbreed, voorspelling, grafiek enkel- en dubbelbreed,
  persoon, kookwekker, zon, grote waarde dubbelbreed, gewone weerkaart; drie
  pagina's) rechtstreeks in de inbox. Daarna de renderdiagnose: op **beide**
  apparaten tien paginacontroles en vijftig overlaycycli geslaagd met
  `count=9`, inclusief de nieuwe controles op onderdelen binnen de kaart,
  dubbele breedte en de grafiek naast/onder de tekst. Guition-heap ~6,46 MB;
  CYD 43,8–53,9 kB vrij (klok- en voorspellingsonderdelen kosten RAM; de
  gewone indeling houdt ~52 kB).
- Guition LVGL-snapshots (lokaal in `diagnostics/guition-0214-demo-p*.png`)
  gecontroleerd: klok met wijzers, tijd en datum; vijf voorspellingskolommen
  met iconen en hoog/laag; sparkline; timer die aftelt; zon; persoon.
- Daarna op beide apparaten de renderdiagnose op de echte indeling van de
  eigenaar geslaagd (Guition `count=10`, CYD `count=6`).
- Gevonden en verholpen tijdens de proef: de datumregel onder het 60px-klokfont
  viel buiten de kaart (nu 48/24 px met fallback zonder datum); het
  sparkline-object begon op de kaartoorsprong (lijn staat nu op eigen positie);
  de breedte-optie komt met het statusbericht na de indeling, dus de pagina's
  worden opnieuw gepakt zodra `size` wijzigt; de zelftest in de YAML rekende
  nog met zes tegels per pagina.

De productie-add-on op HA is de GitHub-versie; na de push verschijnt app
0.2.12 in de winkel. De demo-indeling wordt door de lopende manager binnen
~25 s weer vervangen door de echte indeling. Er is geen langdurige
stabiliteitsproef uitgevoerd; de fysieke bevestiging van de nieuwe kaarten in
de eigen HA-indeling volgt na de add-on-update.
