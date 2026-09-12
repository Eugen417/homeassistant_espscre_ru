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
- Alle acht C++-regressieprogramma's geslaagd. Firmwarebronnen zijn niet gewijzigd;
  daarvoor zijn de geslaagde 0.2.3-builds nog van toepassing.
- Toevoegen getest na verwijderen van positie tien in de testkopie: vrije positie
  zichtbaar, zoeken op entity-ID, toevoegen en automatisch openen van instellingen.
- Bestaande HA-add-on via Supervisor bijgewerkt van 0.2.1 naar 0.2.4 met back-up.
  Inventaris voor/na vergeleken: alle schermindelingen en instellingen exact gelijk;
  Home Assistant verbonden. Geen echte apparaat-acties uitgevoerd.
- Productie-Ingress in de ingelogde HA-browser herladen: nieuw schermvoorbeeld
  met pagina 1/2 zichtbaar; woonkamertegel opent de juiste mini-slider-instelling.
