## 0.2.2

- Firmware: terug naar pagina 1 bij standby sluit nu ook runtime-detailkaarten, waaronder vacuum, historie en media. Installeer hiervoor de nieuwe schermfirmware via Firmware & USB.

## 0.2.1

- Algemene instellingen en Inspector direct bovenaan bereikbaar.
- Tegel selecteren: klikactie, grote waarde, kleine slider (ja/nee) en historieperiode.
- Inspectie per tegel met actuele HA-status en eigenschappen.
- Firmware: schuifgebaren correct verwerken, brede mini-slider zonder handvat, concrete historie-as.
- Nieuwe vacuumkaart met robotweergave, actieve zuigkracht en druk-/opdrachtfeedback.

# 0.2.0

- ESPHome CLI in de app: bestaande profielen controleren, bouwen en via USB/OTA installeren.
- Nieuwe profielen met unieke sleutels; bestaande YAML, secrets en appdata blijven behouden.
- Tegelopties: klikgedrag, grote waarde, mini-schuif en sensorgeschiedenis.
- Media-, weather-, number- en select-kaarten en vernieuwde vacuumkaart op beide borden.
- Actiefeedback, vaste tekstregels en zwarte sliderknoppen.
- Inspector en helderheid/standby als instellingen bij het HA-apparaat.
- Native Guition ST7701S-configuratie en lichte kaartstijl. Duuracceptatie van het fysieke beeld blijft apart van firmwaretests.

# 0.1.2

- Per scherm standbyduur, normale/standby/nachthelderheid en nachturen instellen.
- Klok aan/uit, 12/24 uur en terug naar pagina 1 na standby.
- Instellingen direct doorsturen; firmware 0.1.2 bewaart ze in preferences.
- Bestaande tegels en sleutels blijven behouden. Eenmalig firmware bijwerken.
- De Guition-displaydriver is ongewijzigd; dit is geen oplossing voor paneelstrepen.

# 0.1.1

- Duidelijke foutmelding bij een ongeldige naam, te veel tegels of dubbele entiteiten.
- Updatepad met behoud van bestaande schermindelingen.

# 0.1.0

- Eerste ESP Screen Manager met Home Assistant Ingress.
- Zoek op entiteit, apparaat of ruimte; tien geordende tegels per scherm.
- Tegelwijzigingen en actuele waarden zonder firmwareflash.
- Installatie-YAML met unieke sleutels voor CYD en Guition.
- Permanente indelingen, automatisch herstel na HA-/schermherstart.
