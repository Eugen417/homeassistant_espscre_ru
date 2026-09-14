# Updates publiceren zonder gebruikersconfiguratie te vervangen

## Indeling van de code en gegevens

- `main` is de distributiebranch voor de app **en beide** firmwarepakketten.
  De Guition-branch blijft beschikbaar voor bordwerk; publiceer gedeelde releases
  altijd ook naar main.
- `screen_manager/config.yaml` bevat de appversie. Verhoog die bij iedere apprelease.
  Git push alleen is niet genoeg om een bestaande app een update aan te bieden.
- De appimage bevat uitsluitend code. Indelingen leven in `/data/screens.json`
  (`version: 1`, `screens: {...}`). Een update/herbuild behoudt deze volumegegevens.
- De eigen ESPHome-YAML bevat naam, wifi-verwijzingen en unieke API/OTA-sleutels.
  Gedeelde pakketten bevatten geen secrets, vaste eigenaarsentiteiten of wifi.
- CYD-kalibratie staat in ESP32-preferences. Behoud de preference-key, structuur
  en partitie-indeling, of schrijf een expliciete migratie.
- Het tegelprotocol gebruikt `v: 1`. Houd oudere velden en domeinen bruikbaar.
  Een gewijzigde opslagversie moet een geteste migratie met back-up krijgen.
  De app weigert onbekende versies in plaats van de data leeg te overschrijven.

## Voor elke release

1. Pas de bronprofielen en gedeelde componenten aan. Draai
   `python3 tools/generate_packages.py`; bewerk `packages/*.yaml` nooit rechtstreeks.
2. Draai alle Python-tests met aiohttp geïnstalleerd, alle `tests/*.cpp`, de
   generator met `--check`, en compileer beide Easy Setup-profielen plus de
   bestaande handmatige profielen. Doe dat na elkaar: profielen met dezelfde
   `DEVICE_NAME` delen één build-map, en een parallelle build laat een upload
   het verkeerde `firmware.bin` kiezen. Controleer dat er geen secrets in Git staan.
3. Test appstart, opslaan, opnieuw starten/updaten met bestaande indelingen,
   opnieuw verbinden met HA en een ESP-herstart. Test een nieuwe kaart op echte
   hardware. Een goede build vervangt fysieke touchacceptatie niet.
4. Verhoog appversie en firmwareprojectversie; schrijf CHANGELOG en concrete
   testresultaten. Publiceer alleen compatibele wijzigingen rechtstreeks op main.
5. Commit en push main. Maak een onveranderlijke tag `screens-vX.Y.Z` van dezelfde
   commit. Werk de Guition-branch bij. Test de remote YAML in een lege map:
   alle componenten/fonts moeten via GitHub kunnen worden opgehaald.
6. De gebruiker controleert de appwinkel op updates en update ESP Screen Manager.
   Voor nieuwe schermfuncties: bestaand ESPHome-apparaat → Install → Wirelessly.
   De bestaande YAML blijft staan; `refresh: 0s` haalt bij elke build actuele code.

## Lokale ontwikkeling

Kopieer alleen `screen_manager/` naar de gedeelde `addons/esp_screen_manager/`.
Herlaad de appwinkel, installeer de lokale versie en bouw na codewijzigingen opnieuw.
Een lokale testversie heeft een andere add-onidentiteit dan de GitHub-versie; de
gegevens verhuizen niet automatisch. Test voor de uiteindelijke installatie de
GitHub-versie en zet de lokale versie uit om twee schrijvers te voorkomen.

Voor backendtests op een ontwikkelcomputer:

```sh
python3 -m venv .venv-portal
.venv-portal/bin/pip install aiohttp PyYAML
.venv-portal/bin/python -m unittest discover -s tests
```

Een tijdelijke ontwikkelserver ondersteunt `SCREEN_DEV=1`, `HA_API` (eindigt op
`/api`), `HA_TOKEN_FILE` en `SCREEN_DATA`. Hij bindt alleen op localhost. Zet een
token nooit in broncode, URL's of Git. Productie gebruikt Supervisor en accepteert
alleen het Ingress-proxyadres; er is geen extra publieke poort.


