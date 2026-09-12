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
- Beide remote pakketten zijn vanuit een lege map vanaf GitHub opgehaald en
  gevalideerd met fictieve apparaatinstellingen; geen lokale secrets gebruikt.
- Op beide schermen met hun bestaande indeling: tien paginacontroles,
  vijftig overlaycycli en de slidertest geslaagd.
- De renderdiagnose controleert bij gekleurde tegels ook de werkelijk
  toegepaste LVGL-achtergrond en donkere titelkleur.

De README en Easy Setup beschrijven nu de één-app-route met de ingebouwde
ESPHome-CLI. De oude handmatige CYD-route blijft ingeklapt beschikbaar en wordt
expliciet onderscheiden van de runtime-indeling met twintig tegels.

## Kleurcontrole op de apparaten

Via de echte HA-Ingress-editor is de eerste Guition-tegel tijdelijk rood en de
eerste CYD-tegel groen gemaakt. Op beide apparaten slaagt daarna opnieuw de
renderdiagnose inclusief de controle van de echte achtergrond en titelkleur.
Een interne Guition-schermopname toont de rode kaart met donkere tekst; dit is
een framebuffercontrole, geen fysieke foto van paneel of touch.

Bij visuele controle bleek een algemene fieldsetstijl de kleurkiezer in een
lange kolom te zetten. App 0.2.11 maakt het palet een compact responsief raster.
De bijbehorende actuele firmware blijft 0.2.10; voor deze CSS-correctie is geen
nieuwe schermflash nodig.

## Productie-update en herstel testdata

De HA-app is met back-up bijgewerkt naar 0.2.10 en vervolgens naar de compacte
editor 0.2.11. In de productie-Ingress-pagina is het raster visueel gecontroleerd.
Rood en groen bleven na die appupdate geselecteerd; de bestaande tien Guition-
en vier CYD-tegels bleven behouden. Via dezelfde editor zijn beide tijdelijke
testkleuren daarna expliciet teruggezet op Standaard. De kleurkeuze blijft dus
aan de gebruiker. Actuele firmware op beide apparaten: 0.2.10; app: 0.2.11.
