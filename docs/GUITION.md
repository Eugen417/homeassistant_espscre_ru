# Guition ESP32-S3 4-inch wallbox — 480 × 480

Deze branch voegt een apart profiel toe voor de **Guition 4848S040** met
ST7701S RGB-display en **GT911 capacitieve touch**, ESP32-S3, 16 MB flash en
8 MB octal PSRAM. De naam/2mm-wandplaat beschrijft ook de behuizing; controleer
altijd de elektronica. Dit profiel configureert geen relais.

Hardwarebron: [ESPHome Guition-bordbeschrijving](https://devices.esphome.io/devices/guition-esp32-s3-4848s040/)
en de ingebouwde [ST7701S-driver](https://esphome.io/components/display/st7701s/).
De paneelconfiguratie volgt de originele, fysiek schoon bevonden fabrikantdemo;
zie [de vergelijking](GUITION_FACTORY_REFERENCE.md). Er is geen lokale RGB-driver
of periodiek herstel-script nodig. De GT911 gebruikt ongemirrorde coördinaten.

## Bestanden

- `guition-4848s040.yaml`: neutrale hardware + 480×480-interface.
- `guition-device.example.yaml`: eigen apparaat-/tegelinstellingen.
- `guition-device.yaml`: jouw lokale profiel, buiten Git.
- `secrets.yaml`: lokale wifi/API/OTA-instellingen, buiten Git.
- `tools/verify_gt911.py`: fysieke pixelcontrole; geen resistieve ADC-kalibratie.

Het 2,8-inch CYD-profiel blijft apart. Flash dat niet naar de Guition en neem
geen `calibration.yaml` of XPT2046-correctie over.

## Interface

### Montage en schermrotatie (runtime 0.2.9+)

Installeer Guition-firmware 0.2.9 en ESP Screen Manager 0.2.9. Open het scherm
in de beheerpagina, ga naar **Scherminstellingen** en kies bij **Scherm draaien**
0°, 90°, 180° of 270°, met de klok mee. Klik op opslaan. Een firmwareflash is
daarna niet meer nodig om de hoek te wijzigen. De instelling blijft in de appdata
en op het scherm bewaard na een herstart.

Dit gebruikt de native ESPHome/LVGL-rotatie voor beeld én touch, zonder wijzigingen
aan de paneelinitialisatie of GT911-spiegeling. Controleer na montage de vier
hoeken en de navigatie fysiek; een renderproef test geen aanrakingen. De optie is
alleen zichtbaar voor de Guition nadat zijn nieuwe firmware door HA is ontdekt.
De CYD blijft op zijn bestaande oriëntatie met zijn eigen kalibratie.

Zes tegels van **218 × 108 pixels** per pagina, 12px tussenruimte en een
aparte navigatiestrook. In ESP Screen Manager passen twintig tegels op maximaal
vier pagina’s; tot zes verdwijnt de navigatie. De huidige interface heeft een
lichtgrijze achtergrond, witte kaarten en gekleurde domeiniconen. Vanaf 0.2.10 kun
je per tegel een pastel achtergrond kiezen met donkere tekst. Standby begint
standaard na tien minuten zonder aanraking en is instelbaar in de beheerpagina.
Vanaf 0.2.13 dimt de backlight via de LEDC-hardwarefader (`backlight_fade.h`,
1,5 s naar standby, 80 ms wakker): de volledige LVGL-herteken bij standby
onderbreekt de overgang dan niet meer. ESPHome's lichtstatus wordt na de fade
gesynchroniseerd, zodat de entity in HA en latere overgangen kloppen.

De bestaande tegelacties, climatebediening en vacuumkaart zijn behouden.
Alle kaarttypen zijn beschikbaar op alle twintig runtime-posities. Alleen in
het oude handmatige profiel blijft de vacuumkaart op tegel 6 en hebben posities
8/10 beperkingen; zie [TEGELS.md](TEGELS.md).

## Nieuwe installatie

Volg de Python-/USB-voorbereiding in [README.md](../README.md), in een verse
kopie. Maak dit profiel:

```sh
python tools/new_device.py --board guition --name wallbox-keuken --friendly-name "Wallbox keuken"
```

Dit maakt `guition-device.yaml` en nieuwe secrets; bestaande bestanden worden
niet overschreven. Vul wifi in en wijzig de tegelentiteiten. Gebruik bij
meerdere borden een aparte map per apparaat. Het huidige lokale Guition-profiel
van de eigenaar mag diens bestaande secrets gebruiken.

Controleer de seriële poort en chip voordat je uploadt. Voor het geteste bord
bleken hogere seriële snelheden bij uitlezen onbetrouwbaar. Gebruik 115200 baud:

```sh
python -m esphome config guition-device.yaml
python -m esphome compile guition-device.yaml
```

Flash via `esphome run` als de USB-verbinding betrouwbaar is. Voor expliciet
115200 baud kun je de gecombineerde factory-image gebruiken. Vervang zowel
poort als apparaatnaam in het pad door jouw waarden:

```sh
python -m esptool --chip esp32s3 --port <USB_POORT> --baud 115200 write-flash 0 .esphome/build/wallbox-keuken/.pioenvs/wallbox-keuken/firmware.factory.bin
```

De factory-image hoort bij dit S3-profiel en begint op adres **0**, anders dan
sommige klassieke ESP32-images. Bewaar een herstelkopie vóór vervanging van
bestaande firmware als die later nog nodig is. Houd binaries en logs lokaal.

## GT911 en oriëntatie testen

De GT911 geeft pixels door. Meestal is geen kalibratie nodig; een verkeerde
rotatie/transform mag niet met de CYD-affinewizard worden weggewerkt.

Met een werkende wifi/API-verbinding:

```sh
python diagnostics/control_ui.py touch_diagnostics --host wallbox-keuken.local --name wallbox-keuken
python tools/verify_gt911.py --port <USB_POORT> --output measurements-guition.json
python diagnostics/control_ui.py end_touch_diagnostics --host wallbox-keuken.local --name wallbox-keuken
```

Er verschijnen vijf kruisjes. De wizard vraagt drie afzonderlijke tikken per
punt; eerst Enter en daarna uitsluitend het genoemde kruisje aanraken. Fout
maximaal 16px, spreiding maximaal 18px. De test stuurt geen HA-acties.

Zonder wifi kun je eerst een build maken met `-s CALIBRATION_ON_BOOT true`,
die na USB-upload direct het GT911-meetscherm toont. Na controle opnieuw
bouwen/flashen zonder die override. Deze compatibiliteitsnaam opent alleen
een pixeltest; hij voert geen ADC-kalibratie uit.

Op het geteste paneel zijn beide GT911-assen gespiegeld: `TOUCH_MIRROR_X` en
`TOUCH_MIRROR_Y` staan op `true`. Standaard is `LVGL_ROTATION: "0"`. Bij een andere montage stel je de gewenste
LVGL-rotatie in en geef je dezelfde waarde aan de wizard, bijvoorbeeld
`--rotation 90`. Controleer alle hoeken; verander de GT911 `TOUCH_SWAP_XY` /
`TOUCH_MIRROR_X` / `TOUCH_MIRROR_Y` alleen als de fysieke meting dat vereist.

## HA en acceptatie

Voeg het nieuwe apparaat toe via de ESPHome-integratie (naam/IP, poort 6053,
API-sleutel uit secrets). Geef toestemming voor HA-acties en gebruik
`DIRECT_ACTIONS: "true"` zodra de juiste entiteiten gecontroleerd zijn.

```sh
python diagnostics/run_ui_test.py --host wallbox-keuken.local --name wallbox-keuken
python -m unittest discover -s tests -p 'test_*.py'
```

Doorloop ook de fysieke controles uit [ACCEPTATIE.md](ACCEPTATIE.md), met de
GT911-wizard in plaats van de XPT2046-kalibratie. Controleer beeld, juiste
aanrakingen, paginering, lange druk, climate/vacuum, echte HA-terugmelding,
standby en herstel na herstart. Een compilatie/renderproef vervangt die
fysieke controle niet. De testuitkomst van het oorspronkelijke CYD-bord
zegt niets over dit nieuwe bord.

## Gerenderde interface inspecteren

```sh
python diagnostics/capture_ui.py --host wallbox-keuken.local --name wallbox-keuken --output diagnostics/guition-home.png
```

Dit maakt alleen op verzoek een LVGL-snapshot en stuurt een 240×240-preview
via de versleutelde API. De tijdelijke buffer wordt na ongeveer 24 seconden
vrijgegeven. Het beeld controleert de renderer; het bewijst niet dat de fysieke
RGB-signalen, schermkleuren of montageoriëntatie goed zijn. De renderproef
controleert ook of zichtbare klikgebieden binnen het 480×480-scherm vallen.

## Herstelkopie van dit testbord

De oorspronkelijke gebruikte firmware inclusief bootloader, partitietabel
en NVS is lokaal bewaard als `diagnostics/guition-original-recovery.bin`.
De lengtes van de vijf applicatiesegmenten bepaalden de benodigde 4.788.224
bytes; dit is geen kopie van alle 16 MB flash. De stub controleerde de MD5
van ieder uitgelezen blok. SHA-256 van het complete herstelbestand:

```text
a656c793c422848b5ad72bc16545fb92c844b7dfba029f4e4399d22444a9e046
```

Deze kopie is privé, hoort uitsluitend bij het geteste bord en wordt niet
meegecommit of geëxporteerd. Herstellen gebeurt op adres 0, met dezelfde
115200-baudopdracht als de factory-image, maar met dit herstelbestand.

## RGB-geheugeninstellingen

De pixelklok staat op 16 MHz: op het testbord verdween daarmee het zachte
flikkeren van de 12MHz-instelling. Voor de RGB-bouncebuffer worden code en
constante data vanuit octal PSRAM uitgevoerd, met 64KB datacache en 64-byte
cachelijnen; herstel van de RGB-stroom gebeurt op VSYNC. Deze instellingen
volgen [Espressifs RGB-LCD-aanbevelingen](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/peripherals/lcd/rgb_lcd.html).
Controleer fysiek op incidentele glitches onder wifi-/renderbelasting; een
software-snapshot kan een verstoring van het paneelsignaal niet bewijzen.