### Instellingencompatibiliteit 0.1.2

`settings` is een optioneel object binnen een schermindeling en binnen het bestaande
`v: 1, op: layout`-bericht. Een ontbrekend object behoudt eerder gedrag. Oude
firmware negeert dit extra veld en blijft tegels ontvangen. De beheerpagina meldt
dat nieuwe firmware nodig is. Een oude browser die alleen tegels opslaat, wist
opgeslagen instellingen niet. Onbekende instellingen worden geweigerd.

Firmware gebruikt een afzonderlijke preferences-key `0x53435231` met een vaste
versie-1 structuur (elf int32-velden en een uint32-versie). Wijzig key, structuur
of versie niet zonder migratie. Alleen gewijzigde instellingen worden opgeslagen;
periodieke herhaling veroorzaakt geen flashwrites en geen nieuwe idle-timer.

### Compatibiliteit 0.2.0

Opslagversie en tegelprotocol blijven 1. Optionele `tiles[].options`, `o` in
statusberichten, `history` en `inbox` in het layoutbericht zijn additief. Een
oudere browser die opties niet meestuurt behoudt de bestaande opties voor dezelfde
entiteit. Oudere firmware negeert extra velden. Nieuwe entiteitsdomeinen vereisen
wel firmware 0.2.0; bij terugrollen naar een oudere app eerst de gegevensback-up
herstellen, omdat die app nieuwe domeinen nog niet kan laden. Geen oude opslag
stilzwijgend overschrijven. Bestaande preference-keys blijven ongewijzigd.

De app gebruikt nu de officiële ESPHome-container, mapped `homeassistant_config`
naar `/homeassistant` en vraagt UART/USB voor de door de gebruiker gekozen poort.
Config/secrets blijven in de eigen HA-configmap; builds in `/data`. App-updates
vervangen deze mappen niet. Native getalinstellingen melden wijzigingen via
`esphome.screen_setting`; de manager valideert inbox, sleutel en waarde.

De ingebouwde CLI bewaart opnieuw te downloaden caches onder `/data/build` en
`/data/platformio`. Alleen deze mappen zijn uitgesloten van appback-ups via
`backup_exclude`. `/data/screens.json` en eigen ESPHome-configuraties blijven
back-upgegevens; sluit nooit `/data` als geheel uit.

### Compatibiliteit 0.2.7

Twintig runtime-tegels vereisen firmware 0.2.7+. De manager controleert die versie
voor opslaan en verzenden van grotere indelingen. Bestaande indelingen blijven
behouden bij oudere firmware. Oude appversies ondersteunen niet meer dan tien;
breng de indeling eerst terug tot tien of herstel hun gegevensback-up bij rollback.
`settings.swipe_pages` is additief in opslagversie 1. Op de draad staat het buiten
het ongewijzigde elfvelden-settingsobject. De bestaande preferences blijven gelijk;
alleen swipe gebruikt een nieuwe eigen uint32-key `0x53575031`, standaard uit.

### Compatibiliteit 0.2.9

Guition-rotatie is additief als `settings.rotation` in appdata, uitsluitend
0/90/180/270. Oude beheerpagina's die het veld weglaten behouden de opgeslagen
hoek. Op de draad staat `rotation` naast het ongewijzigde elfvelden-settingsobject.
De firmware bewaart de hoek als afzonderlijke uint32 op key `0x524F5431`; de
bestaande Settings-structuur en CYD-kalibratie blijven ongewijzigd. Alleen de
Guition activeert de rotatiecallback. De diagnostische entity `Guition schermtype`
maakt deze capability herkenbaar, ook als het scherm offline of hernoemd is.

### Compatibiliteit 0.2.10

