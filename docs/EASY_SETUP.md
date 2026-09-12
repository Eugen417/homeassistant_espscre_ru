# Installeren vanuit ESP Screens (vanaf 0.2.0)

De makkelijkste route is nu één app: **ESP Screen Manager**. Deze kan zelf
ESPHome-profielen bewaren, bouwen en via USB of OTA installeren. Zie de
[complete stappen voor deze route](WHATS_NEW_020.md#firmware-zonder-een-tweede-beheerpagina).
De instructies hieronder blijven bruikbaar als je ESPHome Device Builder wilt
blijven gebruiken. Beide routes behouden dezelfde eigen YAML en secrets.

# Een nieuw scherm, van USB naar dagelijks gebruik

Met **ESP Screen Manager** kies je je tegels in Home Assistant. Je zoekt op
naam, apparaat, ruimte of entity-ID, zet ze in de gewenste volgorde en klikt
op **Opslaan & naar scherm**. Daarna blijft de status automatisch actueel.
Je hoeft voor andere tegels **niet opnieuw te flashen**.

Dit werkt met Home Assistant OS op een 64-bits Raspberry Pi of een amd64-machine,
ESPHome Device Builder en een van deze exacte schermvarianten:

| Keuze | Hardware |
| --- | --- |
| CYD | ESP32-2432S028, 320×240, ILI9341 en XPT2046 |
| Guition | ESP32-S3-4848S040, 480×480, ST7701S en GT911 |

Andere schermen met ongeveer dezelfde naam kunnen andere pinnen hebben. Gebruik
het bordprofiel dat bij de hardware hoort. Gebruik een USB-kabel die data ondersteunt.
De wallbox-relais worden niet gebruikt.

## 1. Installeer de twee apps

1. Open **Instellingen → Apps → Installeer een app** (in oudere HA-versies:
   **Instellingen → Add-ons → Add-onwinkel**).
2. Installeer **ESPHome Device Builder**, start deze en zet hem in de zijbalk.
   Gebruik ESPHome **2026.6.2 of nieuwer**. De release is met 2026.6.2 getest.
3. Open in de appwinkel het menu rechtsboven → **Repositories**. Voeg toe:
   `https://github.com/MaxGramser/homeassistant_espscreen`.
4. Zoek **ESP Screen Manager**, installeer en start deze. Zet **Starten bij
   opstarten** en **Tonen in zijbalk** aan. Open de webinterface.

De beheerpagina draait binnen Home Assistant en gebruikt je HA-aanmelding.
Je hoeft geen extra gebruiker, MQTT, blueprint of long-lived token in te stellen.
Installeer voor normaal gebruik de GitHub-versie: een handmatig gekopieerde lokale
test-add-on ontvangt geen updates uit GitHub.

## 2. Download je eigen installatie-YAML

Klik in ESP Screens op **Nieuw scherm**. Kies CYD of Guition, een herkenbare naam
en een unieke apparaatnaam zoals `scherm-keuken`. Klik **Maak installatie-YAML**. Gebruik **Kopieer YAML** om de tekst meteen in
ESPHome te plakken, of **Download bestand**. Op een HTTP-verbinding kan Chrome
downloads blokkeren; de kopieeroptie blijft bruikbaar.

Bij **Bewaar profiel in ESP Screens** controleert de wizard automatisch de bestaande
ESPHome `secrets.yaml`. Zijn `wifi_ssid` en `wifi_password` aanwezig, dan hoef je
geen wifi in te vullen. Alleen bij een eerste installatie zonder secretsbestand
vraagt hij die gegevens één keer. Bestaande secrets worden niet overschreven.

Dit bestand bevat unieke API- en OTA-sleutels. Bewaar het. Maak voor ieder nieuw
scherm een nieuw bestand. Download voor een bestaand scherm niet telkens een nieuw
bestand: daarmee zou je nieuwe sleutels genereren.

Open **ESPHome Device Builder → Secrets** en vul eenmaal je 2,4GHz-wifi in:

```yaml
wifi_ssid: "Jouw wifi"
wifi_password: "Jouw wifiwachtwoord"
```

Andere bestaande secrets mogen blijven staan. Gebruik geen gedeelde API-sleutel
voor alle nieuwe schermen; de gedownloade YAML bevat al een eigen sleutel.

## 3. Eerste flash via USB op de Raspberry

1. Klik in ESPHome op **New device**, gebruik dezelfde unieke naam en sla de
   directe installatie over. Kies zo nodig ESP32 als tijdelijke wizardkeuze.
2. Klik **Edit** bij het nieuwe apparaat. Vervang de **hele inhoud** door je
   gedownloade YAML en sla op. Het pakket levert zelf de juiste hardwareconfiguratie.
3. Sluit het scherm met USB aan op de Raspberry waarop Home Assistant draait.
4. Klik **Install → Plug into the computer running ESPHome Device Builder**.
   Kies de USB-seriële poort van dit scherm. Bij meerdere schermen: sluit ze voor
   de eerste installatie één voor één aan.
5. Wacht op een geslaagde build én upload. De eerste build kan op een Raspberry
   lang duren. Laat deze afronden. Het scherm maakt daarna verbinding met wifi.

Een browseroptie **Plug into this computer** bedoelt de computer waarop je browser
draait, niet de Raspberry. Kies die alleen wanneer het scherm aan die computer zit.

**CYD:** bij de eerste start verschijnt de kalibratie. Tik het zichtbare kruisje
drie keer rustig aan, houd elke tik kort vast en volg steeds het volgende kruisje.
Er zijn vijf posities. Het midden controleert de nauwkeurigheid. Bij een mislukte
meting vraagt het scherm opnieuw te beginnen. De correctie wordt lokaal opgeslagen
en blijft bij OTA-updates behouden. Via de HA-apparaatknop **Touch kalibreren** kun
je later opnieuw meten. Voor diagnose en de oudere handmatige installatie:
[CALIBREREN.md](CALIBREREN.md).

**Guition:** de GT911-touchmapping zit in het bordprofiel; er is geen ADC-kalibratie.

## 4. Koppel het scherm aan Home Assistant

1. Open **Instellingen → Apparaten & diensten**. Voeg het ontdekte ESPHome-apparaat
   toe. Niet ontdekt? Voeg de integratie **ESPHome** handmatig toe met het IP-adres
   uit de ESPHome-logs, poort 6053.
2. Vraagt HA om een encryptiesleutel? Kopieer de waarde van **api → encryption → key**
   uit je eigen YAML. Gebruik niet het OTA-wachtwoord.
3. Open bij de ESPHome-integratie **Configureren** en zet **Allow the device to
   perform Home Assistant actions** aan. Zonder deze toestemming verschijnen
   waarden wel, maar kan het scherm lampen en apparaten niet bedienen.
4. Open ESP Screens. Het scherm verschijnt binnen ongeveer 30 seconden.

## 5. Kies en wijzig je tegels

Selecteer je scherm, vul de titel in en zoek entiteiten. De kiezer heeft
domeinfilters met gekleurde iconen. Je kunt maximaal tien tegels toevoegen.
Het schermvoorbeeld toont hun plaats: twee kolommen, zes tegels per pagina.
Sleep een tegel naar een andere tegel om te ordenen. Klik op een tegel voor
een eigen naam, de pijltjes voor volgorde en **Bediening & weergave instellen**:
klikgedrag, een mini-slider of een grote waarde waar ondersteund. Een lege
tegel brengt je naar de zoekbalk; toevoegen vult de eerstvolgende vrije positie.
Het voorbeeld toont de indeling, geen live sensorwaarden.
Klik **Opslaan & naar scherm** om je wijzigingen door te sturen.

- Lamp, switch, input_boolean en fan: tik om aan/uit te zetten.
- Lamp lang indrukken: helderheid, regenboogkleur en wittemperatuur, voor zover
  de lamp die functies ondersteunt.
- Climate, vacuum en cover: tik om de bedieningskaart te openen.
- Fan lang indrukken: snelheid als het apparaat percentages ondersteunt.
- Scene/script: tik om uit te voeren; button/input_button: tik om in te drukken.
- Sensor/binary_sensor/input_select: toont de waarde, voert geen actie uit.

Er staan zes tegels op een pagina. Bij maximaal zes verdwijnen Vorige/Volgende.
De standby-tijd is standaard tien minuten. Bij offline apparaten blokkeert het
scherm acties. Als de app/HA langer dan circa 95 seconden geen updates geeft,
wordt de bediening geblokkeerd totdat de gegevens opnieuw ontvangen zijn.

Je mag indelingen opslaan terwijl een scherm offline is. De app verstuurt ze
zodra het scherm terugkomt. De app moet blijven draaien voor actuele tegeldata.

## 6. Updates zonder je instellingen kwijt te raken

| Wat verandert? | Wat doe je? | Wat blijft behouden? |
| --- | --- | --- |
| Andere entiteiten, namen of volgorde | Opslaan in ESP Screens | Wifi, sleutels, kalibratie |
| Nieuwe beheerpagina/appversie | Appwinkel → ESP Screen Manager → Update | Alle indelingen in `/data/screens.json` |
| Nieuwe schermfunctie/kaart | ESPHome → bestaand apparaat → Install → Wirelessly | Eigen YAML, sleutels en CYD-kalibratie; app stuurt tegels opnieuw |

De eigen YAML verwijst naar de firmwarepakketten in `main`. Bij een nieuwe build
haalt ESPHome de nieuwste gepubliceerde pakket- en componentcode op. Je vervangt
je eigen YAML dus niet door een nieuw downloadbestand. Wifi, naam en sleutels
staan buiten het gedeelde pakket en blijven gelijk.

Maak vóór updates een Home Assistant-back-up inclusief ESP Screen Manager en
ESPHome Device Builder. **Verwijderen/herinstalleren** van een app is niet hetzelfde
als updaten; daarmee kun je de gegevensmap wissen. Houd de apparaatnaam en de
entity-ID van **Tegelinstellingen** gelijk, zodat de bestaande indeling gekoppeld blijft.

Voor een nieuwe ondersteunde kaart update je eerst de app, daarna de firmware.
De beheerder moet protocol- en gegevensmigraties achterwaarts compatibel maken;
zie [RELEASING.md](RELEASING.md). Voor terugrollen kun je `ref: main` in je eigen
YAML tijdelijk vervangen door een eerdere releasetag, zonder de sleutels te wijzigen.

## Als iets niet werkt

- **Geen scherm in de lijst:** controleer of de nieuwe Easy Setup-firmware draait,
  de ESPHome-integratie verbonden is en de tekstentiteit **Tegelinstellingen**
  niet uitgeschakeld is. De oude handmatige firmware publiceert die standaard niet.
- **Wel tegels, geen acties:** geef het apparaat toestemming voor HA-acties.
- **Niet beschikbaar:** controleer of de geselecteerde entiteit in HA bestaat en
  beschikbaar is. Een hernoemde entity-ID moet je opnieuw kiezen.
- **Geen OTA:** controleer wifi/IP en het oorspronkelijke OTA-wachtwoord. Gebruik
  zo nodig dezelfde eigen YAML via USB. Genereer geen nieuwe identiteit.
- **Build mislukt:** lees de eerste fout, controleer ESPHome-versie en internet voor
  GitHub/fontdownloads. Gebruik bij te weinig Raspberry-geheugen tijdelijk een
  krachtigere computer voor compileren; de YAML blijft hetzelfde.
- **Bestaand handmatig scherm migreren:** bewaar de oude YAML en neem de bestaande
  apparaatnaam, API-sleutel en OTA-wachtwoord over in het nieuwe installatieprofiel.
  Kies daarna de tegels in de app. De oude vaste tegel-substituties worden niet
  automatisch in de nieuwe beheerpagina geïmporteerd.

HA Container zonder Supervisor heeft geen appwinkel. Deze installatiehandleiding
richt zich op Home Assistant OS; de ontwikkelserver is geen productieroute voor
een los openbaar portal.

Gebruikte HA-mechanismen: [Ingress](https://developers.home-assistant.io/docs/apps/presentation/),
[interne HA-API](https://developers.home-assistant.io/docs/apps/communication/) en
[ESPHome-pakketten](https://esphome.io/components/packages/).


## Scherminstellingen aanpassen (vanaf 0.1.2)

Update ESP Screen Manager naar 0.1.2 en installeer één keer de nieuwe firmware
via je **bestaande** ESPHome-apparaat → Install → Wirelessly. Behoud de eigen YAML
met wifi en sleutels. Open daarna het scherm in ESP Screen Manager en klap
**Scherminstellingen** open. Klik na aanpassen op **Opslaan & naar scherm**.
Hierna vragen wijzigingen aan deze instellingen geen nieuwe firmwareflash.

| Instelling | Mogelijkheden | Standaard |
|---|---|---|
| Automatisch standby | Aan/uit | Aan |
| Standby na | 1–1440 minuten na de laatste aanraking | 10 minuten |
| Helderheid normaal | 5–100% | 100% |
| Helderheid standby | 0–100%, maximaal normale helderheid | 20% |
| Nachtstand | Aan/uit; geldt tijdens standby | Aan |
| Begin/einde nacht | Uur en minuut, ook over middernacht | 22:00–07:00 |
| Helderheid nacht | 0–100%, maximaal normale helderheid | 10% |
| Klok tonen | Aan/uit | Aan |
| Tijdnotatie | 24 of 12 uur, zonder AM/PM | 24 uur |
| Terug naar pagina 1 | Bij standby; anders huidige pagina onthouden | Uit |

Nachturen gebruiken de tijdzone van het ESPHome-apparaat en de tijd uit HA.
Zonder geldige tijd gebruikt het scherm de gewone standbyhelderheid; gelijke
begin- en eindtijd schakelen het nachtvenster uit. Op 0% gaat alleen de
achtergrondverlichting uit: dit is geen deep sleep en geen schermbeveiliging.
De eerste tik wekt het scherm zonder een apparaat te bedienen.

De add-on bewaart alles per scherm in zijn permanente gegevens. Het scherm bewaart
de laatst ontvangen instellingen ook in preferences; ESPHome bundelt die
schrijfacties (normaal maximaal een minuut). Trek daarom niet direct na opslaan
de voeding los. Bestaande CYD-kalibratie, tegels, API- en OTA-sleutels blijven staan.
Een offline scherm krijgt wijzigingen zodra het terugkomt. Gewone HA-statusupdates
wekken het scherm niet en veranderen de standbytimer niet.

Bij oudere firmware toont de beheerpagina dat eerst een update nodig is.
De tegels blijven bruikbaar. Handmatige YAML-profielen blijven hun substitutions
gebruiken; voor beheer via deze app gebruik je het Easy Setup-pakket.

De gemelde Guition-paneelstrepen worden apart onderzocht. Deze instellingenupdate
wijzigt geen displaytimings of driver en is geen bewezen oplossing voor die strepen.
