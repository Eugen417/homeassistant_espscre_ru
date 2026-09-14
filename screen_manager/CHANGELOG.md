## 0.2.37 (firmware 0.2.31)

- **Cheatsheet Alerts in ESP Screens.** De knop **Alerts** bovenaan opent een naslagpagina met alles over `show_alert`: per scherm de exacte actienaam (`esphome.<apparaatnaam>_show_alert`) met kopieerknop en of de firmware het al kan, een kant-en-klaar YAML-voorbeeld per scherm, alle zeven velden met uitleg, voorbeelden en de tekstlimieten per bord, alle iconen met glyph en naam (zoeken, tikken kopieert de naam), de kleuren als stalen, het gedrag (timeout, knop, knipperen, standby, vervangen) en het event `esphome.screen_alert` met een voorbeeldautomatisering die op de knop wacht. Home Assistant toont bij ESPHome-acties zelf geen uitleg of keuzelijsten; deze pagina vult dat gat.
- Geen nieuwe firmware; de doelversie blijft 0.2.31.

## 0.2.36 (firmware 0.2.31)

- **Alert vanuit een automatisering.** Elk scherm heeft nu de actie `esphome.<scherm>_show_alert` met `title`, `subtitle`, `icon`, `color`, `button_text`, `timeout` en `flash`. De kaart ligt op LVGL's toplaag over alles heen (pagina's, kaarten, de standby-laag), wekt het scherm en houdt de backlight op de normale helderheid tot iemand op de knop tikt (`button_text`, standaard Oké), of tot `timeout` seconden (0 is tot de knop; de knop sluit ook een alert met timeout direct). `flash: true` laat de backlight vier keer knipperen bij binnenkomst. Iconen zijn de namen uit de tegelkiezer (ook als `mdi:naam`, of als hex-codepoint van een meegecompileerd glyph); onbekend wordt `alert-outline`. Kleuren zijn de pasteltinten van de tegels; leeg is de witte kaart. Een nieuwe alert vervangt de huidige. Oké, timeout, vervangen en `dismiss_alert` melden zich als event `esphome.screen_alert` met `action`, `title` en `screen`.
- De actie `esphome.<scherm>_dismiss_alert` haalt een alert op afstand weg, bijvoorbeeld als de deur al open is.
- De iconenlijst schrijft nu ook `components/smart_display/tile_icon_names.h` (naam naar codepoint) via `tools/generate_icons.py`.
- Firmware 0.2.31 voor beide borden; de app biedt de update aan. Alleen de doelversie verandert in de app.

## 0.2.35 (firmware 0.2.30)

- **Snelle randveeg pakt nu ook in één keer (Guition).** Op 0.2.29 wisselde de veeg wel, maar ongeveer de helft van de snelle vegen eindigde met `randveeg niet gevuurd: 5 px naar binnen` terwijl de vinger duidelijk verder ging; elke aanraking duurde in de firmware bijna exact 160 ms en leverde maar één meting op. Een druk in de lege rand landde op de achtergrondcontainer en de pagina, en het thema geeft elk object een ingedrukte-stijl (45% dekking): bijna het hele scherm werd bij indrukken en loslaten opnieuw getekend, wat de touch-polling precies zo lang blokkeerde als een flick duurt. De pagina en de tegelcontainer nemen nu geen drukken meer aan (`LV_OBJ_FLAG_CLICKABLE` uit), dus geen hertekening en de metingen komen elke 20 ms door.
- Zolang een randveeg gewapend is, logt de firmware elke meting (`veeg id=0 st=2 x=.. y=..`) en de eerste druk meldt zijn contact-id, zodat de bemonstering uit het log te lezen is.
- Firmware 0.2.30 voor beide borden; de app biedt de update aan. Alleen de doelversie verandert in de app.

## 0.2.34 (firmware 0.2.29)

