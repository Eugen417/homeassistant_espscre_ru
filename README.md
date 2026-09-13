# Home Assistant ESP Screens

Een Home Assistant-bedieningsscherm dat je zelf indeelt. **ESP Screen Manager**
beheert je tegels én bouwt en installeert de ESPHome-firmware vanuit Home Assistant.
Geen blueprint, MQTT of long-lived token nodig voor normaal gebruik.

**Nieuw scherm? Volg de [complete installatiehandleiding](docs/EASY_SETUP.md).**
Voor elke nieuwe gebruiker en elk nieuw apparaat maak je een eigen profiel.

## Ondersteunde schermen

| Scherm | Resolutie | Display / touch |
| --- | --- | --- |
| CYD ESP32-2432S028 | 320 × 240 | ILI9341 / resistieve XPT2046 |
| Guition ESP32-S3-4848S040, 4 inch | 480 × 480 | ST7701S RGB / capacitieve GT911 |

Gebruik deze exacte bordvarianten: gelijkende productnamen kunnen andere
controllers of aansluitingen hebben. Wallbox-relais worden niet aangestuurd.
De firmware en ingebouwde CLI zijn getest met **ESPHome 2026.6.2**.

## Wat je kunt instellen

- **Tot twintig tegels**, verdeeld over maximaal vier vaste pagina's met zes
  tegels. Zoek op entiteit, apparaat of ruimte en sleep om te ordenen.
- **Instellingen per tegel:** eigen naam, klikgedrag, een kleine slider waar
  ondersteund, of een grote waarde voor bijvoorbeeld temperatuur en verbruik.
  De grote waarde toont vanaf firmware 0.2.13 een klein domeinicoon naast de
  titel; een te lang getal wordt met puntjes afgekort, de eenheid blijft staan.
- **Pastel achtergronden per tegel:** kies rood voor een alles-uit-script,
  groen voor alles-aan, of een andere kleur. Titel en status blijven donker en
  leesbaar. De kleur verschijnt ook in het schermvoorbeeld; **Standaard** herstelt
  de normale kleuren. Vereist firmware 0.2.10 of nieuwer.
- **Lampbediening:** helderheid, regenboogkleur en wittemperatuur volgens de
  mogelijkheden van de lamp. Open de detailbediening met een lange aanraking.
- **Meer kaarten:** climate, vacuum, fan, cover, media player, sensoren,
  select/input_select, number/input_number, schakelaars, scènes, scripts en
  buttons. Een sensor kan een historiekaart voor 1, 6 of 24 uur openen.
- **Speciale kaarten (firmware 0.2.14+):** een **klok** (digitaal of analoog)
  als ingebouwde tegel, een **weersvoorspelling** met vijf dagen op een
  dubbelbrede kaart, een **grafiek** van de sensorgeschiedenis in de tegel,
  **zonsopgang/-ondergang** (`sun.sun`), een **kookwekker** (`timer.*`, tikken
  start of pauzeert) en **aanwezigheid** (`person.*`). Kies ze in de kiezer als
  elke andere tegel; **Dubbelbreed** is een optie voor iedere tegel.
- **Scherminstellingen:** standby-tijd, normale en gedimde helderheid,
  nachturen, klok, terug naar de hoofdpagina en optioneel swipen tussen pagina's.
- **Guition-rotatie:** 0°, 90°, 180° of 270°, direct vanuit de beheerpagina.
  De native LVGL-rotatie draait beeld en touch samen. De CYD behoudt zijn vaste
  oriëntatie en eigen kalibratie.
- **Inspector:** controleer entiteiten, status en configuratie in ESP Screens.
  Actiefeedback laat zien dat een opdracht onderweg is.

Functies hangen af van de mogelijkheden die Home Assistant voor een entiteit
meldt. De app moet blijven draaien om de schermen van actuele gegevens te voorzien.

Vanaf firmware **0.2.12** toont lang indrukken op een switch een grote
schakelaar. Een korte tik schakelt direct; de uitstand krijgt een grijs icoon.
De feedback stopt zodra Home Assistant de gewijzigde stand meldt, met een
minimum van 150 ms voor switches. Tegels met een mini-slider behouden hun
icoon; op CYD staan het icoon en tekstblok verticaal gecentreerd.

## Installeren vanuit Home Assistant

### Heb ik ESPHome nodig?