`tiles[].options.background` is een optionele paletnaam binnen opslagversie 1.
Op de draad staat dit in het bestaande `o`-object. Oudere firmware negeert het
veld; nieuwe firmware valt bij onbekende namen terug op de normale kleuren.
De app accepteert uitsluitend het lichte palet uit `TILE_BACKGROUNDS`. Een oude
editor die `background` weglaat behoudt de opgeslagen kleur voor dezelfde entity;
expliciet `auto` herstelt de standaard. Geen gewijzigde voorkeurstructuur of
sleutels, geen nieuwe firmwareflash nodig voor latere kleurkeuzes.

Een oude apprelease van vóór 0.2.10 kent dit optieveld niet. Bij terugrollen van
de app herstel je daarom de bijbehorende gegevensback-up; wis of negeer geen
onbekende opties om een oud opslagbestand toch te openen.

### Compatibiliteit app 0.2.15 / firmware 0.2.16

Opslagversie en tegelprotocol blijven 1. `tiles[].options.background` krijgt de
extra paletnaam `none` ("Geen"): geen kaart achter de tegelinhoud. Oudere
firmware kent de naam niet en valt terug op de normale kaart; omdat dat voor de
gebruiker onzichtbaar misgaat, stuurt de manager zo'n indeling pas na firmware
0.2.16 (`min_firmware`) en bewaart haar ondertussen. De firmware verbergt
uitsluitend vulling en rand (`bg_opa`/`border_opa`); maten, padding en de
pressed-feedback op de PRESSED-state blijven gelijk. De analoge klok wijzigt
alleen de tekening (streepjes, cijfers, kalenderblok op enkele tegels); er is
geen nieuw veld voor nodig. Geen gewijzigde preferences of sleutels.

### Compatibiliteit 0.2.16 / firmware 0.2.17

`FIRMWARE_VERSION` in `screen_manager/app/core.py` is de firmware die bij deze
app hoort; `tests/test_updates.py` eist dat hij gelijk is aan
`SCREEN_FIRMWARE_VERSION` in beide bordprofielen en pakketten. Verhoog ze samen.
Een scherm met een lagere `Schermfirmware` krijgt een update-aanbod; een build
haalt `main`, dus publiceer firmware en app in dezelfde commit.

Firmware 0.2.17 voegt de diagnostische text sensors `Apparaatnaam`
(`${DEVICE_NAME}`) en `IP-adres` (`wifi_info`) toe. De app koppelt een scherm
via `Apparaatnaam` aan het profiel met dezelfde `esphome.name`; oudere firmware
valt terug op één profiel met gelijke `friendly_name` en een handmatig ingevuld
adres. Hernoem deze sensors niet zonder `core.discover` aan te passen.

Updatestatus staat in `/data/updates.json` (`version: 1`: `auto`, `hosts`,
`results`, `last_round`), los van `screens.json`. Onbekende versies worden
geweigerd. De ronde draait één scherm tegelijk, wacht op `Schermfirmware >=`
doelversie en één minuut stabiliteit, en stopt bij de eerste fout. Alleen de
nachtelijke ronde schrijft een `persistent_notification` in HA.

### Compatibiliteit 0.2.18 / firmware 0.2.18

Opslagversie en tegelprotocol blijven 1. `tiles[].options.icon` is additief:
`auto` of een naam uit `screen_manager/app/tile_icons.py`. Op de draad stuurt de
manager in `o.icon` alleen de opgeloste codepoint in hex (`F06B5`): de gekozen
naam, of bij `auto` het `mdi:`-icoon uit de HA-attributen als dat in de set zit.
Oudere firmware negeert het veld; nieuwe firmware controleert of de glyph in de
font zit en valt anders terug op het domeinicoon. Daarom geen `min_firmware`.
Een oude editor die `icon` weglaat behoudt de opgeslagen keuze.

`tile_icons.py` is de enige lijst. `tools/generate_icons.py` schrijft de glyphs
als YAML-anker in de drie MDI-fonts van beide bordprofielen en bouwt
`static/tile-icons.woff` voor de editor (met left bearing gelijk aan xMin, anders
staan iconen in de browser uit het midden); draai daarna `generate_packages.py`.
De substituties `MDI_GLYPH_*` zijn vervallen: `TILEn_ICON` in handmatige
profielen moet uit de set komen. Geen gewijzigde preferences of sleutels.

