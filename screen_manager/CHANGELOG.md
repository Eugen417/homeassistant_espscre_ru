## 0.2.19 (firmware 0.2.18 blijft actueel)

- **Sneller overzicht**: `/api/inventory` las bij elke aanroep alle ESPHome-profielen opnieuw in (tot vier keer per verzoek), waardoor de add-on op een Pi seconden per verzoek blokkeerde. Profielen worden nu per bestand gecachet op inode, wijzigingstijd en grootte; alleen een gewijzigd profiel wordt opnieuw gelezen en verwijderde profielen verdwijnen direct. Het parsen zelf gebruikt libyaml wanneer die beschikbaar is (circa tien keer sneller).
- Het overzicht en de updatestatus lezen de profiellijst en de HA-inventaris nog één keer per verzoek; de synchronisatielus rekent de inventaris één keer per ronde uit in plaats van per scherm.
- De pagina pollt niet meer in een verborgen tabblad en ververst direct zodra het tabblad weer zichtbaar is.
- Alleen de app verandert; firmware 0.2.18 blijft actueel.

## 0.2.18 (firmware 0.2.18)

- **Eigen icoon per tegel**: in het tegelpaneel staat onder de naam het veld **Icoon**. Kies uit 150 iconen in twaalf groepen (verlichting, ruimtes, klimaat, weer, media en muziek, beveiliging, apparaten, energie, zonwering, tuin en huisdieren, mensen en onderweg, overig), met zoeken op Nederlandse naam, Engelse MDI-naam of groep. De mockup en de kop van het paneel tonen het gekozen icoon direct.
- **Automatisch** gebruikt het icoon dat je in Home Assistant aan de entiteit gaf (`mdi:…`), als het in de set zit; anders het standaardicoon zoals voorheen. De mockup toont nu overal het icoon dat het scherm werkelijk tekent.
- Firmware 0.2.18 bevat alle 150 iconen in de drie icoonfonts van beide borden (CYD +22 KB, Guition +36 KB). Oudere firmware negeert de keuze en houdt het standaardicoon; het paneel meldt dat een update nodig is.
- De weersvoorspelling blijft het icoon van het actuele weer tonen; klok, voorspelling en zonnebaan hebben geen icoonkeuze.

## 0.2.17 (firmware 0.2.17 blijft actueel)

- Het vinkje **Elke nacht automatisch bijwerken** kreeg de algemene invoerstijl (100% breed met padding), waardoor de tekst buiten de zijkolom onder het editorpaneel viel. Het checkboxje heeft nu een vaste maat. Alleen de app verandert.

## 0.2.16 (firmware 0.2.17)

- **Bijwerken met één knop**: een scherm met oudere firmware krijgt in de lijst een badge *Update 0.2.17* en een knop **Bijwerken**. De app bouwt het eigen profiel, installeert draadloos en toont een spinner tot het scherm terug is met de nieuwe versie en een minuut stabiel blijft. Het resultaat blijft een dag zichtbaar bij het scherm.
- **Elke nacht automatisch bijwerken**: vinkje onder *Firmware-updates*. Tussen 03:00 en 06:00 (tijdzone van HA) werkt de app schermen met oudere firmware één voor één bij, met twee minuten pauze ertussen. Mislukt een scherm, dan stopt de ronde en verschijnt een melding in Home Assistant; de overige schermen blijven op hun oude firmware.
- **Alle schermen bijwerken** doet dezelfde ronde direct, bijvoorbeeld na een app-update.
- Nieuwe firmware is er zodra deze app een nieuwe versie heeft: de app kent de bijbehorende firmwareversie en vergelijkt die met `Schermfirmware` per scherm.
- Firmware 0.2.17 meldt twee diagnostische sensors extra: **Apparaatnaam** (de ESPHome-naam, gelijk aan het YAML-profiel) en **IP-adres**. Daarmee vindt de app zelf het profiel en het OTA-adres. Een scherm met oudere firmware wordt op apparaatnaam aan een profiel gekoppeld en vraagt eenmalig het IP-adres.

## 0.2.15 (firmware 0.2.16)