**Je hoeft de aparte ESPHome Device Builder-app niet te installeren.**
ESP Screen Manager bevat de ESPHome-CLI al en kan zelf firmware bouwen,
via USB installeren en later draadloos via OTA bijwerken.

**Je moet het geflashte scherm wel koppelen via de ESPHome-integratie in HA.**
Die koppeling staat onder **Instellingen → Apparaten & diensten**, niet in de
appwinkel. Voeg daar het ontdekte apparaat toe. Verschijnt het niet automatisch,
kies dan **Integratie toevoegen → ESPHome** en vul het IP-adres van het scherm in.
Gebruik bij een sleutelvraag de `api.encryption.key` uit je eigen apparaat-YAML
en geef het apparaat toestemming om Home Assistant-acties uit te voeren.

| Onderdeel | Nodig? | Waarvoor? |
| --- | --- | --- |
| ESP Screen Manager-app | Ja, voor deze installatieroute | Firmware installeren, tegels beheren en actuele gegevens naar het scherm sturen |
| ESPHome Device Builder-app | Nee, optioneel | Alternatieve editor en firmware-installatie; dezelfde CLI zit al in ESP Screens |
| ESPHome-integratie in HA | Ja, koppel ieder scherm | De verbinding tussen Home Assistant en het fysieke scherm |

Een verse installatie zonder ESPHome Device Builder werkt dus ook. Als er nog
geen ESPHome `secrets.yaml` bestaat, vraagt onze wizard de wifi eenmalig en
bewaart die lokaal. Bestaande wifi-secrets worden hergebruikt. API- en OTA-sleutels
worden per nieuw scherm aangemaakt en blijven in het eigen apparaatprofiel.

### Stap voor stap

Voor Home Assistant OS met Apps/Add-ons op **aarch64 of amd64**:

1. Open de appwinkel en voeg deze repository toe:
   `https://github.com/MaxGramser/homeassistant_espscreen`.
2. Installeer **ESP Screen Manager**, start de app en open **ESP Screens**.
   ESPHome Device Builder is optioneel: de ESPHome-CLI zit al in deze app.
3. Kies **Nieuw scherm**, selecteer CYD of Guition en geef een unieke naam.
   Gebruik **Bewaar profiel in ESP Screens**. Bestaande `wifi_ssid` en
   `wifi_password` in ESPHome `secrets.yaml` worden hergebruikt; bij een verse
   installatie vraagt de wizard de wifi eenmalig. API- en OTA-sleutels worden
   uniek aangemaakt en in je eigen apparaat-YAML bewaard.
4. Sluit het scherm met een USB-datakabel aan op de **Home Assistant-machine**.
   Open **Firmware & USB**, kies het eigen profiel en de juiste USB-poort en
   start **Bouwen & installeren**. Een eerste build kan meerdere minuten duren.
5. **CYD:** doorloop de kalibratie op het scherm. **Guition:** gebruikt GT911
   zonder resistieve kalibratie. Koppel daarna het ontdekte ESPHome-apparaat in
   **Instellingen → Apparaten & diensten**. Gebruik bij een sleutelvraag de
   `api.encryption.key` uit je eigen YAML. Geef het apparaat toestemming om
   Home Assistant-acties uit te voeren.
6. Selecteer het scherm in ESP Screens, kies je tegels en klik
   **Opslaan & naar scherm**. Test vervolgens de fysieke bediening.

Je kunt later vanuit **Firmware & USB → Wifi / OTA** nieuwe firmware installeren.
Gebruik voor een bestaand scherm altijd het bestaande profiel; opnieuw een
installatieprofiel aanmaken genereert nieuwe sleutels.

## Tegels en kleuren aanpassen

Open **Tegels instellen**, klik een tegel in het schermvoorbeeld en open
**Bediening & weergave instellen**. Kies bij **Pastel achtergrond** een kleur,
zoals rood of groen. Pas eventueel de naam, klikactie, mini-slider of grote
waarde aan. Klik **Opslaan & naar scherm** om de wijzigingen toe te passen.
Dit vereist na de eerste ondersteunende firmware-update geen nieuwe flash.

Een kleur is een vaste keuze voor die tegel: hij blijft dus bijvoorbeeld rood
wanneer je het alles-uit-script gebruikt. De entiteitsstatus en actiefeedback
blijven afzonderlijk zichtbaar.

## Updates en behoud van je instellingen

