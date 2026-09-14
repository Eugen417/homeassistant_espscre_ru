# Testresultaten app 0.2.35 / firmware 0.2.30 (2026-09-14)

Vervolg op 0.2.34, live op Studio 1 (firmware 0.2.29): de randveeg wisselt (14 wissels in
één reeks), maar ongeveer de helft van de snelle vegen eindigde met `randveeg niet gevuurd:
1…17 px naar binnen` terwijl de vinger verder ging. Elke aanraking duurde van druk tot
wissel of loslaten 147–199 ms en leverde maar één meting op; geen LVGL-waarschuwingen
(de drempel daarvan groeit mee tot 200 ms, dus stalls van 100 ms zijn onzichtbaar).

## Diagnose

- Een druk in de lege rand landt op `tile_scroll` (of `home_page`), beide met de thema-stijl
  `obj: pressed: bg_opa: 45%`: LVGL hertekent bij indrukken en loslaten een vlak van
  480×348 px met blending. Op een tegel is diezelfde hertekening klein (de guard zag daar
  wel 73 px drift), in de rand blokkeert hij de ESPHome-loop en daarmee de 20 ms-polling
  van de GT911 precies zolang een flick duurt.

## Wat veranderde

- `on_boot`: `LV_OBJ_FLAG_CLICKABLE` af van `home_page` en `tile_scroll`. Een druk buiten
  de tegels heeft geen LVGL-object meer: geen pressed-stijl, geen hertekening, geen events.
  De randveeg loopt via de touchscreen-triggers en `lv_indev_wait_release` werkt zonder
  actief object.
- Trace: zolang een randveeg gewapend is logt `on_update` elke meting (`veeg id= st= x= y=`),
  en `GT911 press` meldt het contact-id.

## Geautomatiseerd

- C++ `test_cyd_ui` PASS (geen wijziging in de klassen); Python: 91 tests OK
  (`FIRMWARE_VERSION` 0.2.30); `generate_packages.py --check` groen.
- ESPHome 2026.6.2 compile: `easy-guition-device.yaml` SUCCESS in 42 s (RAM 32,0%, flash
  18,3%), `easy-cyd-device.yaml` SUCCESS in 39 s (RAM 31,0%, flash 76,1%); geen nieuwe
  waarschuwingen.

## Te controleren op Studio 1

- Snelle vegen vanaf beide randen: het log moet per veeg meerdere `veeg`-regels (elke ~20 ms)
  tonen en `randveeg: pagina …` binnen 40 px. Blijft het bij één meting, dan zit de
  bemonstering elders (I2C, GT911-frame-rate) en is de trace de volgende aanwijzing.