- **Randveeg werkte in 0.2.32 en 0.2.33 helemaal niet, hersteld.** De veeg was daar op de pointer-events van LVGL gezet, maar de LVGL 9.5.0 die ESPHome meelevert stuurt het invoerapparaat wel PRESSED en RELEASED maar geen PRESSING, dus de veeg kreeg nooit positie-updates (live gezien op Studio 1: wel `GT911 press`, verder niets). De Guition volgt de veeg weer via ESPHome's touchscreen-triggers, zoals in 0.2.24 (dat werkte live zodra de kaartfix uit 0.2.32 erbij zat), met de eigen rotatiemapping gelijk aan die van ESPHome's LVGL-component, de 45°-regel, `end()` bij loslaten (een niet-afgemaakte veeg kan daardoor niet meer bij de volgende aanraking afgaan) en de logregels `randveeg genegeerd: …` en `randveeg niet gevuurd: …`.
- Firmware 0.2.29 voor beide borden; de app biedt de update aan. Alleen de doelversie verandert in de app.

## 0.2.33 (firmware 0.2.28)

- **Guition: loslaten binnen de tegel telt.** De verplaatsingsgrens voor een tik staat op de Guition uit (`TOUCH_MOVE_LIMIT_PX: "0"`); LVGL beslist, zoals iOS: laat je los binnen de tegel waarop je begon, dan is het een tik, verlaat je vinger de tegel, dan niet. Een stevige druk die een centimeter verschoof (73 px op Studio 1) werd anders nog geweigerd. De randveeg neemt zijn eigen tik weg en sliders vangen hun eigen sleep, dus de grens had daar geen taak meer. De CYD houdt zijn 56 px, omdat het resistieve paneel kan springen.
- Firmware 0.2.28 voor beide borden; de app biedt de update aan. Alleen de doelversie verandert in de app.

## 0.2.32 (firmware 0.2.27)

- **Randveeg die nooit pakte, opgelost.** Live meegelezen op een Guition: de veeg werd herkend maar geblokkeerd door de voorwaarde "kaart open". De lichtkaart, klimaatkaart en stofzuigerkaart wisten bij sluiten (kruisje, tik naast de kaart) en bij wakker worden uit standby de interne "actieve kaart" niet, dus na één keer een kaart openen bleef vegen stil geblokkeerd tot de indeling veranderde. Dat gold ook voor de oude veeg over het hele scherm en voor de CYD. Beide borden sluiten kaarten nu via één script (`close_cards`) dat alles verbergt én die toestand wist; wakker worden gebruikt hetzelfde pad.
- **Randveeg op LVGL's eigen manier.** De Guition volgt de veeg nu via de pointer-events van LVGL 9 (`lv_indev_add_event_cb`, PRESSED/PRESSING) in plaats van ESPHome's touchscreen-`on_update`: de coördinaten komen al gedraaid van LVGL (geen eigen rotatiemapping meer), en `lv_indev_wait_release` wordt vanuit LVGL zelf aangeroepen. De richtingseis is versoepeld van "twee keer zo horizontaal" naar "meer zijwaarts dan verticaal" (45°), zodat een schuine duimveeg vanaf rechts ook in één keer pakt.
- **Log zegt waarom.** Een herkende maar geblokkeerde randveeg meldt de reden (`randveeg genegeerd: kaart open`), en een randveeg die zonder paginawissel eindigt meldt hoe ver de vinger kwam (`randveeg niet gevuurd: 28 px naar binnen, 35 px verticaal`), naast het bestaande `randveeg: pagina 0 -> 1`.
- Firmware 0.2.27 voor beide borden; de app biedt de update aan. Alleen de doelversie verandert in de app.

## 0.2.31 (firmware 0.2.26)

