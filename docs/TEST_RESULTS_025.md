# Vacuumstatus 0.2.5

Een tegel openen gebruikt korte lokale feedback. Het statuslabel in de grote
vacuumkaart zag deze feedback ten onrechte als een echte opdracht en werd daarna
niet meer bijgewerkt. De kaart onderscheidt nu lokale feedback van een verstuurde
HA-actie. De kop en de statusbadge worden beide bijgewerkt: bij openen de actuele
status (bijvoorbeeld In dock), alleen na een opdracht de actiefeedback.

Gedeelde firmwarecorrectie voor Guition en CYD. Geen wijzigingen aan indelingen,
instellingen, opslagversie, voorkeurensleutels of paneelaansturing.

- 42 Python-tests en alle acht C++-regressieprogramma's geslaagd.
- Regressie voor lokale feedback versus echte acties en het verlopen van de
  wachttijd toegevoegd aan de bestaande action-feedback-test.
- Gegenereerde pakketten met `--check` en Git-diff gecontroleerd.
- Alle vier ESPHome-builds geslaagd: Easy Setup en handmatige profielen van
  Guition en CYD.
- Guition via OTA bijgewerkt; HA bevestigt firmware 0.2.5 en online-status.
- Vacuumkaart op de echte Guition via previewservice geopend, zonder HA-actie.
  Interne LVGL-opname toont **In dock** in zowel de kop als de robotbadge.
  Fysiek aantikken door de eigenaar is niet door deze preview vervangen.
- CYD niet aangesloten; alleen softwaretests en builds uitgevoerd.
