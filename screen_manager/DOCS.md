# Installatie en dagelijks gebruik

Zie [de volledige 0.2.0-handleiding](https://github.com/MaxGramser/homeassistant_espscreen/blob/main/docs/WHATS_NEW_020.md).

Open **Nieuw scherm** om een eigen profiel te bewaren. Open **Firmware & USB**
om een profiel te controleren, bouwen of installeren. Bestaande ESPHome-profielen
in de HA-configmap worden automatisch gevonden. Eerste flash: USB aan de Raspberry
en expliciet de bijbehorende poort kiezen. Later: hetzelfde profiel plus het
IP-adres van het scherm voor OTA. Bestaande secrets worden niet vervangen.

Na koppeling via de HA ESPHome-integratie kies je in ESP Screens de tegels.
Onder **Tegelinstellingen** staan klikgedrag, grotere waarden en mini-schuiven.
Onder **Scherminstellingen** staan helderheid en standby. **Inspector** helpt bij
ontbrekende attributen of een offline scherm. Nieuwe kaarten vereisen firmware 0.2.0.
