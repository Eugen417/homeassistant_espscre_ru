# Problemen oplossen

| Symptoom | Eerst controleren |
|---|---|
| Geen USB-poort | Datakabel, andere USB-poort, bordvoeding; vergelijk `python -m serial.tools.list_ports` voor/na aansluiten. Installeer zo nodig de driver van de USB-chipfabrikant. |
| Permission denied op Linux | Toegang tot de seriële devicegroep (vaak `dialout`); opnieuw inloggen na een groepswijziging. |
| Poort bezet | Stop ESPHome-logs, IDE-monitor en andere seriële lezers. Eén lezer tegelijk. |
| Upload blijft verbinden | Controleer juiste bord/poort. Houd BOOT ingedrukt tijdens verbinden en laat daarna los als het bord dat vereist. |
| Build faalt | Python 3.11–3.14 en ESPHome 2026.6.2; volledige uitgepakte map, lokale fonts/components aanwezig, internet en schijfruimte. Bewaar eerste echte fout uit de buildlog. |
| Wit/zwart of verkeerd gedraaid beeld | Controleer ILI9341 + XPT2046-variant. Niet willekeurig de pinnen, rotatie of SPI-instellingen van een ander CYD-model kopiëren. |
| Geen meetscherm | De upload moet klaar zijn, niet alleen de build. Flash met `-s CALIBRATION_ON_BOOT true`. Zoek in USB-logs `USB calibration ready`; normaal opstarten toont geen kruisjes. |
| Geen metingen | Vijf kruisjes zichtbaar, andere loglezer dicht, juiste poort. Druk eerst Enter in de wizard en tik daarna drie keer hetzelfde gevraagde punt, met loslaten ertussen. |
| Tik springt naar andere plek | Controleer onafhankelijke `verify`, eigen `calibration.yaml` en vaste oriëntatie. Niet dezelfde tikken gebruiken om zowel te fitten als te verifiëren. |
| Fit meldt spreiding/fout | Meet opnieuw volgens de prompts. Instabiele ADC-metingen niet oplossen door de tolerantie te verhogen; controleer voeding, kabel en touchhardware. |
| Dubbele HA-actie | Controleer zowel touchlogs als HA-automations. Gebruik niet tegelijk directe acties én een automation op de actie-sensor voor dezelfde handeling. |
| Wifi verbindt niet | 2,4GHz, juiste lokale secrets, netwerkbereik. USB-kalibratie heeft HA niet nodig. |
| HA ziet het bord niet | Handmatig IP uit logs gebruiken; poort 6053 bereikbaar, juiste encryptiesleutel, geen gastnetwerkisolatie. |
| Status werkt maar actie niet | HA-optie voor toegestane apparaat-acties, `DIRECT_ACTIONS: "true"`, echte entity-ID en ondersteunde actie controleren. |
| Vacuum/climate deels bruikbaar | Ondersteunde modi/attributen verschillen per integratie; zie TEGELS.md en test dezelfde actie eerst in HA. |
| Verkeerde paginering | `TILE_COUNT` is 1–10; tot zes verborgen knoppen, vanaf zeven twee pagina's. Opnieuw flashen na wijzigen. |
| Vastloper/reset | USB-log bewaren, resetreden/voeding controleren en renderproef draaien. Noteer handeling en tijdstip. |

Voor een USB-log (stop andere lezers):

```sh
python diagnostics/capture_serial.py --port <USB_POORT> --seconds 120
```

De bestaande `CYD_STABILITY.md` en `TEST_RESULTS.md` beschrijven eerder
onderzoek aan één specifiek bord. Hun meetwaarden zijn geen kalibratie voor
een nieuw bord. Begin bij [README.md](../README.md), behoud een werkende
lokale configuratie en verander één oorzaak tegelijk.
