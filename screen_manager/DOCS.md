# Aan de slag

Start de app en open de webinterface. Klik **Nieuw scherm**, kies het exacte bord
en download je eigen YAML. De wizard legt de eerste USB-flash via ESPHome Device
Builder, wifi, HA-koppeling en kalibratie uit. Kies daarna je tegels in deze app.

De volledige handleiding staat op
[GitHub: Easy Setup](https://github.com/MaxGramser/homeassistant_espscreen/blob/main/docs/EASY_SETUP.md).

## Updates

Update de app via de appwinkel. Indelingen staan in de permanente `/data`-map en
blijven bij een update behouden. Verwijder de app niet om een update te installeren.
Voor nieuwe schermfuncties kies je in ESPHome bij je **bestaande apparaat**:
Install → Wirelessly. Houd je eigen YAML, apparaatnaam en sleutels gelijk.

Maak vóór updates een HA-back-up inclusief deze app en ESPHome Device Builder.

## Werking

De app leest entiteiten, apparaten, ruimtes en status uit jouw HA en schrijft
uitsluitend configuratieberichten naar de ESPHome-tekstentiteit Tegelinstellingen.
Het scherm voert de gekozen HA-acties uit. Geef het daarvoor toestemming in de
ESPHome-integratie. De app moet draaien om actuele gegevens te blijven versturen.