| Wijziging | Actie |
| --- | --- |
| Tegels, namen, kleuren, volgorde of scherminstellingen | Opslaan in ESP Screens; geen firmwareflash |
| Nieuwe versie van de beheerpagina | ESP Screen Manager updaten in de HA-appwinkel |
| Nieuwe functie op het fysieke scherm | Bestaand profiel bijwerken via Firmware & USB → Wifi / OTA |

De eigen YAML en wifi/API/OTA-instellingen blijven in de ESPHome-configmap.
Tegelindelingen en opties staan in de permanente appdata. CYD-kalibratie en
schermvoorkeuren blijven op het apparaat opgeslagen. Updates vervangen deze
gebruikersgegevens niet. Maak wel normale Home Assistant-back-ups en bewaar je
apparaatprofielen; een app verwijderen of flashgeheugen wissen is geen update.

Zie [releasegeschiedenis](screen_manager/CHANGELOG.md) en
[releases en protocolcompatibiliteit](docs/RELEASING.md).

## Handleidingen en hulp bij installatie

- [Complete installatie vanuit ESP Screens](docs/EASY_SETUP.md)
- [Guition-hardware, montage en rotatie](docs/GUITION.md)
- [CYD-kalibratie en USB-diagnose](docs/CALIBREREN.md)
- [Fysieke acceptatietest](docs/ACCEPTATIE.md)
- [Instructies voor developers en LLM's](AGENTS.md)

Geef een developer of LLM een schone kopie van deze repository en bijvoorbeeld:

> Lees AGENTS.md, README.md en docs/EASY_SETUP.md. Help me dit CYD- of
> Guition-scherm via USB op mijn Home Assistant te installeren. Identificeer
> het bord en gebruik mijn bestaande profiel als dat er al is. Begeleid
> kalibratie, HA-koppeling, tegelkeuze en fysieke tests. Houd sleutels lokaal
> en geef aan welke controles werkelijk zijn uitgevoerd.

Een build bewijst niet dat fysieke touch of het paneelbeeld goed is. De eigenaar
moet het beeld controleren en de gevraagde tikken uitvoeren.

<details>
<summary>Oudere handmatige CYD-installatie via een computer</summary>

Onderstaande route gebruikt het oudere handmatige profiel met maximaal tien
tegels. De positiebeperkingen hiervan gelden niet voor de twintig runtime-tegels
van ESP Screen Manager. Gebruik voor nieuwe installaties bij voorkeur de route
hierboven; de handmatige instructies blijven beschikbaar voor onderhoud.

# CYD Home Assistant-bedieningsscherm

Van een nieuwe **ESP32-2432S028 met ILI9341 + XPT2046** naar een gekalibreerd
Home Assistant-scherm, via USB. Met lichtbediening, scènes/scripts, climate,
vacuum, maximaal tien tegels en vaste pagina's. Bij maximaal zes tegels
verdwijnt de paginering. Standby begint standaard na tien minuten.

**Handmatige installatie via de computer:** je hoeft geen Home Assistant-token te maken
of een ESPHome-add-on te installeren om via de computer te flashen.

## Met een LLM of developer werken

Geef de uitgepakte map en deze opdracht:

> Lees eerst AGENTS.md en README.md. Begeleid mij bij een nieuwe installatie
> op een ESP32-2432S028 die via USB is aangesloten, met mijn eigen Home
> Assistant. Controleer het bord, de seriële poort en de Python-omgeving.
> Maak een lokaal device.yaml-profiel; gebruik geen bestaande persoonlijke
> entiteiten of paneelkalibratie. Doorloop USB-kalibratie, HA-koppeling,
> tegelconfiguratie en de acceptatietest. Vraag mij om fysieke tikken wanneer
> nodig. Houd sleutels en wachtwoorden in lokale bestanden en rapporteer
> welke controles echt zijn uitgevoerd.

De begeleider kan de code en logs lezen, maar kan het fysieke scherm niet
vanzelf zien. Jij controleert het beeld en voert de gevraagde tikken uit.

## 1. Benodigdheden controleren

- Het bedoelde bord: ESP32-2432S028, 320×240, **ILI9341-display en resistieve
  XPT2046-touch**. Er bestaan borden met bijna dezelfde naam en een andere
  schermcontroller. Deze handleiding garandeert die varianten niet.
- Een USB-**datakabel** en een computer met macOS, Linux of Windows.
- Python **3.11–3.14**, internet voor de eerste build en enkele GB vrije ruimte.
  De versiegrens komt uit het geteste ESPHome-pakket; gebruik geen Python 3.15.
- Een 2,4GHz-wifinetwerk; Home Assistant moet het bord via het netwerk kunnen
  bereiken. Houd het wifiwachtwoord lokaal beschikbaar.
- Toegang tot Home Assistant om entiteiten te kiezen en ESPHome toe te voegen.

Gebruik bij voorkeur één aangesloten ESP-bord tegelijk. Noteer de variant en
het MAC-adres uit de boot-/uploadlogs. `home-like.yaml` en `buttons.yaml` uit
het oorspronkelijke project zijn **niet** de startersconfiguratie voor dit bord.

## 2. De werkomgeving installeren

Kloon [de repository](https://github.com/MaxGramser/homeassistant_espscreen)
of pak de starter-ZIP uit. Open een terminal **in de codefolder**. Alle opdrachten hieronder
worden vanuit die folder uitgevoerd.

macOS/Linux:

```sh
python3 -m venv .venv
source .venv/bin/activate
python -m pip install --upgrade pip
python -m pip install -r requirements.txt
```

Windows PowerShell:

```powershell
py -3.13 -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install --upgrade pip
python -m pip install -r requirements.txt
```

Als PowerShell activeren blokkeert, gebruik rechtstreeks
`.\.venv\Scripts\python.exe` in plaats van `python`; een systeemwijziging is
niet nodig. Controleer daarna:

```sh
python --version
python -m esphome version
python -m serial.tools.list_ports
```

Verwacht **ESPHome 2026.6.2**. Deze repo bevat een aangepaste touchdriver die
met deze versie is getest. Upgrade niet stilzwijgend tijdens een installatie.
Zie ook de [officiële ESPHome-installatiehandleiding](https://esphome.io/install/).

Noteer de poort: bijvoorbeeld `/dev/cu.usbserial-130` op macOS,
`/dev/ttyUSB0` op Linux of `COM3` op Windows. Verderop staat `<USB_POORT>`;
vervang dat steeds door de werkelijke poort, zonder punthaken.

## 3. Een eigen apparaatprofiel maken

Kies een unieke naam, bijvoorbeeld `display-keuken`:

```sh
python tools/new_device.py --name display-keuken --friendly-name "Display keuken"
```

Dit maakt drie lokale bestanden en overschrijft niets:

| Bestand | Wat je erin aanpast |
|---|---|
| `device.yaml` | Apparaatnaam, ruimte, tegels en gedrag |
| `calibration.yaml` | Kalibratie van dit ene fysieke paneel; de wizard schrijft dit |
| `secrets.yaml` | Wifi, API-encryptiesleutel, OTA- en fallbackwachtwoord |

De sleutels worden uniek gegenereerd. Vul in `secrets.yaml` alleen jouw
`wifi_ssid` en `wifi_password` in. Laat de gegenereerde sleutels staan en
bewaar dit bestand bij je eigen configuratie. Deel het niet mee in de ZIP.
De API-encryptiesleutel heb je later nodig bij de HA-koppeling; kopieer die
lokaal uit het bestand, niet via een openbare chat.

Bestaan deze bestanden al? Werk in een nieuwe kopie van de map voor een nieuw
scherm. Wis de sleutels/kalibratie van een werkend scherm niet.

`device.yaml` importeert de basis en jouw kalibratie. Het overschrijft ook
**alle verborgen tegelposities**, zodat die geen entiteiten van iemand anders
blijven volgen. Begin met de zes voorbeeldtegels. `DIRECT_ACTIONS: "false"`
voorkomt directe HA-aansturing tijdens de eerste installatie. Maak in deze
fase nog geen HA-automations die op de actie-sensor reageren.

## 4. Eerste USB-flash en kalibratie

Controleer eerst de configuratie (ESPHome schermt secrets standaard af):

```sh
python -m esphome config device.yaml
```

Flash de kalibratiemodus, die zonder HA-verbinding werkt:

```sh
python -m esphome -s CALIBRATION_ON_BOOT true run device.yaml --device <USB_POORT>
```

De eerste build kan meerdere minuten duren. Na upload verschijnen vijf
kruisjes op een donker scherm. `run` blijft logs tonen: stop **alleen de
loglezer met Ctrl+C** voordat je de kalibratiewizard start. Het bord blijft aan.

Volg nu [docs/CALIBREREN.md](docs/CALIBREREN.md): per doel worden drie tikken
verzameld, de vier hoeken bepalen de correctie en het midden is een
onafhankelijke controle. Daarna flash je de uitkomst en meet je opnieuw.
**Kopieer nooit de meetwaarden van een ander paneel.**

## 5. Home Assistant koppelen

Na een geslaagde kalibratie flash je de normale modus:

```sh
python -m esphome run device.yaml --device <USB_POORT>
```

Controleer in de logs de wifi-IP, stabiele boot en apparaatnaam. Ctrl+C sluit
de loglezer. Het scherm moet nu de normale tegelpagina tonen.

1. Open in HA **Instellingen → Apparaten & diensten**.
2. Kies het ontdekte ESPHome-apparaat, of **Integratie toevoegen → ESPHome**.
3. Gebruik `display-keuken.local` of het IP-adres uit de logs; API-poort **6053**.
4. Vul de `api_encryption_key` uit je lokale `secrets.yaml` in als daarom wordt gevraagd.
5. Controleer of de apparaatnaam klopt en de status online wordt.
6. Open de opties/configuratie van deze ESPHome-integratie en schakel
   **Allow the device to perform Home Assistant actions** in.

De precieze vertaling/plaats van de optie verschilt per HA-versie. Zie de
[officiële integratiehandleiding](https://www.home-assistant.io/integrations/esphome).
De HA-integratie verbindt met het bord; de ESPHome Device Builder/add-on is
optioneel en is een andere functie.

## 6. Eigen tegels instellen

Volg [docs/TEGELS.md](docs/TEGELS.md). Verzamel de echte entity-ID's in HA,
controleer ondersteunde attributen/acties en pas `device.yaml` aan. Gebruik
korte titels. Als je alles hebt gecontroleerd:

```yaml
substitutions:
  DIRECT_ACTIONS: "true"
  TILE_COUNT: "6"
```

Dit zijn wijzigingen in de **bestaande** substitutions-map, geen tweede map
onderaan het bestand. Bouw en flash opnieuw met `run device.yaml`. De tegels
volgen daarna de werkelijke HA-status. Een actieve API-verbinding alleen
bewijst nog niet dat een actie door HA is toegestaan.

## 7. Opleveren en testen

Voer [docs/ACCEPTATIE.md](docs/ACCEPTATIE.md) uit. Dat omvat een onafhankelijke
touchmeting, beide pagina's indien aanwezig, lange druk, sliders, climate,
vacuum, werkelijke HA-status en de juiste actie-effecten. De ingebouwde
renderproef stuurt zelf geen HA-apparaten aan:

```sh
python diagnostics/run_ui_test.py --host display-keuken.local --name display-keuken
```

Bewaar bij deze persoon: `device.yaml`, `calibration.yaml`, `secrets.yaml`, de
meetbestanden, de gebruikte ESPHome-versie en een kort opleverrapport.
Voor later bijwerken kun je USB blijven gebruiken of OTA gebruiken:

```sh
python -m esphome run device.yaml --device display-keuken.local
```

Verander naam/API-sleutel niet zonder ook de HA-koppeling bij te werken.
Bij storing: [docs/PROBLEMEN.md](docs/PROBLEMEN.md).

## Deze code aan de volgende persoon geven

Gebruik de exporteur in plaats van je hele werkmap te kopiëren:

```sh
python tools/export_bundle.py --output dist/cyd-starter.zip
```

De ZIP bevat de code, lokale fonts, licenties, handleidingen en neutrale
voorbeelden. De basisdefaults worden voor de ontvanger geneutraliseerd.
Geen `secrets.yaml`, eigen `device.yaml`, metingen, logs, `.git`, `.esphome`,
Python-omgeving of firmwarebackups. De exporteur weigert een bestaande ZIP
te overschrijven; kies bij een nieuwe versie een andere bestandsnaam.

De originele basis is van
[akuehlewind/ESPHome-touch-display-mount](https://github.com/akuehlewind/ESPHome-touch-display-mount).
Project-, ESPHome-driver- en fontlicenties staan respectievelijk in `LICENSE`,
`components/xpt2046/LICENSE` en `fonts/`. Historische borddiagnose staat in
[CYD_STABILITY.md](CYD_STABILITY.md); volg voor een **nieuwe** installatie de
handleidingen hierboven.

</details>
