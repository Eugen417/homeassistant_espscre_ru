> Historisch onderzoek aan één paneel. Voor een nieuw scherm: [README.md](README.md)
> en [de vijfpuntskalibratie](docs/CALIBREREN.md); neem geen meetwaarden over.

# ESP32-2432S028: stabiele bediening

Gebruik **home-like-2432s028.yaml** voor de aangesloten 2,8-inch CYD met
ILI9341 en XPT2046. `home-like.yaml` hoort bij het andere ST7789V/GT911-bord.
De wifi-, API- en OTA-geheimen blijven in `secrets.yaml`.

## Bediening

- Stel `TILE_COUNT` in op het aantal gebruikte tegels (1–10). Bij maximaal
  zes tegels verdwijnen beide paginaknoppen en de pagina-aanduiding; hogere
  tegelnummers zijn verborgen.
- Pagina 1 bevat tegels 1–6; pagina 2 bevat 7–10. Gebruik de knoppen
  **Vorige** en **Volgende** onderaan. Slepen is uitgeschakeld.
- Kort tikken voert de bestaande tegelactie uit. Lang indrukken opent de
  bestaande licht-, climate- of vacuumkaart, of de ingestelde lange actie.
- Standby begint na tien minuten zonder aanraking (`AUTO_DIM_TIMEOUT: "600"`).
- De eerste tik op het gedimde scherm maakt het scherm wakker.
- Contacten korter dan 60 ms worden genegeerd. Eén aanraking kan maximaal
  één tegelactie geven; een verplaatsing groter dan 18 pixels annuleert de tik; dezelfde tegel wordt 600 ms tegen contactdender beschermd.
- De thermostaat verstuurt de afgeronde, begrensde temperatuur die hij toont.
  Een lokale temperatuur- of moduskeuze krijgt drie seconden om met HA te
  synchroniseren. Een ingedrukte bediening voorkomt automatisch dimmen.
- Pippa gebruikt de echte `vacuum.s8`-status, zodat docken, schoonmaken,
  pauzeren en terugkeren onderscheiden worden. Ontbrekende HA-statussen
  worden als `Niet beschikbaar` getoond.

## Technische keuzes

ESPHome 2026.6.2 met ESP-IDF; er zijn geen Arduino-componenten nodig.
De displaybus draait op 40 MHz, zoals ESPHome's standaard CYD-profiel.
XPT2046 wordt elke 20 ms uitgelezen zonder GPIO36-interrupt. De drukdrempel
is 800, na een verdachte hoekmeting bij druk 422. Een lokale XPT2046-driver
vereist drie consistente ADC-samples voor een nieuwe aanraking, negeert losse
uitschieters en wacht twee drukloze samples af voor loslaten. Dit voegt circa
40–60 ms toe bij aanraken en voorkomt dat een onstabiele eerste meting meteen
als een andere knop wordt geïnterpreteerd. De driver staat onder `components/`. De geïnstalleerde
interruptdriver probeert op die input-only pin een niet-beschikbare interne
pull-up te activeren; de oude firmware logde hiervoor een GPIO-fout.

LVGL krijgt 12% tekenbuffer. Geen scrollanimaties en geen extra flitspuls na
elke tik. Tegelachtergronden zijn ondoorzichtig. Updates binnen 100 ms worden
samengevoegd; ongewijzigde tegellabels en kleuren worden niet opnieuw gezet.
De UI-initialisatie draait pas nadat LVGL is gestart. De klok ververst direct
bij tijdsynchronisatie. Roboto staat lokaal in `fonts/`, inclusief OFL-licentie.

De actieve indeling is 320×240. De oude, uitgecommentarieerde portretpresets
zijn nog geen aangepaste paginalay-outs: reserveer bij een andere oriëntatie
ook onderaan 34 pixels voor de navigatie en controleer de tegelposities.

## Bouwen en flashen

```sh
esphome compile home-like-2432s028.yaml
esphome upload home-like-2432s028.yaml --device /dev/cu.usbserial-130
esphome logs home-like-2432s028.yaml --device /dev/cu.usbserial-130
```

Sluit een lopende seriële loglezer voordat je flasht. De vorige configuratie
staat lokaal in `diagnostics/before-2432s028.yaml.bak`; de bijbehorende vorige
factorybinary in `diagnostics/before-2432s028.factory.bin`. Deze lokale backups
worden niet in Git opgenomen. Teruggaan kan door de backup als configuratie
te herstellen en opnieuw te bouwen/flashen; herstel de oude Arduino-toolchain
als je de oude configuratie bouwt.

## Tests

```sh
clang++ -std=c++17 -Wall -Wextra -Werror tests/test_cyd_ui.cpp -o /tmp/test_cyd_ui
/tmp/test_cyd_ui
python3 tests/test_layout.py
clang++ -std=c++17 -Wall -Wextra -Werror tests/test_touch_filter.cpp -o /tmp/test_touch_filter
/tmp/test_touch_filter
```

De C++-test controleert contactdender, korte ruispulsen, opeenvolgende tegels,
herhaling binnen één aanraking, `millis()`-overflow en temperatuurafronding.
De indelingstest controleert overlap, schermgrenzen, ruimte voor navigatie
en het filter vóór beide handlers van alle tien tegels.

Met de Python-omgeving van ESPHome (met aioesphomeapi, PyYAML en pyserial):

```sh
python diagnostics/capture_serial.py --reset --seconds 120 > diagnostics/runtime.log
python diagnostics/run_ui_test.py --host cyd-2432s028.local
```

De API-test leest de encryptiesleutel intern uit `secrets.yaml` en drukt deze
niet af. Hij wisselt tien keer van pagina en tekent de vijf overlays in elke
ronde. Hij roept geen Home Assistant-acties aan. Raak het scherm tijdens
deze circa 11 seconden niet aan. Dit is een teken-/geheugentest; hij vervangt
geen fysieke toets van de touchkalibratie of een test van HA-service-effecten.

Diagnostiek logt iedere 30 seconden vrije heap en grootste vrije geheugenblok.
`DEBUG` is tijdelijk nuttig voor touch- en verbindingsdiagnose. Voor dagelijks
gebruik kan `logger.level` op `INFO` zodra de fysieke bediening goed is.

## Touchdiagnose

`python diagnostics/control_ui.py touch_diagnostics` opent een apart scherm
met vier kruizen. Tijdens deze meting kunnen aanrakingen geen tegelactie
uitvoeren. Tik nauwkeurig achtereenvolgens linksboven, rechtsboven,
rechtsonder en linksonder. De ruwe en genormaliseerde coördinaten staan in
de seriële logs met tag `touch`. Sluit af met
`python diagnostics/control_ui.py end_touch_diagnostics`. De kruizen staan
op (20,20), (299,20), (299,219) en (20,219) voor landschap 320×240.

De huidige kalibratie (X 225–3800, Y 274–3609) is afgeleid uit de drie
herhaalde metingen rechtsboven en linksonder op dit fysieke bord. De eerdere
metingen linksboven waren onstabiel en zijn bewust niet gebruikt om het
hele scherm naar een verkeerde positie te verschuiven.

Daarbovenop corrigeert `set_raw_correction(...)` in `on_boot` de gemeten
scheefstand. De fit gebruikt de gefilterde hoekmedianen (830,556), (544,3461),
(3410,3391) en (3492,372). De vier medianen vallen na correctie binnen vijf
pixels van hun meetdoel in de regressietest. Dit zijn bord-specifieke
coëfficiënten; neem ze niet blind over op een ander paneel. De `raw=`-logs
blijven de fysieke, gefilterde ADC-metingen tonen.
