# App 0.2.16 / firmware 0.2.17 — bijwerken met één knop en nachtelijke ronde

## Wijzigingen

- **Firmware**: twee diagnostische text sensors per scherm, `Apparaatnaam`
  (`${DEVICE_NAME}`, elke 60 s) en `IP-adres` (`wifi_info`). Verder ongewijzigd;
  geen preferences, protocol of layoutcode aangeraakt.
- **App** (`screen_manager/app/updates.py`): `Updater` met `FIRMWARE_VERSION`
  als doelversie, profielkoppeling via `Apparaatnaam` → `esphome.name` in de
  ESPHome-map (`Firmware.profile_names`, leest YAML zonder `!secret` uit te
  voeren), terugval op één profiel met dezelfde `friendly_name` plus een
  handmatig adres. Eén ronde tegelijk: `install` (bestaande `Firmware.start`),
  wachten op `Schermfirmware >= doel` (max 240 s), 60 s stabiliteit, 120 s pauze
  tot het volgende scherm; de eerste fout beëindigt de ronde. Nachtelijke ronde
  tussen 03:00 en 06:00 in de HA-tijdzone, één keer per dag, alleen met het
  vinkje aan; bij een fout een `persistent_notification` in HA.
- **UI**: schermkaart is nu een `div` met een selectieknop en een updaterij
  (badge *Update x.y.z* + **Bijwerken**, spinner met fase, resultaat een dag
  zichtbaar). Blok *Firmware-updates* met **Alle schermen bijwerken** (zichtbaar
  bij twee of meer bij te werken schermen) en het vinkje voor de nachtronde.
  Een open IP-formulier overleeft de periodieke verversing; polling versnelt naar
  3 s zolang er een update loopt.
- Opslag `/data/updates.json` (`version: 1`), naast `screens.json`.

## Tests

- `.venv-portal/bin/python -m unittest discover -s tests`: 62 tests OK, waaronder
  de nieuwe `tests/test_updates.py` (versiebron gelijk in app en beide pakketten,
  discovery van naam en adres, profielen lezen zonder secrets, aanbod en
  handmatig adres, ronde stopt na fout en meldt alleen automatisch, scherm dat
  niet terugkomt faalt, nachtvenster en persistentie, HTTP-endpoints met CSRF).
- `python3 tools/generate_packages.py --check`: beide pakketten actueel.
- `esphome compile` voor `easy-cyd-device.yaml` en `easy-guition-device.yaml`
  (lokale pakketten met dezelfde inhoud als `packages/*.yaml`): geslaagd.
- USB-flash na `esptool chip_id` per poort (`usbserial-130` = ESP32/CYD,
  `usbserial-210` = ESP32-S3/Guition). Uploadlog toont
  `.esphome/build/cyd-2432s028/…/firmware.bin` resp.
  `.esphome/build/guition-wallbox/…/firmware.bin`.
- `device_info` via de API: CYD `cyd-2432s028` project_version 0.2.17,
  Guition `guition-wallbox` project_version 0.2.17, beide gecompileerd
  2026-09-13 10:03. Beide melden `Schermfirmware=0.2.17`, `Apparaatnaam` gelijk
  aan de profielnaam en `IP-adres` 192.168.146.134 resp. 192.168.146.136.
- Lokale preview van de beheerpagina met een nep-HA en een trage nep-CLI:
  badge en knop bij een scherm op 0.2.16, spinner met "Bouwen en installeren…",
  daarna "Bijgewerkt naar firmware 0.2.17." bij het scherm; scherm zonder
  gemeld IP vraagt eenmalig het adres en start daarna; scherm op 0.2.17 toont
  geen aanbod; het vinkje bewaart `auto` via `PUT /api/updates`.

## App 0.2.17 (zelfde firmware)

- Productie draaide nog de lokale app 0.2.12 uit `/addons`; de GitHub-app
  `ec8ae0ed_esp_screen_manager` 0.2.16 is via de Supervisor-API geïnstalleerd,
  de twee indelingen zijn via de ingress-API overgezet (7 en 5 tegels), de
  lokale app is verwijderd. Voor de CYD ontbrak een profiel in de ESPHome-map
  van HA; `cyd-2432s028.yaml` is aangemaakt met de paneelkalibratie en
  `!secret`-sleutels (gelijk aan de sleutels in de firmware); `esphome config`
  via de app slaagde. Beide schermen koppelen nu aan profiel én IP.
- In de echte HA-pagina (Chrome, 1400 px) kreeg het checkboxje de algemene
  `input`-stijl (100% breed, padding), waardoor de labeltekst naast de zijkolom
  onder het editorpaneel viel. Fix: vaste 16 px, `flex: none`, geen padding.
  Het blok *Firmware-updates* start nu verborgen tot de eerste inventory.

## Nog te doen in productie

- Add-on bijwerken vanuit de HA-appwinkel (0.2.16) en beide schermen (nu al
  0.2.17 via USB) moeten *geen* aanbod tonen. De eerste echte OTA-ronde volgt
  bij de volgende firmwareversie; controleer dan het log onder Firmware & USB,
  de spinnerfases en de melding bij een geforceerde fout.