### Compatibiliteit 0.2.34 / firmware 0.2.29

Alleen firmware. LVGL 9.5.0's `send_event()` in `lv_indev.c` stuurt het invoerapparaat
PRESSED, RELEASED, CLICKED, LONG_PRESSED en KEY, maar geen PRESSING; een
`lv_indev_add_event_cb` krijgt dus geen positie-updates tijdens een druk. `cyd::EdgeSwipe`
werkt daarom weer op de native touchpunten uit `on_touch`/`on_update`/`on_release`
(`configure(width, height, band, travel)`, `begin(x, y, rotation)` met ESPHome's
rotatiemapping, `update()`, `end()`), de indev-callback is weg. Verder ongewijzigd.

### Compatibiliteit 0.2.33 / firmware 0.2.28

Alleen firmware. `TouchGuard::configure(0, ...)` schakelt de verplaatsingsgrens uit
(`moved_` wordt dan nooit gezet; LVGL's press-lost bepaalt of een tik doorgaat). Het
Guition-profiel zet `TOUCH_MOVE_LIMIT_PX` op 0, de CYD houdt 56. Verder ongewijzigd.

### Compatibiliteit 0.2.32 / firmware 0.2.27

Alleen firmware. `cyd::EdgeSwipe` werkt op LVGL-pointercoördinaten: `configure(band, travel)`,
`begin(x, y, width)` met `lv_display_get_horizontal_resolution()`, `update()` met de 45°-regel,
plus `inward()`/`sideways()` voor het log. De Guition registreert één `lv_indev_add_event_cb`
(PRESSED/PRESSING/RELEASED) in `on_boot`; de touchscreen-triggers voeden alleen nog de
`TouchGuard`. Beide bordprofielen krijgen het script `close_cards` (roept
`runtime_tiles::dismiss()` aan, anders overlays verbergen en `active_entity` wissen); de
kruisjes en achtergrondtikken van de kaarten en `wake_display` gebruiken het. Protocol, opslag,
preferences en sleutels ongewijzigd.

### Compatibiliteit 0.2.31 / firmware 0.2.26

Opslagversie en tegelprotocol blijven 1. `tiles[].slot` is additief: de absolute plek
van een tegel (pagina × 6 + rij × 2 + kolom, 0–47, `MAX_SLOTS` in `core.py`); een
dubbelbrede tegel staat op een even plek en bedekt ook de plek rechts ervan. Lege plekken
zijn toegestaan. `validate_layout` eist plekken voor alle tegels of voor geen: zonder
plekken (oude editor, oude opslag) krijgt de indeling de plekken van de oude
volgorde-packing (`pack_slots`, hetzelfde als `pack()` in de firmware), zodat er op
het scherm niets verandert; met plekken worden overlap, oneven dubbelbreed en bereik
geweigerd en de tegels op plek gesorteerd. `save()` herhaalt de packing na het
terugzetten van opgeslagen opties (die kunnen een tegel verbreden). `pages` (1–8) is
een optioneel, additief veld: pagina's die de gebruiker leeg wil houden.

Op de draad krijgt het layoutbericht `slots` (één plek per entity, in dezelfde
volgorde) en `pages`. Firmware 0.2.26 (`Model::set_layout` met posities, `place()`
naast het oude `pack()`, `MAX_PAGES` 8) tekent de tegels op die plekken; een wijziging
van alleen plekken (`moved`) herplaatst de pagina's zonder tegelstatussen te wissen.
Oudere firmware negeert beide velden en pakt de entities in volgorde; omdat de
entities op plek gesorteerd zijn is dat de oude weergave zonder gaten, daarom geen
`min_firmware`. De editor meldt het (`has_gaps`/`hasGaps`) bij firmware < 0.2.26. Geen
gewijzigde preferences of sleutels.

### Compatibiliteit 0.2.30 / firmware 0.2.25

Alleen de editor: het statusbolletje in de schermlijst is een eigen `span.dot`
(groen met `.online`). Firmware krijgt alleen een nieuw versienummer; protocol,
opslag, preferences en sleutels ongewijzigd.

### Compatibiliteit 0.2.29 / firmware 0.2.24

Alleen firmware (Guition). `cyd::EdgeSwipe` (in `cyd_ui.h`, getest in
`test_cyd_ui.cpp`) vervangt op de Guition de LVGL-gesture op `home_page`: `begin()`
in `on_touch` met de LVGL-rotatie, `update()` in `on_update` voor het eerste contact;
bij een treffer `touch_guard.consume()`, `lv_indev_wait_release()` op alle indevs en
`show_tile_page`. Substituties `EDGE_SWIPE_BAND_PX`/`EDGE_SWIPE_TRAVEL_PX`;
`DISPLAY_W`/`DISPLAY_H` bepalen de rotatiemapping (ESPHome: 90° `x=y, y=W-x-1`,
270° `x=H-y-1, y=x`, 180° spiegelt beide). De CYD houdt de LVGL-gesture. De
instelling `swipe_pages` blijft de schakelaar; protocol, opslag en sleutels ongewijzigd.

### Compatibiliteit 0.2.28 / firmware 0.2.23

Alleen firmware. `cyd::TouchGuard` krijgt `configure(move_limit_px, min_press_ms)`
(aangeroepen in `on_boot` uit de substituties `TOUCH_MOVE_LIMIT_PX` en
`TOUCH_MIN_PRESS_MS`; zonder aanroep gelden de oude 18 px en 60 ms), volgt alleen
het contact-id waarmee de aanraking begon, meet de verplaatsing als afstand tot een
over vier metingen gesetteld referentiepunt en meldt via `reason()` waarom een tik
is geweigerd; `runtime_tiles::allowed()` logt dat op INFO met tag `touch`.
Protocol, opslag, preferences en sleutels ongewijzigd.

### Compatibiliteit 0.2.27 (firmware 0.2.22 blijft)

Alleen de app. `POST /api/install` (YAML-download) is verwijderd; `POST
/api/firmware/profiles` schrijft het profiel, vult ontbrekende wifi-sleutels in
`secrets.yaml` aan (alleen de ontbrekende regels, geverifieerd door het resultaat
opnieuw te parsen; een ongeldig bestand blijft onaangeraakt) en start met `target`
(een gemelde USB-poort) direct `install`. Poort en bouwslot worden vóór het
schrijven gecontroleerd. Het antwoord bevat `api_key`; de pagina toont hem eenmalig
voor de HA-koppeling. Het jobobject krijgt `stage` (`config`, `compile`, `upload`).
`profile_meta` levert extra `screen` (profiel gebruikt het bordpakket van dit
project) en `api_key`; `/api/inventory` krijgt de additieve lijst `pending` met
zulke profielen zonder gekoppeld scherm. Opslag, protocol, preferences en sleutels
ongewijzigd.

### Compatibiliteit 0.2.26 / firmware 0.2.22

Opslagversie en tegelprotocol blijven 1. Het layoutbericht krijgt het additieve veld
`keepalive` (seconden, 5–3600): het interval waarin de app de hele indeling herhaalt
(`KEEPALIVE_SECONDS` in `server.py`, 120 s sinds 0.2.20). De firmware leidt daar zijn
bewaking van de gegevensstroom uit af (twee rondes plus 60 s) en meldt daarna
"ESP Screens niet actief"; "HA niet verbonden" komt nu van ESPHome's eigen
API-verbindingsstatus (`api_is_connected()`), niet meer uit de leeftijd van het laatste
bericht. Zonder het veld rekent firmware 0.2.22 met 120 s; firmware tot 0.2.21 negeert
het veld en houdt zijn vaste 95 s, wat met een keepalive van 120 s de bekende
"HA niet verbonden"-flikkering geeft. Verander `KEEPALIVE_SECONDS` daarom alleen samen
met een firmwareflash van bestaande schermen, of houd hem onder 40 s. Geen gewijzigde
preferences of sleutels.

### Compatibiliteit 0.2.24 / firmware 0.2.20

Alleen intern: `Tile::forecast` en `Tile::hours` zijn vectoren (voorheen vaste arrays in
elke tegel); protocol en opslag ongewijzigd. De wizard-YAML krijgt `power_save_mode: none`
onder `wifi:`; de gedeelde pakketten bevatten geen wifi-blok, dus bestaande schermen
veranderen pas als de gebruiker de regel zelf toevoegt.

### Compatibiliteit 0.2.23 / firmware 0.2.19

Opslagversie en tegelprotocol blijven 1. `tiles[].options.controls` is additief:
`none` of een set uit `CONTROLS` in `core.py` per domein (climate `setpoint`/`mode`,
switch/light/fan `toggle` plus `brightness`/`speed`, vacuum/cover/timer `buttons`,
cover `position`, media_player `volume`/`playback`, number `stepper`/`slider`,
select `stepper`, scene/script/button `run`). Op de draad stuurt de manager in
`o.controls` alleen de set die de kaart werkelijk toont: uitsluitend dubbelbreed,
weergave standaard en zonder mini-schuif; zonder keuze de eerste set van het
domein, bij `none` niets. Statusberichten krijgen de attributen `device_class`,
`hvac_action` en `is_volume_muted` (de enige boolean die meegaat). Oudere
firmware negeert het veld en toont de gewone brede kaart, daarom geen
`min_firmware`; het tegelpaneel meldt vanaf welke firmware het werkt. Een oude
editor die `controls` weglaat behoudt de opgeslagen keuze (zoals `background`).
De firmware bouwt het paneel lui per set (`layout_panel`), stuurt −/+ na 700 ms
als één `set_temperature`/`set_value` en laat de lokale waarde staan tot HA hem
meldt (of 10 s). De iconfonts krijgen 18 vaste bedieningsglyphs
(`tile_icons.FIXED`). Geen gewijzigde preferences of sleutels.

Ook additief in dezelfde release: `x.hours` (maximaal acht uren `t`, `c`, `h`,
`p`, `r`) en `p`/`r` per dag in `x.days` voor de weerkaart; `x.last` (unix-tijd)
voor scènes, scripts en knoppen; de attributen `humidity`, `wind_speed`,
`wind_speed_unit`, `apparent_temperature`, `fan_modes`, `swing_modes`, `fan_mode`
en `swing_mode`. `supported_features` mag boven de grens van een miljoen. Oudere
firmware negeert al deze velden. De bezig-status gebruikt LVGL's spinner; beide
bordprofielen bevatten daarvoor een verborgen `spinner` (`busy_spinner_seed`),
anders compileert ESPHome `LV_USE_SPINNER` als 0.

### Compatibiliteit 0.2.12 / firmware 0.2.14

Opslagversie en tegelprotocol blijven 1. Additief: `tiles[].options.size`
(`single`/`wide`), nieuwe `display`-waarden (`forecast` voor weer, `graph` voor
sensoren, `digital`/`analog` voor `screen.clock`), de ingebouwde entity
`screen.clock` en de HA-domeinen `sun`, `timer` en `person`. Statusberichten
krijgen een optioneel object `x` met door de manager berekende waarden
(voorspellingsdagen, zontijden in de HA-tijdzone, timer-eindtijd als epoch).
Oudere firmware negeert `size`, nieuwe displays en `x`, maar weigert onbekende
domeinen in de hele indeling; de manager stuurt zo'n indeling daarom pas na
firmware 0.2.14 (`min_firmware`) en bewaart haar ondertussen. Voorspellingen
komen via `weather.get_forecasts` met `return_response`; zonder antwoord blijft
de weerkaart de gewone kaart. Geen gewijzigde preferences of sleutels.
