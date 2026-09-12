# Controle van versie 0.2.1 — 12 september 2026

- 42 Python-tests en acht C++-testprogramma's geslaagd. Inclusief het toestaan
  van een vastgehouden schuifgebaar, met behoud van ruis-, bounce- en dubbele
  actiebeveiliging.
- Easy Setup en handmatige profielen voor zowel CYD als Guition gecompileerd met
  ESPHome 2026.6.2. Gegenereerde pakketten gecontroleerd.
- Remote Guition-YAML vanuit een nieuwe map via GitHub gevalideerd.
- Guition via OTA bijgewerkt; tien paginacontroles en vijftig overlaywissels op
  het apparaat geslaagd. Interne LVGL-schermopnamen gecontroleerd voor de
  vacuumkaart, de mini-slider zonder handvat en de historie-as.
- Bestaande HA-add-on van 0.2.0 naar 0.2.1 bijgewerkt. Alle tien bestaande tegels,
  tegelopties en scherminstellingen vergeleken vóór/na: ongewijzigd.
- In de echte HA-browser de mini-slider uitgezet, opgeslagen, via de
  tegel-inspector teruggelezen en weer aangezet.
- Algemene instellingen geopend via de navigatie bovenaan. Tijdelijk 95%
  helderheid opgeslagen; de native HA-entiteit van het scherm rapporteerde 95%.
  Daarna de oorspronkelijke 100% hersteld.
- Ingebouwde ESPHome CLI via de echte HA-pagina: profielvalidatie, volledige
  Raspberry-build en OTA-installatie geslaagd. Eerste build plus upload duurde
  circa 16 minuten; upload zelf circa 9 seconden. Na herstart meldt HA firmware
  0.2.1 en is het scherm online. Alle tegelkeuzes en scherminstellingen behouden
  (de UI-test maakte alleen de bestaande standaardweergave expliciet).
- Ook op de door HA gebouwde firmware zijn tien paginacontroles en vijftig
  overlaywissels geslaagd.

Interne schermopnamen bewijzen de LVGL-layout, niet de afwezigheid van fysieke
paneelstrepen. Deze controles vervangen geen fysieke acceptatie van aanraken,
langdurige standby en iedere aangesloten HA-apparaatfunctie. Een CYD was niet
fysiek aangesloten voor deze release; daarvan zijn builds en regressies getest.
