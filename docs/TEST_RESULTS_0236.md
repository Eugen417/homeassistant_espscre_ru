# Testresultaten app 0.2.36 / firmware 0.2.31 (2026-09-14)

Nieuw: een alert vanuit een Home Assistant-automatisering (`esphome.<scherm>_show_alert`),
zie README "Alert vanuit een automatisering". De kaart staat op LVGL's toplaag boven elke
pagina, kaart en de standby-laag; het scherm wordt gewekt en blijft op de normale
helderheid tot de knop of de timeout.

## Wat veranderde

- `components/smart_display/alert_overlay.h` (namespace `screen_alert`): titel/subtitle
  trimmen en afkappen op een UTF-8-grens, knoptekst met fallback "Oké", icoonnaam,
  `mdi:`-prefix of hex-codepoint alleen als het glyph in de fonts zit (anders
  `alert-outline`), paletkleur of witte kaart, timeout 0..86400.
- `components/smart_display/tile_icon_names.h` wordt door `tools/generate_icons.py`
  gegenereerd uit dezelfde lijst als de fonts (naam naar codepoint).
- Beide profielen: `top_layer` met `alert_overlay`, scripts `alert_show`, `alert_dismiss`,
  `alert_flash` (vier backlight-knippers), `alert_timeout`, `alert_report`
  (`esphome.screen_alert` met `action`, `title`, `screen`); API-acties `show_alert` en
  `dismiss_alert`; de standby-guard wacht op `alert_active`. Het veld heet `button_text`:
  ESPHome reserveert namen van geladen componenten als id en de CYD heeft `button:`.

## Geautomatiseerd

- C++ (clang++ -std=c++17 -Wall -Wextra -Werror -I.): 12/12 PASS; nieuw
  `test_alert_overlay` (iconen, kleuren, timeout-grenzen, afkappen, knoptekst).
- Python: 98 tests OK (`FIRMWARE_VERSION` 0.2.31 in app, profielen en pakketten); nieuw
  `test_alert.py` (actievelden, toplaag vóór de pagina's, standby-guard, geen ander
  sluitpad, events, zelftest zonder HA-actie, kaartgeometrie op beide borden);
  `generate_packages.py --check` en `generate_icons.py --check` groen; bench-kopieën
  opnieuw gegenereerd.
- ESPHome 2026.6.2 compile: `easy-cyd-device.yaml` SUCCESS in 27 s (RAM 31,3%, flash
  76,8%; 0.2.29 was 31,0% / 76,1%), `easy-guition-device.yaml` SUCCESS in 28 s (RAM
  32,3%, flash 18,5%; was 32,0% / 18,3%). Eén waarschuwing, dezelfde als eerder
  (`runtime_tiles.h:546`, enum-conversie in de detailkaart), niet uit deze wijziging.

## Niet getest

- Geen scherm aan USB tijdens deze build: de alert is niet op glas gezien. Wekken, de
  knippers, de knop en de events zijn alleen door validatie, compile en de tekst-tests
  gedekt.

## Te controleren op Studio 1 na de update

- `esphome.studio_1_show_alert` met en zonder `flash`, met `timeout: 0` en `timeout: 10`;
  de kaart moet boven een open lichtkaart en op elke pagina liggen; de knop sluit direct,
  ook met een timeout.
- Backlight blijft op de normale helderheid voorbij de standby-tijd zolang de kaart staat;
  na de knop dimt het scherm weer na de gewone standby-tijd.
- Event `esphome.screen_alert` (Ontwikkelhulpmiddelen → Events) met `action` ok, timeout,
  replaced of remote, `title`, `screen` en `device_id`.
- Iconen (`doorbell`, `mdi:bell`, `F002A`, onbekend) en kleuren; een lange titel krijgt
  puntjes, een lange subtitle wikkelt binnen de kaart; `button_text` op de knop.
- `esphome.studio_1_dismiss_alert` haalt de kaart weg.
- Guition: rotatie 90/180/270 met een alert op het scherm.
