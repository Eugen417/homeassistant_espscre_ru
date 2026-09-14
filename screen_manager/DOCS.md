# Installatie en dagelijks gebruik

Zie [de volledige 0.2.0-handleiding](https://github.com/MaxGramser/homeassistant_espscreen/blob/main/docs/WHATS_NEW_020.md).

Open **Nieuw scherm** om een scherm te installeren: sluit het via USB aan op de
Home Assistant-machine, kies bord, naam en USB-poort en klik **Installeren**.
Het profiel met unieke sleutels komt in de ESPHome-map van de HA-configuratie,
ontbrekende wifi gaat in `secrets.yaml` (bestaande secrets blijven staan), en de
build en de flash lopen in hetzelfde venster; daarna toont het de API-sleutel
voor de koppeling. De koppeling zelf doe je in Home Assistant onder **Instellingen → Apparaten & diensten** (knop in het venster en op de kaart *nog niet in Home Assistant* onder Mijn schermen). Elk scherm heeft zijn eigen profiel. **Firmware & USB** is
voor bestaande profielen: controleren, bouwen of opnieuw installeren via USB of
het IP-adres (OTA). Bestaande ESPHome-profielen in de HA-configmap worden
automatisch gevonden.

Na koppeling via de HA ESPHome-integratie kies je in ESP Screens de tegels.
Onder **Tegelinstellingen** staan klikgedrag, grotere waarden en mini-schuiven.
Onder **Scherminstellingen** staan helderheid en standby. **Inspector** helpt bij
ontbrekende attributen of een offline scherm. Nieuwe kaarten vereisen firmware 0.2.0.