- **Vrij slepen, met lege plekken**: de editor werkt met vaste plekken in plaats van een volgorde die steeds wordt aangeschoven. Elke tegel heeft zijn eigen plek (kolom, rij, pagina) en die verandert alleen als jij hem versleept. Lege plekken zijn gewoon lege plekken: naast een enkele tegel, midden op een pagina, waar je wilt; ze blijven staan zodat je kunt ordenen. Sleep een tegel op een lege plek en hij staat daar. Sleep hem op een andere tegel en die twee wisselen: de ander neemt de plek die vrijkwam, of anders de dichtstbijzijnde vrije plek. Alle overige tegels blijven waar ze staan. Tijdens het slepen laat het voorbeeld al zien waar alles terechtkomt; loslaten bevestigt precies dat, loslaten buiten het voorbeeld verandert niets.
- **Naar een volgende pagina slepen**, ook als de vorige nog niet vol is: onder de laatste pagina staat tijdens het slepen een lege pagina klaar. Met **Pagina toevoegen** maak je zelf een lege pagina, die ook bewaard blijft; een lege pagina heeft **Pagina weghalen** (de pagina's erna schuiven op). Maximaal acht pagina's.
- **Lege plek aanklikken**: klik op een lege plek en de volgende tegel uit de kiezer komt daar ("Volgende tegel komt hier"); zonder keuze vult toevoegen de eerste vrije plek. Een tegel die dubbelbreed wordt houdt zijn rij als de plek ernaast vrij is en gaat anders naar de dichtstbijzijnde vrije rij; de buurtegel blijft staan. Dubbelbreed terug naar normaal laat de rechterplek leeg.
- **Geen tekstselectie meer tijdens het slepen**, en een sleep die snel begint start toch (de muis blijft aan de kaart gekoppeld tot de sleep loopt). Pijltjestoetsen verplaatsen een gefocuste tegel per plek; Enter opent de instellingen.
- Firmware 0.2.26 tekent de tegels op precies die plekken: lege plekken blijven leeg en een lege pagina blijft een pagina. Oudere firmware negeert de nieuwe velden en schuift de tegels aan tot de eerste vrije plek, zoals voorheen; de editor meldt dat onder het voorbeeld zolang zo'n scherm een indeling met gaten heeft. Bestaande indelingen krijgen bij het laden de plekken die ze al hadden, er verandert niets op het scherm.
- Opslag en protocol (voor ontwikkelaars): `tiles[].slot` (0–47, absolute plek: pagina × 6 + rij × 2 + kolom; dubbelbreed altijd op een even plek) en `pages` (1–8) zijn additief in opslagversie 1; het layoutbericht krijgt `slots` en `pages`. Firmware 0.2.26 voor beide borden; de app biedt de update aan.

## 0.2.30 (firmware 0.2.25)

- **Groen bolletje voor online schermen**: in de lijst met schermen is het bolletje voor **Online** nu groen, zodat je in één oogopslag ziet welke schermen bereikbaar zijn. **Offline** blijft een grijs open rondje.
- Firmware 0.2.25 voor beide borden; de app biedt de update aan. Inhoudelijk verandert er in de firmware niets, alleen het versienummer.

## 0.2.29 (firmware 0.2.24)

- **Vegen vanaf de zijrand op de Guition**: met **Vegen tussen pagina's** aan wissel je van pagina door vanaf de linker- of rechterrand naar binnen te vegen, zoals terug-vegen op een telefoon. Vanaf rechts naar links is volgende, vanaf links naar rechts vorige. Langzaam of snel maakt niet uit: de veeg begint in een band van 32 px (circa 5 mm) langs de rand en telt na 40 px (circa 6 mm) naar binnen, duidelijk meer zijwaarts dan omhoog of omlaag (`EDGE_SWIPE_BAND_PX`, `EDGE_SWIPE_TRAVEL_PX`). Werkt in alle vier de rotaties. Een veeg die midden op het scherm begint doet niets meer, zodat tikken en slepen op tegels nooit per ongeluk van pagina wisselen; de tegel onder een randveeg krijgt geen tik. Het log meldt `randveeg: pagina 0 -> 1`.
- De CYD houdt de bestaande snelle veeg over het scherm (LVGL-gesture, firmware 0.2.7+); daar verandert niets, de firmware krijgt alleen het nieuwe versienummer.
- Firmware 0.2.24 voor beide borden; de app biedt de update aan. In de app is alleen de omschrijving van de instelling aangepast.

## 0.2.28 (firmware 0.2.23)

- **Tikken die niet doorkwamen**: de firmware gooide een tik weg zodra de vinger tijdens het drukken meer dan 18 px (nog geen 3 mm) van het eerste contactpunt afweek, ver onder LVGL's eigen veegdrempel en ook met "Vegen tussen pagina's" uit. Een vinger die platter wordt of iets rolt haalde dat al. Nu geldt per bord een grens van ongeveer één centimeter (`TOUCH_MOVE_LIMIT_PX`: Guition 67 px, CYD 56 px), gemeten als afstand tot een referentiepunt dat over de eerste vier metingen (circa 60 ms) settelt in plaats van tot het allereerste punt. Eindigt de vinger op een andere tegel, dan vangt LVGL dat nog steeds op (press lost).
- Alleen het contact dat de aanraking begon telt: een tweede vinger of een duim aan de rand geldt niet meer als verplaatsing (`touch.id` van de touchscreen-component).
- De minimale contactduur is per bord (`TOUCH_MIN_PRESS_MS`): 60 ms op de resistieve CYD (contactdender), 20 ms op de capacitieve Guition, waar een lichte snelle tik anders verloren ging.
- Elke geweigerde tik staat nu in het ESPHome-log (tag `touch`, niveau INFO) met de reden: `verplaatst 73 px (grens 67)`, `te kort (12 ms, minimaal 20)`, `al verwerkt in dit contact` of `dezelfde knop binnen de dendertijd`, zodat een gemiste aanraking uit het log te lezen is.
- Firmware 0.2.23 voor beide borden; de app biedt de update aan. Alleen firmware verandert; de app-kant is ongewijzigd behalve de doelversie.

## 0.2.27 (firmware 0.2.22 blijft actueel)

- **Nieuw scherm in één venster**: bord, naam, USB-poort, **Installeren**. Het venster maakt het profiel met unieke API- en OTA-sleutels in de ESPHome-map, bouwt de firmware en schrijft die via USB, met het ESPHome-log en de fase (bouwen, schrijven) in hetzelfde venster. Na afloop staat de API-sleutel klaar met een kopieerknop plus de koppelstappen voor Home Assistant; mislukt de build, dan staat het log open en is er **Opnieuw proberen**. De apparaatnaam volgt uit de naam (`Keuken` → `keuken`) en is aan te passen. Zonder aangesloten scherm kun je alleen het profiel bewaren; het staat dan in dezelfde map als ESPHome Device Builder. Elk scherm krijgt zijn eigen profiel; het venster zegt dat nu ook.
- **Waar is mijn scherm?** De koppeling gebeurt in Home Assistant zelf, buiten ESP Screens. Het klaar-scherm van het venster zegt dat nu expliciet, met een knop **Open Apparaten & diensten** die je daarheen brengt. Zolang een ESP Screens-profiel nog niet in Home Assistant staat, toont **Mijn schermen** er een kaart voor ("geïnstalleerd, maar nog niet in Home Assistant", of "nog niet geflasht"), met dezelfde knop, **Kopieer API-sleutel** en de herinnering om bij Configureren "Allow the device to perform Home Assistant actions" aan te zetten (anders ziet het scherm alles, maar bedient het niets). De kaart verdwijnt zodra het scherm in de lijst staat. `/api/inventory` levert daarvoor `pending` (profielen die het bordpakket van dit project gebruiken zonder gekoppeld scherm).
- **Wifi zonder handwerk**: ontbreken `wifi_ssid` of `wifi_password` in de ESPHome `secrets.yaml`, of bestaat het bestand niet, dan vraagt het venster ze en zet ESP Screens alleen de ontbrekende regels in het bestand; commentaar en andere secrets blijven staan (de wizard weigerde eerder bij een bestaand bestand zonder wifi). Een ongeldig `secrets.yaml` wordt nog steeds niet aangeraakt.
- Kopiëren en downloaden van de installatie-YAML is vervallen (`POST /api/install` bestaat niet meer): het profiel staat al in de ESPHome-map. `POST /api/firmware/profiles` accepteert `target` (USB-poort) en start dan direct de build en flash; poort en vrije bouwslot worden gecontroleerd vóór er iets wordt geschreven, en het antwoord bevat `api_key`. Het firmwarejob-object meldt de lopende fase in `stage`.
- Alleen de app verandert; firmware 0.2.22 blijft actueel.

## 0.2.26 (firmware 0.2.22)

- **"HA niet verbonden" om de twee minuten opgelost**: sinds 0.2.20 herhaalt de app de indeling elke 120 s, maar de firmware verwachtte binnen 95 s een bericht (gemaakt voor de oude 25 s). Op een rustig scherm stonden alle tegels daardoor 25 tot 45 s per ronde op "Niet beschikbaar". De firmware gebruikt nu ESPHome's eigen API-verbindingsstatus voor "HA niet verbonden" (direct bij wegvallen en terugkeren van Home Assistant) en bewaakt de gegevensstroom apart met het interval dat de app zelf declareert: het layoutbericht bevat `keepalive` (seconden). Pas na twee gemiste rondes plus marge meldt het scherm "ESP Screens niet actief". Zonder het veld (oudere app) rekent de firmware met 120 s. Firmware 0.2.22 voor beide borden; firmware tot 0.2.21 negeert het veld en houdt zijn 95 s.

## 0.2.25 (firmware 0.2.21)

- **− / + in één beweging**: snel drie keer tikken telt drie stappen (van 20 naar 17); de touch-guard hield een tweede tik op dezelfde knop binnen 600 ms tegen. De − / + knoppen gebruiken nu een eigen korte guard (150 ms, alleen tegen stuiteren) en stappen ook door zolang je ze vasthoudt (drie per seconde). Na 700 ms rust gaat nog steeds één opdracht naar Home Assistant.

## 0.2.24 (firmware 0.2.20)

- **Lichter en zuiniger**: de dag- en uurvoorspelling zitten alleen nog in het geheugen van weertegels (was 832 bytes in elk van de twintig tegels, ruim 16 KB op de CYD zonder PSRAM). De mini-slider onderin een tegel (**Kleine slider**) heeft nu dezelfde zichtbare witte greep als de nieuwe schuiven. De licht-, ventilator- en zonweringkaart op de CYD gebruiken het lichte palet van de Guition (de laatste rest van het donkere thema).
- **Wifi zonder modem-slaap** (`power_save_mode: none`) in de installatie-YAML van de wizard en de bordprofielen: statusupdates, tikacties en OTA wachten niet meer op een wifi-beacon. Bestaande schermen: voeg de regel zelf toe onder `wifi:` in je ESPHome-YAML.
- Doorlichting van de rendering: LVGL 9.5 tekent partieel en slaat ongewijzigde posities en maten al over; de winst zat in de stijlzetters (0.2.23) en in niet meer herrenderen tijdens *bezig*. Een paginawissel tekent bewust twee keer (skelet, dan inhoud). Verder geen structurele last gevonden; zie docs/TEST_RESULTS_0224.md.

## 0.2.23 (firmware 0.2.19)

- **Directe bediening op dubbelbrede tegels**, zoals de entiteitsrijen in Home Assistant: rechts op de kaart staan knoppen of een schuif, links blijven icoon, naam en status. Per domein kies je in het tegelpaneel onder **Directe bediening op de tegel** welke set de tegel toont:
  - klimaat: **Temperatuur − / +** (de gewenste temperatuur in een pill; tikken past direct aan, na een korte pauze gaat één opdracht naar HA) of **Uit, verwarmen, koelen** (maximaal drie modusknoppen uit `hvac_modes`, de actieve gekleurd);
  - schakelaar, input_boolean, lamp en ventilator: **Aan/uit-schakelaar** (toggle die direct omklapt); lamp ook **Helderheidsschuif**, ventilator ook **Snelheidsschuif**;
  - stofzuiger: **Start, stop, naar dock** (start wordt pauze tijdens het schoonmaken; dock en stop grijs als ze niet kunnen);
  - zonwering: **Open, stop, dicht** (pijlen horizontaal voor gordijnen, deuren, poorten en zonneschermen; open/dicht grijs aan het eind van de slag; stop alleen als het apparaat het kan) of **Positieschuif**;
  - mediaspeler: **Volume en dempen** (schuif met witte greep en dempknop) of **Vorige, play/pauze, volgende**;
  - getal: **Waarde − / +** of **Schuif**; keuzelijst: **Vorige / volgende keuze**; kookwekker: **Start/pauze en annuleren**; scène, script en knop: één knop **Activeren**, **Uitvoeren** of **Indrukken**.
- Zonder keuze toont een dubbelbrede tegel van zo'n domein de eerste set; **Geen** houdt de gewone kaart. Enkele tegels veranderen niet. De statusregel volgt HA: `Koelen · 21.5°`, `Open · 80%`, `TV · 17%`.
- De mockup in de editor toont de gekozen bediening in het klein; de Inspector meldt de gekozen set. De add-on stuurt alleen de werkelijk getoonde set mee (`o.controls`), plus `device_class`, `hvac_action` en `is_volume_muted`. Firmware 0.2.18 en ouder negeert het veld; het paneel meldt dat.
- Firmware 0.2.19: het paneel wordt alleen opgebouwd voor tegels die het tonen en per set hergebruikt; de render-selftest controleert dat knoppen binnen de kaart en rechts van de tekst blijven. De drie icoonfonts bevatten 18 extra bedieningsglyphs (CYD +3 KB, Guition +5 KB) en het middenpunt `·` in de tekstfonts.
- **Bezig-status als spinner**: een tegel die op Home Assistant wacht krijgt een licht-witte laag met een klein draaiend cirkeltje in plaats van de blauwe voortgangsbalk en de tekst "Bezig...". De laag vangt tikken op zolang de opdracht loopt.
- **Weerkaart** (bediening openen op een weertegel), in twee kaarten: *nu* met groot icoon, temperatuur, omschrijving en een gedempte regel met gevoelstemperatuur, luchtvochtigheid en wind, daaronder de komende zes uren (tijd, icoon, temperatuur, regenkans in blauw); en *Komende dagen* met per dag icoon, omschrijving, regen met druppel (kans en millimeters) en de hoogste temperatuur vet naast de laagste gedempt. De add-on haalt naast de dagvoorspelling nu ook de uurvoorspelling op (`weather.get_forecasts` type `hourly`, elk half uur ververst) en stuurt `x.hours` plus regen (`p` kans, `r` mm) per dag en uur mee. Zonder uurvoorspelling blijft de kaart bij de dagen.
- **Klimaat**: het vinkje rechtsboven in de klimaatkaart is een **aan/uit-knop** geworden (`climate.turn_on`/`turn_off`, licht op als het apparaat draait); het kruisje sluit nog steeds. De moduskiezer toont naast de HVAC-modi ook de **ventilatorstanden** en **zwenkstanden** van het apparaat (`fan_modes`, `swing_modes`, maximaal vier per rij; Nederlands waar bekend). Bij aantikken van een klimaattegel kun je nu ook **Aan / uit** kiezen (`climate.toggle`).
- **Scènes, scripts en knoppen** tonen niet langer "Uit" maar wanneer ze voor het laatst liepen: `Laatst 14:32`, `Gisteren 14:32` of `Laatst 13 sep`, en `Bezig...` zolang een script draait; `Nog niet gestart` als het nog nooit liep. De add-on stuurt daarvoor `x.last` (unix-tijd uit `last_triggered` of de tijdstempel-status).
- **Moduskiezer van de klimaatkaart** opnieuw ingedeeld: titel *Modus*, de kiezer op het schermgrijs met witte chips met randje (contrast), Nederlandse HVAC-chips drie per rij, de actieve chip in het accentblauw met witte tekst, daaronder de rijen *Ventilator* en *Zwenken* met de standen zoals het apparaat ze meldt (alleen een hoofdletter, geen vertaaltabel: werkt met elk apparaat). Het overbodige vinkje is weg; een chip past de stand toe en sluit, het kruisje sluit. *Nu:* en *Gewenst* vervangen de Engelse labels in de kaart.
- **Lichter tekenen**: de firmware zet lettertypen, uitlijning, padding en lijndikte alleen nog als ze echt veranderen (elke `lv_obj_set_style_*` maakt anders het hele object ongeldig), en herrendert tijdens *bezig* niet meer elke 250 ms de hele pagina. Een tikkende klokkaart of een draaiende spinner tekent zo alleen nog zichzelf. Het donkere thema is uit de runtime-code verwijderd; het lichte palet is het enige palet.
- Getallen boven een miljoen gaan niet mee als weergavewaarde, behalve `supported_features` (een bitveld; mediaspelers zitten boven de 8 miljoen), anders hadden Sonos-tegels geen volume- of playbackknoppen.

## 0.2.22 (firmware 0.2.18 blijft actueel)

- De eventstream stuurt het eerste event direct bij openen in plaats van na 3 s. Gemeten op een Home Assistant Yellow na 0.2.21: `/api/inventory` 50 ms (was 7,2 s), lichte poll 2,6 KB, CPU in rust 0,5% van één core (was 8,7%). Alleen de app verandert.

## 0.2.21 (firmware 0.2.18 blijft actueel)

- **Live updates zonder pollen**: de pagina luistert op `/api/events` (server-sent events). De add-on stuurt schermstatus en updatestatus zodra de synchronisatielus of een opslag iets verandert, en controleert elke 3 s op wijzigingen tijdens een firmware-update. Zolang de stream open is pollt de pagina niet meer; valt de stream weg, dan neemt de poll van 10 s het over. De volledige lijst met entiteiten, achtergronden en iconen wordt nog elke 5 minuten ververst.
- Het log meldt bij het wegvallen van de Home Assistant-verbinding hoe lang die stond, de websocket close-code en de fout, ook wanneer de verbinding zonder foutmelding sluit. Het add-on-log op de Yellow toonde herhaalde "Home Assistant verbonden"-regels zonder waarschuwing ervoor; met deze regels is de oorzaak bij de volgende keer uit het log te halen.
- Alleen de app verandert; firmware 0.2.18 blijft actueel.

## 0.2.20 (firmware 0.2.18 blijft actueel)

- **Minder verbruik in rust**: de synchronisatielus wordt alleen nog wakker voor entiteiten die op een indeling staan of bij een scherm horen (Tegelinstellingen, Schermfirmware, Apparaatnaam, IP-adres, Guition schermtype), niet meer bij elke `state_changed` in Home Assistant.
- De entity-, device- en area-registry (circa 1 MB JSON) wordt niet meer elke 30 s opgehaald, maar bij een `*_registry_updated`-event van Home Assistant (1 s gedebounced) en als vangnet elke 10 minuten.
- De volledige keepalive naar de schermen loopt elke 2 minuten in plaats van elke 25 s. Een scherm dat offline en weer online komt krijgt de volledige indeling direct, zoals voorheen.
- **Lichte poll**: de pagina haalt elke 10 s alleen nog schermen en updatestatus op (`/api/inventory?light=1`, enkele KB) en de volledige lijst met entiteiten, achtergronden en iconen bij openen, bij terugkeer naar het tabblad en elke 5 minuten.
- Alleen de app verandert; firmware 0.2.18 blijft actueel.

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
