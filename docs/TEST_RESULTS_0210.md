# 0.2.10 — pastel tegelkleuren en actuele documentatie

De editor levert negen lichte kleuren plus Standaard. Dezelfde paletwaarden
worden in firmware gebruikt. De gekozen achtergrond blijft vast bij een
statuswisseling; iconen, status en actiefeedback blijven de entiteit volgen.
Titels zijn bij een eigen kleur donker op zowel Guition als CYD.

## Uitgevoerd

- 51 Python-tests geslaagd, inclusief overeenkomst tussen editor- en
  firmwarepalet, minimaal 7:1 titelcontrast en 4,5:1 statuscontrast, validatie
  van kleuren en behoud bij appherstart/opslaan vanuit een oude editor.
- Alle negen C++-testprogramma's geslaagd; JavaScript-syntax gecontroleerd.
- Beide runtimefirmwares gebouwd met ESPHome 2026.6.2 en via OTA op de vooraf
  geïdentificeerde Guition en CYD gezet. Eigen sleutels en kalibratie behouden.
- Ook beide handmatige firmwareprofielen bouwen succesvol.
- Op beide schermen met hun bestaande indeling: tien paginacontroles,
  vijftig overlaycycli en de slidertest geslaagd.
- De renderdiagnose controleert bij gekleurde tegels ook de werkelijk
  toegepaste LVGL-achtergrond en donkere titelkleur.

De README en Easy Setup beschrijven nu de één-app-route met de ingebouwde
ESPHome-CLI. De oude handmatige CYD-route blijft ingeklapt beschikbaar en wordt
expliciet onderscheiden van de runtime-indeling met twintig tegels.
