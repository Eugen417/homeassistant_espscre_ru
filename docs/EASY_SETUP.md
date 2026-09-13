# Installeren en beheren vanuit ESP Screens

Een nieuw scherm, van USB naar dagelijks gebruik

Met **ESP Screen Manager** kies je je tegels in Home Assistant. Je zoekt op
naam, apparaat, ruimte of entity-ID, zet ze in de gewenste volgorde en klikt
op **Opslaan & naar scherm**. Daarna blijft de status automatisch actueel.
Je hoeft voor andere tegels **niet opnieuw te flashen**.

Dit werkt met Home Assistant OS op een 64-bits Raspberry Pi of een amd64-machine,
en een van deze exacte schermvarianten. De ESPHome-CLI zit in ESP Screen Manager;
ESPHome Device Builder is optioneel:

| Keuze | Hardware |
| --- | --- |
| CYD | ESP32-2432S028, 320×240, ILI9341 en XPT2046 |
| Guition | ESP32-S3-4848S040, 480×480, ST7701S en GT911 |

Andere schermen met ongeveer dezelfde naam kunnen andere pinnen hebben. Gebruik
het bordprofiel dat bij de hardware hoort. Gebruik een USB-kabel die data ondersteunt.
De wallbox-relais worden niet gebruikt.

## 1. Installeer ESP Screen Manager

1. Open **Instellingen → Apps → Installeer een app** (in oudere HA-versies:
   **Instellingen → Add-ons → Add-onwinkel**).
2. Open het menu rechtsboven → **Repositories** en voeg toe:
   `https://github.com/MaxGramser/homeassistant_espscreen`.
3. Installeer en start **ESP Screen Manager**. Zet **Starten bij opstarten**
   en **Tonen in zijbalk** aan. Open de webinterface **ESP Screens**.

Deze app bevat de geteste ESPHome 2026.6.2-CLI en draait binnen je HA-aanmelding.
Een tweede ESPHome-beheerpagina, MQTT, blueprint of long-lived token is niet nodig.
Gebruik de GitHub-versie voor updates; een lokale test-add-on is een andere app.

## 2. Maak en bewaar je eigen apparaatprofiel

Klik **Nieuw scherm**. Kies CYD of Guition, een herkenbare naam en een unieke
apparaatnaam, bijvoorbeeld `scherm-keuken`. Klik **Bewaar profiel in ESP Screens**.

De wizard controleert de bestaande ESPHome `secrets.yaml`. Met `wifi_ssid` en
`wifi_password` aanwezig hoef je geen wifi in te vullen. Alleen als er nog geen
secretsbestand is, vraagt de wizard je 2,4GHz-wifi eenmalig. Een bestaand bestand
met ontbrekende of ongeldige wifi-sleutels moet je eerst herstellen; het wordt
niet stilzwijgend overschreven.

Je apparaat-YAML bevat unieke API- en OTA-sleutels. Bewaar dit profiel en gebruik
het opnieuw bij updates. Voor een bestaand scherm telkens een nieuw profiel
maken genereert nieuwe sleutels en is niet de updateroute.

Wil je ESPHome Device Builder gebruiken, kies dan **Maak installatie-YAML** en
kopieer/download die YAML naar een eigen apparaat daar. Beide routes gebruiken
dezelfde firmwarepakketten. Wifi blijft in ESPHome `secrets.yaml`; API- en
OTA-sleutels staan in het eigen apparaatprofiel.

## 3. Eerste flash via USB op de Raspberry

1. Sluit het scherm met een **USB-datakabel** aan op de machine waarop Home
   Assistant draait. Bij meerdere borden: identificeer eerst de juiste poort,
   of sluit ze voor de eerste installatie één voor één aan.
2. Open **Firmware & USB** in ESP Screens en kies het zojuist bewaarde profiel.
3. Kies bij **Installeren naar** de USB-poort van dit scherm.
4. Klik **Bouwen & installeren** en wacht op een geslaagde build én upload.
   De eerste build kan op een Raspberry meerdere minuten duren.
5. Het scherm herstart en verbindt met wifi. Latere firmware-updates kunnen via
   **Wifi / OTA** in dezelfde pagina, met het bestaande profiel.

