## Easy Setup en releases

De voorkeursroute voor nieuwe gebruikers is docs/EASY_SETUP.md: ESP Screen Manager
plus remote ESPHome-pakketten. Geen token of blueprint nodig. Tegels staan in de
permanente add-ondata; wifi/API/OTA blijven in de eigen ESPHome-YAML. Lees
docs/RELEASING.md voordat je updates publiceert. Main distribueert beide borden.
Elke push naar GitHub is een release: verhoog dan ook altijd de add-onversie in
screen_manager/config.yaml (met CHANGELOG-regel), anders ziet HA geen update.
Genereer packages met tools/generate_packages.py; bewerk ze niet handmatig.
Runtime-modus ondersteunt alle kaarten op alle twintig posities. De positiebeperkingen
hieronder gelden uitsluitend voor de oude handmatige profielen.
Behoud gegevensschema, protocolcompatibiliteit, unieke sleutels en CYD-preferences.
Test updates met bestaande gegevens. Publiceer geen onbekende opslagversie zonder
migratie. Productie-Ingress heeft geen long-lived token of publieke poort nodig.

## Guition-branch

De Guition 4848S040 heeft een apart profiel `guition-4848s040.yaml` met
480×480, ST7701S RGB en GT911. Lees docs/GUITION.md. Gebruik de eigen lokale
`guition-device.yaml`; neem geen CYD-layout of XPT2046-kalibratie over.
Controleer touch met tools/verify_gt911.py en houd de CYD-regressies groen.
Configureer geen wallbox-relais als onderdeel van displayondersteuning.

# Werkinstructies voor LLM's en developers

Dit project bedient een ESP32-2432S028 met ILI9341 + XPT2046 (320×240,
LVGL 90°). Lees README.md en docs/ voordat je installeert. De eigenaar kan
fysiek tikken; een agent kan dat niet vervangen door softwarecoördinaten.

## Een nieuw scherm installeren

1. Identificeer USB-poort en bordvariant. Gebruik ESPHome 2026.6.2 met Python
   3.11–3.14. Controleer eerst of er lokale configuratie bestaat.
2. Maak in een verse kopie met `tools/new_device.py` een eigen `device.yaml`,
   `calibration.yaml`, `secrets.yaml`. Overschrijf nooit een werkend profiel.
   Laat de eigenaar wifi lokaal invullen. Toon geen sleutels in logs/chat.
3. Flash `device.yaml` met de CLI-substitutie `CALIBRATION_ON_BOOT=true`.
   Het geïsoleerde scherm heeft vijf kruisjes en werkt zonder HA.
4. Volg docs/CALIBREREN.md: geleide USB-capture, fit, flash, nieuwe capture,
   onafhankelijke verify. Vraag fysieke tikken per doel. Wacht op bevestiging
   dat het meetscherm werkelijk zichtbaar is; een geslaagde build is geen flash.
5. Flash zonder die override. Koppel de eigen HA via de ESPHome-integratie.
   Lees echte entity-ID's en ondersteunde attributen; verzin geen entiteiten.
6. Configureer alle gebruikte tegels volgens docs/TEGELS.md. De vacuumkaart
   zit op positie 6. Posities 8/10 hebben geen volledige slider/climatebinding.
   Test geen echte apparaat-acties zonder toestemming van de eigenaar.
7. Doorloop docs/ACCEPTATIE.md en rapporteer werkelijk uitgevoerde tests,
   beperkingen en de geobserveerde stabiliteitsduur.

## Code en regressies

- Houd basishardware en UI in `home-like-2432s028.yaml`; eigen gegevens horen
  in de genegeerde lokale profielen. `home-like.yaml` is een ander oud profiel.
- Behoud vaste pagina's, verborgen navigatie bij maximaal zes tegels en
  minimaal 600 seconden standaard-standby. Geen vrij scrollen herintroduceren
  zonder touch-/navigatieregressies fysiek te testen.
- Behoud touchfilter en eventguard vóór acties; kalibratie verwerkt gefilterde
  fysieke ADC-waarden. Zet de affine correctie niet dubbel in driver en UI.
- Kalibratiewizard veronderstelt swap_xy=false, mirror_x=true, mirror_y=false
  en LVGL 90°. Een gewijzigde oriëntatie vereist ook nieuwe projectie/tests.
- Run Python-tests, beide C++-tests en ESPHome-validatie/build bij codewijzigingen.
  Firmwaretests en hardwareacceptatie zijn verschillende controles.
- `diagnostics/run_ui_test.py` rendert zonder HA-acties; raak tijdens die test
  het scherm niet aan. Gebruik `--name` voor de verwachte apparaatidentiteit.
  `diagnostics/send_layout.py` zet een demo-indeling met alle kaarttypen op een
  scherm via de API-inbox (geen HA-acties); de manager herstelt de echte
  indeling binnen ~25 s. Guition: `capture_ui.py` bewaart de LVGL-render als PNG.
  Zonder scherm: `tools/render_topbar.py` rendert de echte bovenbalkcode van beide
  borden via ESPHome host + SDL2 naar `.esphome/render-topbar/out/sheet.png`.
- Deel via `tools/export_bundle.py` of Git. Stage geen secrets, metingen,
  binaries, logs, buildcache of lokale apparaatprofielen.
- Geen automatische firmware-upload naar een willekeurige aangesloten poort.
  Bij meerdere borden eerst de bedoelde poort vaststellen.
- Profielen met dezelfde `DEVICE_NAME` (Easy Setup en handmatig) delen
  `.esphome/build/<naam>`. Compileer of upload ze nooit gelijktijdig; controleer
  in het uploadlog het pad van `firmware.bin` en daarna de compileertijd via
  `device_info`. Een verkeerd profiel haalt het scherm uit ESP Screen Manager.

Historische diagnose is achtergrond, geen bewijs dat een nieuw paneel goed
werkt. Maak tijdens onboarding geen claims over niet-uitgevoerde fysieke tests.
