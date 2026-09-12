# Lichtregelaars — 12 september 2026

Gedeelde `light_controls.h` vervangt de kleurpresets in de CYD- en
Guition-profielen. Hue, wittemperatuur en helderheid staan op één kaart;
capabilities bepalen welke schuiven verschijnen.

## Uitgevoerd

- 23 Python-regressietests op de Guition-branch geslaagd.
- C++-tests voor touchguard, touchfilter en nieuwe lichtwaardenparser geslaagd.
- Beide persoonlijke profielen volledig gebouwd met ESPHome 2026.6.2.
- Guition: RAM statisch 74.380 bytes; firmware 1.349.559 bytes.
- CYD: RAM statisch 70.820 bytes; firmware 1.291.519 bytes (70,4% app-partitie).
- Guition via OTA geflasht en apparaatidentiteit via versleutelde API bevestigd.
- Op Guition `Light sliders: PASS`: twintig preview-events per regelaar,
  één commit bij loslaten, geen dubbele commit, geen commit na press-lost,
  vier capabilitycombinaties. Callbacks vervangen tijdens deze test: geen lampacties.
- Tien pagina-/geometriecontroles en vijftig overlay-rendercycli geslaagd.
  Frames 6–69; vrije heap circa 6.514.320 bytes.
- Veilige voorbeeldkaart uit LVGL als screenshot opgehaald en visueel gecontroleerd.
  Eerste screenshotpoging verloor de API-verbinding; herhaling met USB-logging
  voltooide alle 240 rijen zonder exception/backtrace in de capture.
- Neutrale starterbundel met gedeelde header succesvol geëxporteerd.

De CYD-build vond een lokale PlatformIO-esptool-koppeling naar versie 4.9,
terwijl de platformmetadata 5.3 vereiste. De lokale editable installatie is
hersteld naar de al aanwezige 5.3-package; de volledige build slaagt daarna.
Dit is een lokale toolchainreparatie, geen wijziging van bord/flashinstellingen.

## Grenzen

Een succesvolle build is geen fysieke CYD-test: alleen de Guition was aangesloten.
Schuifevents in de firmwaretest zijn synthetisch; menselijke touchbediening en
werkelijke HA-lampacties moeten apart worden gecontroleerd. De eigenaar vond de
voorbeeldkaart er goed uitzien; de HA-koppeling ontbrak op dat moment nog.