- **Achtergrond: Geen** als extra keuze in het tegelpalet: de kaart en de rand vallen weg en de tegelinhoud staat direct op de schermachtergrond, in dezelfde maat en op dezelfde plek als mét kaart. Werkt voor elke tegel; de mockup toont zo'n tegel met een stippellijn. Vereist schermfirmware 0.2.16; de app bewaart de indeling en meldt het als het scherm ouder is.
- **Analoge klok**: streepjes als index met de cijfers 12, 3, 6 en 9 (de CYD houdt alleen streepjes). Op een enkele tegel staat naast de wijzerplaat een kalenderblok — weekdag, grote dag, maand (CYD: "13 sep"). Dubbelbreed blijft de digitale tijd met datum naast de wijzerplaat.

## 0.2.14 (firmware 0.2.15 blijft actueel)

- Tegelinstellingen openen in een paneel bóven de schermmockup (op mobiel een sheet onderaan): naam, weergave, breedte, tikgedrag, slider, geschiedenis en kleur als knoppen, direct zichtbaar in de mockup erachter. Geen springende pagina meer.
- Verwijderen kan direct in de mockup met het kruisje op een tegel; de melding onderaan heeft **Ongedaan maken**.
- De tegellijst onder de mockup is vervallen; ordenen doe je door te slepen.

## 0.2.13 (firmware 0.2.15)

- Editor: sleep tegels in de schermmockup om te ordenen en sleep entiteiten rechtstreeks uit de lijst naar een plek in de mockup — met muis én touch (even vasthouden). De opslaan-balk blijft altijd in beeld.
- Nieuwe kop met duidelijke acties (**Nieuw scherm**, **Firmware & USB**, **Uitleg**) en een korte uitleg in drie stappen; de firmwaredialoog legt profiel, doel en knoppen uit.
- **Zonnebaan**: `sun.sun` toont dubbelbreed een horizon met de zon op zijn huidige positie tussen opkomst en ondergang (’s nachts onder de horizon), met beide tijden. Nieuwe zon-, weer- en kloktegels starten meteen dubbelbreed in hun mooiste weergave.
- Grafiek: vloeiende curve door dezelfde 24 punten met een zachte vulling eronder, zoals de HA-trendkaart.
- Scherm: de paginering is één geïntegreerde onderbalk — links tikken is vorige, rechts is volgende, het paginanummer staat in het midden.
- Scherm: bij een paginawissel verschijnen direct de kaartkaders van de nieuwe pagina (skeleton) en vult de inhoud een fractie later; geen oude waarden meer in nieuwe kaders.
- CYD: hetzelfde lichte kleurenschema als de Guition — lichtgrijze achtergrond, witte kaarten met een fijne rand, donkere tekst en lichtblauwe accenten, ook in de bedieningskaarten.

## 0.2.12 (firmware 0.2.14)

- Nieuwe kaarten in de kiezer: **Klok** (digitaal of analoog, ingebouwd), **weersvoorspelling** met vijf dagen op een dubbelbrede weerkaart, **grafiek** van de sensorgeschiedenis in de tegel, **zon** (`sun.sun`, opkomst en ondergang in je eigen tijdzone), **kookwekker** (`timer.*`, live aftellen; tikken start of pauzeert, lang indrukken annuleert) en **aanwezigheid** (`person.*`).
- **Dubbelbreed** als breedte-optie voor elke tegel. Een dubbelbrede tegel begint links en telt voor twee vakjes; het schermvoorbeeld toont ook de lege plek ervoor.
- De weerkaart toont het icoon van de actuele weersituatie. Voorspellingen komen van `weather.get_forecasts` en worden elk half uur ververst.
- Nieuwe domeinen en de klok vereisen schermfirmware 0.2.14; de app weigert opslaan voor oudere firmware en bewaart je bestaande indeling. Breedte en weergave-opties negeert oudere firmware gewoon.

## Firmware 0.2.13 (app 0.2.11 blijft bruikbaar)

- Grote waarde: klein domeinicoon naast de titel, titel en waarde verticaal gecentreerd, groter cijferfont (38 px Guition, 22 px CYD). Een te lange waarde krijgt puntjes; de eenheid staat altijd rechts naast het getal.
- Guition: de backlight dimt naar standby via de LEDC-hardwarefader in 1,5 s en wordt in 80 ms wakker. Schermherteken onderbreekt de overgang niet meer, dus geen schokkerig uitfaden.
- Installeer de nieuwe schermfirmware via ESP Screens; geen appupdate of gewijzigde configuratie nodig.

