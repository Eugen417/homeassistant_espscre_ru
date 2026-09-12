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
   bestaande handmatige profielen. Controleer dat er geen secrets in Git staan.
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
