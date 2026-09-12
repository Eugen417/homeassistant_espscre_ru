# Visuele tegelkiezer 0.2.4

De beheerpagina toont een schematisch schermvoorbeeld met twee kolommen en zes
tegels per pagina, maximaal tien tegels. Domeinen hebben herkenbare symbolen,
kleuren en tekstlabels in de zoekresultaten, filters en het schermvoorbeeld.
Klikken op een voorbeeldtegel opent alleen diens bestaande instellingen.
Mini-sliders en grote waarden zijn herkenbaar; het voorbeeld toont geen live
meetwaarden. Sleep tussen bestaande posities of gebruik de pijlen bij de
tegelinstellingen. Toevoegen vult de eerstvolgende vrije positie.

Opslag, tegelprotocol, firmware en secrets blijven ongewijzigd. Dit is uitsluitend
een add-onupdate; de firmware blijft 0.2.3.

Controles:
- 42 Python-regressies geslaagd; JavaScript-syntax en git diff gecontroleerd.
- Browserpreview met een kopie van de bestaande tien HA-tegels: beide pagina's,
  domeiniconen, klik naar de juiste lampinstellingen en mini-slider-keuze getest.
- Schermvoorbeeld en geselecteerde instellingen visueel gecontroleerd in de
  browser. De previewserver schrijft niet naar Home Assistant.