## Firmware 0.2.12 (app 0.2.11 blijft bruikbaar)

- Switches herkennen nu een aan/uit-terugmelding met ongewijzigde attributen. Daarmee verdwijnt de onterechte wachttijd van zes seconden. Na HA-bevestiging geldt voor switches slechts 150 ms minimale feedback.
- Uitgeschakelde switches hebben een grijs icoon en een grijze icoonachtergrond. Een zelfgekozen pastel tegelachtergrond blijft behouden.
- Mini-sliders behouden een compact domeinicoon naast titel en waarde. CYD centreert iconen en tekst verticaal; de renderdiagnose controleert centrering en vrije ruimte boven de slider.
- Lang indrukken op een switch of input_boolean opent een grote native LVGL-schakelaar met de echte HA-status, ook op CYD. Openen verstuurt geen opdracht.
- Installeer de nieuwe schermfirmware via ESP Screens; geen appupdate of gewijzigde configuratie nodig.

## 0.2.11

- Compact raster voor de pastelkleuren in de tegeleditor; voorkomt dat algemene formulierstijlen het palet in een lange kolom zetten. Alleen de app verandert; schermfirmware 0.2.10 blijft actueel.

## 0.2.10

- Kies per tegel een pastel achtergrond: rood, oranje, geel, groen, mint, blauw, paars, roze of grijs. Standaard herstelt de normale kleuren. De keuze is zichtbaar in het schermvoorbeeld en werkt met donkere tekst op Guition én CYD.
- Tegelkleuren blijven behouden bij appupdates en bij opslaan vanuit een oudere beheerpagina. Installeer firmware 0.2.10 op het scherm om de kleuren weer te geven.
- README en Easy Setup beschrijven de huidige één-app-installatie, twintig tegels, inspector, rotatie en updates via de ingebouwde ESPHome-CLI.

## 0.2.9

- Guition: kies 0°, 90°, 180° of 270° onder Scherminstellingen. Na opslaan draait de interface direct mee, inclusief touch. De hoek blijft bewaard na herstart; een nieuwe flash per hoek is niet nodig.
- Installeer eerst Guition-firmware 0.2.9 voor deze optie. De CYD behoudt zijn vaste oriëntatie en kalibratie.

## 0.2.8

- Firmware: vaste tekstbreedtes voor nette afkapping met puntjes. Ondertitels gebruiken de beschikbare ruimte, ook bij korte titels.
- Firmware: op de CYD staat de mini-slider onder beide tekstregels, met minder verticale padding. Zonder icoon krijgen de tekstregels de volledige kaartbreedte.
- De renderdiagnose controleert nu ook de daadwerkelijke tekst- en slidercoördinaten. Installeer de schermfirmware om deze verbeteringen te gebruiken; bestaande tegels, instellingen en sleutels blijven behouden.

## 0.2.7

- Tot twintig tegels op vier vaste pagina’s; installeer eerst firmware 0.2.7. Zes tegelvakken worden hergebruikt om geheugen te sparen.
- Optionele LVGL-swipes tussen pagina’s via scherminstellingen. Sliders en detailmenu’s wisselen niet van pagina; na een swipe wordt de aanraking geconsumeerd.

## 0.2.6

- Nieuw scherm hergebruikt bestaande ESPHome-wifi-secrets automatisch. De wizard vraagt alleen wifi als secrets.yaml nog niet bestaat. Ontbrekende wifi-sleutels in een bestaand bestand worden gemeld zonder het bestand te overschrijven.

## 0.2.5

- Firmware: een vacuumkaart openen toont de echte status, geen opdrachtmelding. Het statuslabel in de robotkaart blijft ook na een actie actueel.

## 0.2.4

- Visuele tegelkiezer met domeiniconen, zachte kleuren en een klikbaar schermvoorbeeld van zes tegels per pagina. Selecteer een tegel om de instellingen te openen; sleep tegels in het voorbeeld om te ordenen. Geen firmware-update nodig.

## 0.2.3

- Firmware: iets donkerdere Guition-achtergrond, zachte domeinkleuren op iconen en de echte HA-lampkleur in iconen/mini-sliders. Aangepaste Lovelace-thema- en kaartkleuren worden niet automatisch geïmporteerd.

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