Een USB-kabel aan je laptop is niet zichtbaar als USB-poort van de Raspberry.
Gebruik voor die route de ESPHome-CLI op de laptop met je eigen profiel, of de
browserinstallatie van ESPHome Device Builder. De stappen hierboven gebruiken
uitsluitend ESP Screens op de HA-machine.

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
domeinfilters met gekleurde iconen. Je kunt maximaal twintig tegels toevoegen.
Het schermvoorbeeld toont hun plaats: twee kolommen, zes tegels per pagina.
Sleep een tegel naar een andere tegel om te ordenen. Klik op een tegel voor
een eigen naam, de pijltjes voor volgorde en **Bediening & weergave instellen**:
klikgedrag, een mini-slider, een grote waarde, een grafiek (sensoren), een
weersvoorspelling (weer) of de breedte **Dubbelbreed**. Een dubbelbrede tegel van
een klimaat, schakelaar, lamp, ventilator, stofzuiger, zonwering, mediaspeler,
getal, keuzelijst, kookwekker, scène, script of knop krijgt rechts **directe
bediening** zoals de rijen in Home Assistant (bijvoorbeeld temperatuur − / +,
open/stop/dicht, volume met dempen, een toggle); kies onder **Directe bediening
op de tegel** welke set, of **Geen** (firmware 0.2.19+). **Bediening openen** op
een weertegel toont de weerkaart met de komende uren en dagen (regen inbegrepen);
op een klimaattegel de kaart met aan/uit-knop en de modus-, ventilator- en
zwenkstanden. De ingebouwde **Klok**
staat bovenaan de kiezer; zon, kookwekkers en personen vind je via de filters. Kies bij
**Pastel achtergrond** een eigen kleur met donkere tekst; **Standaard** herstelt
de normale weergave en **Geen** laat de kaart weg, zodat de inhoud even groot
direct op de schermachtergrond staat (firmware 0.2.16+). Hiervoor is firmware 0.2.10+ nodig. Een lege
tegel brengt je naar de zoekbalk; toevoegen vult de eerstvolgende vrije positie.
Het voorbeeld toont de indeling, geen live sensorwaarden.
Klik **Opslaan & naar scherm** om je wijzigingen door te sturen.

- Lamp, switch, input_boolean en fan: tik om aan/uit te zetten.
- Lamp lang indrukken: helderheid, regenboogkleur en wittemperatuur, voor zover
  de lamp die functies ondersteunt.
- Climate, vacuum en cover: tik om de bedieningskaart te openen.
- Fan lang indrukken: snelheid als het apparaat percentages ondersteunt.
- Scene/script: tik om uit te voeren; button/input_button: tik om in te drukken.
- Sensor: tik voor de historiekaart; stel bij de tegel 1, 6 of 24 uur in.
- Binary sensor: status bekijken. Select/input_select: open het keuzemenu.

Vanaf firmware 0.2.7 passen twintig tegels op maximaal vier pagina’s. Oudere
firmware houdt de limiet van tien totdat je bijwerkt. Via **Algemene instellingen
→ Vegen tussen pagina’s** kun je horizontaal swipen inschakelen. Sliders bedienen
alleen hun waarde; detailmenu’s en standby wisselen niet van pagina.

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
| Nieuwe schermfunctie/kaart | Knop **Bijwerken** bij het scherm, of vinkje **Elke nacht automatisch bijwerken** (handmatig: Firmware & USB → Wifi / OTA) | Eigen YAML, sleutels en CYD-kalibratie; app stuurt tegels opnieuw |

De eigen YAML verwijst naar de firmwarepakketten in `main`. Bij een nieuwe build
haalt ESPHome de nieuwste gepubliceerde pakket- en componentcode op. Je vervangt
je eigen YAML dus niet door een nieuw downloadbestand. Wifi, naam en sleutels
staan buiten het gedeelde pakket en blijven gelijk.

Maak vóór updates een Home Assistant-back-up inclusief ESP Screen Manager en
de eigen ESPHome-configuraties. **Verwijderen/herinstalleren** van een app is niet hetzelfde
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
| Terug naar pagina 1 | Sluit ook detailmenu’s bij standby | Uit |
| Vegen tussen pagina’s | Native horizontale swipe, firmware 0.2.7+ | Uit |
| Guition draaien | 0°, 90°, 180°, 270°, firmware 0.2.9+ | 0° |

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

De huidige Guition gebruikt de native ST7701S-configuratie; zie
[de hardwarevergelijking](GUITION_FACTORY_REFERENCE.md). Instellingen en
tegelkleuren veranderen geen paneeltimings. Controleer beeld en touch fysiek.
