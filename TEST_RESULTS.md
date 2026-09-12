# Testresultaten ESP32-2432S028 — 12 september 2026

Getest op de via USB aangesloten `cyd-2432s028`, ESP32 rev3.1, ESPHome
2026.6.2 / ESP-IDF. De firmware meldt compilatietijd
`2026-09-12 09:54:02 +0200` en is via USB geschreven en geverifieerd.

## Geslaagd

- C++-regressietests voor ruis, contactdender, verspringende aanrakingen,
  echte sleepbewegingen, temperatuurafronding en `millis()`-overflow.
- Vijf indelings-/configuratietests voor schermgrenzen, overlap, modale
  lagen, voorwaardelijke paginering en guards vóór alle tegelhandlers.
- ESPHome-validatie met `TILE_COUNT: 6`; navigatie is in de code aan
  `count > 6` gekoppeld en tegels boven het ingestelde aantal zijn verborgen.
- Volledige ESP-IDF-build en USB-upload met verificatie van de geschreven data.
- Verbinding met de bestaande Home Assistant-installatie en ontvangst van
  entiteitsstatussen.
- **10 paginacontroles en 50 overlay-cycli op het echte apparaat.**
  Render-startteller steeg van 38 naar 101 tussen de eerste en
  laatste paginacontrole: de test controleert ook werkelijke tekenactiviteit.
- Runtime-geometrie: tegelgebied `(0,40)–(319,203)`, knop Volgende
  `(211,206)–(310,237)`. De gebieden overlappen niet.
- Vrije heap tijdens de tien testmetingen: 86,416–139,696 bytes
  volgens `esp_get_free_heap_size()`. Geen oplopend geheugenverlies of reset
  in deze korte tekenproef.

## Fysieke touchdiagnose

De gebruiker tikte herhaaldelijk de vier schermhoeken aan. De oorspronkelijke
meting sprong linksboven soms meer dan honderd pixels weg. Na de driverfilter
bleven de samples in herkenbare hoekgroepen. De overblijvende scheefstand is
met een affine correctie op de hoekmedianen gecorrigeerd; de regressietest
plaatst die medianen binnen vijf pixels van het doel. De bronwaarden en
coëfficiënten staan in de configuratie, tests en CYD_STABILITY.md.

De gebruiker bevestigde na de laatste flash dat de juiste knoppen reageren
en de bediening nu goed werkt.

## Grenzen

Dit is geen langdurige duurproef. Home Assistant-acties zijn niet automatisch
uitgevoerd tijdens de schermtest. De gebruiker heeft de fysieke bediening na de laatste kalibratiecorrectie
bevestigd: "Ja, dit werkt nu goed". De tien minuten
standby zijn ingesteld; er is niet tien minuten gewacht om de volledige
idle-timeout af te laten lopen.

Lokale ruwe logs staan in `diagnostics/acceptance-ui-test.log` en
`diagnostics/acceptance-runtime.log` en worden niet in Git opgenomen.

## Overdraagbare installatie — aanvullende softwarecontrole

Op 12 september 2026 is de nieuwe onboarding getest:

- 17 Python-tests slagen: layout, kalibratie-fit, onafhankelijke verificatie,
  foutieve/ruisende metingen, unieke secrets, niet overschrijven en export.
- Beide C++17-regressieprogramma's slagen (touchfilter en UI-guard).
- Starter-ZIP uitgepakt in een verse tijdelijke map; nieuw profiel met eigen
  gegenereerde sleutels aangemaakt. Volledige ESPHome 2026.6.2 / ESP-IDF-build
  met kalibratie-opstartmodus geslaagd.
- Gegenereerde C++ gecontroleerd op identiteitcorrectie voor het nieuwe bord.
  Het aparte lokale profiel behoudt de eerder gemeten zes correctiecoëfficiënten.
- Git-index en bestaande historie gecontroleerd op de lokale credentials;
  geen overeenkomsten. Lokale profielen en secrets zijn uitgesloten.

De vijfpuntswizard is met synthetische meetgegevens getest. Hij is in deze
ronde niet opnieuw fysiek uitgevoerd of naar het bestaande bord geflasht;
voor ieder nieuw paneel blijft de fysieke acceptatie verplicht. De bovenstaande
oudere hardwaretest betreft de eerder geïnstalleerde firmware.
