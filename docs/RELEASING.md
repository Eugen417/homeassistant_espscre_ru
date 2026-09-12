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
