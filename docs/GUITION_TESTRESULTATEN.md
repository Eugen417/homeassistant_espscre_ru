# Guition hardwaretest — 12 september 2026

Getest op de aangesloten ESP32-S3 rev. 0.2, 16 MB flash en 8 MB PSRAM,
GT911 op I²C-adres 0x5D, GUITION-4848S040 RGB-display op 480×480.

## Werkende versie

ESPHome 2026.6.2 / ESP-IDF 5.5.4. De firmware rapporteert buildtijd
`2026-09-12 10:45:49 +0200`, config-hash `0xbaee760b`.

- Beide GT911-assen gespiegeld. De gebruiker zag eerst linksboven als
  rechtsonder; na correctie kwamen de vijf aangeraakte regio's in de juiste
  volgorde binnen: linksboven, rechtsboven, rechtsonder, linksonder, midden.
- Pixelklok 16 MHz. De gebruiker bevestigde dat het zachte flikkeren daarmee
  verdween, maar meldde nog af en toe een glitch.
- Vervolgens PSRAM-uitvoering voor code/constanten, 64KB datacache met
  64-byte cachelijnen en RGB-herstel op VSYNC ingeschakeld. Na deze update en
  de herhaalde renderproef bevestigde de gebruiker: **“het werkt super!”**

## Uitgevoerde controles

- 23 Python-regressietests geslaagd, inclusief nieuw bordprofiel, tegelformaat,
  navigatieruimte, GT911-coördinaten/rotatie en overdracht zonder lokale secrets.
- Beide C++17-testprogramma's geslaagd: eventguard/temperatuur en CYD-touchfilter.
- Volledige S3-build geslaagd. Eerste USB-upload op 115200 baud met verificatie
  van de geschreven hash; vervolguploads via geauthenticeerde OTA geslaagd.
- De originele gebruikte applicatie, bootloader, partitietabel en NVS zijn
  als lokale herstelkopie bewaard; zie GUITION.md voor lengte en SHA-256.
- Bootlogs bevestigen 8192KB beschikbare PSRAM, de GT911 en RGB-driver.
- **Tien paginacontroles en vijftig overlaycycli op de definitieve firmware**
  geslaagd, inclusief controle dat zichtbare klikgebieden binnen 480×480 liggen.
- Tegelgebied eindigt op y=419; de volgende-knop begint op y=432. Geen overlap.
- Vrije heap bleef tijdens de definitieve renderproef rond **6,51 MB**.
- Een echte LVGL-snapshot via de versleutelde API opgehaald en visueel
  gecontroleerd op tekst, kleuren en indeling. Deze screenshotfunctie bekijkt
  de renderer; de fysieke signaalcontrole komt van de gebruiker.
- Een verse export uitgepakt en een neutraal Guition-profiel aangemaakt;
  ESPHome-codegeneratie geslaagd zonder de persoonlijke tegelconfiguratie.

## Grenzen van deze test

De fysieke check gebruikte één tik per doel plus gebruik van de bediening;
het volledige drie-tikken-per-punt-protocol uit `verify_gt911.py` is niet op
het bord uitgevoerd. Stabiliteit is tijdens deze testsessie geobserveerd,
niet gedurende dagen. De tien-minutenstandby is behouden in de configuratie;
een nieuwe volledige standbyduurtest is niet uitgevoerd.

Home Assistant ontdekte het nieuwe apparaat. De koppeldialoog wacht nog op de
lokale API-encryptiesleutel. Werkelijke HA-acties en de terugmelding van eigen
entiteiten zijn op dit nieuwe apparaat daarom **nog niet geverifieerd**.
