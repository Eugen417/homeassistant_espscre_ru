# App 0.2.15 / firmware 0.2.16 — kalenderklok en tegels zonder achtergrond

## Wijzigingen

- **Analoge klok**: index-streepjes (`lv_line`, parts 0–11) met de cijfers 12,
  3, 6 en 9 als labels op de vier hoofdposities van grote kaarten; de CYD
  houdt twaalf streepjes. Op een enkele tegel staat naast de wijzerplaat een
  kalenderblok: weekdag (tegelsubfont), grote dag (`clock_digits`) met de
  korte maand (`label`) op dezelfde basislijn — LVGL's `base_line` wordt
  daarvoor gebruikt. De CYD toont "13 sep" in het grote-waardefont. De
  dubbelbrede kaart houdt de digitale tijd en datum naast de wijzerplaat.
  Nieuwe extra-modus `calendar` (enkel) naast `analog` (dubbelbreed), zodat
  de parts bij een breedtewissel opnieuw worden opgebouwd.
- **Achtergrond: Geen** (`background: none`): de firmware zet `bg_opa` en
  `border_opa` van de tegel op transparant; padding, maten en positie blijven
  identiek aan een kaart, zoals de eigenaar vroeg ("niet groter maken zonder
  achtergrond"). De pressed-flits blijft werken (PRESSED-state). Het skeleton
  bij een paginawissel toont voor zo'n tegel geen kader. `check_tile_geometry`
  controleert nu ook of de opaciteit klopt met de tegeloptie.
- **App**: paletsleutel `none` ("Geen") met een doorgestreept staal; de mockup
  toont zo'n tegel als stippellijn; `min_firmware` eist 0.2.16 voor indelingen
  met deze optie. Demo-indeling (`diagnostics/send_layout.py`) gebruikt nu de
  enkele analoge klok zonder achtergrond; `--wide` geeft de dubbelbrede klok.

## Tests

- 54 Python-tests (met nieuwe paletnaam-test) en negen C++-testprogramma's
  geslaagd; `tools/generate_packages.py --check` verifieert beide pakketten.
- Guition-build geslaagd (RAM 31,0 %, flash 17,4 %); CYD-build geslaagd
  (RAM 30,0 %, flash 73,0 %). Eerste build faalde op `std::max(int, long)`
  rond `lv_font_get_line_height`; opgelost met expliciete int-casts.
- Poorten vooraf vastgesteld met `esptool chip_id`: `usbserial-210` = ESP32-S3
  (Guition), `usbserial-130` = ESP32 (CYD). Beide via USB geflasht; het
  uploadlog toont `firmware.bin` uit de juiste buildmap.
- Guition, demo-indeling: acht `page_check=PASS count=9` (kalenderklok zonder
  achtergrond, forecast, grafieken, zonnebaan), daarna één FAIL op het moment
  dat de manager de echte indeling terugzette (slot 5 wisselde van zonnebaan
  naar een gewone tegel binnen dezelfde frame; count ging naar 5). Tweede run
  op de echte indeling: 10/10 PASS (`count=5`, heap ~6,46 MB). Snapshot
  `diagnostics/guition-0216-demo-p1.png` toont de echte indeling met de
  kalenderklok (streepjes, cijfers, "zondag 13 sep").
- CYD, demo-indeling: 10/10 PASS (zes met `count=9`, daarna `count=7` na
  herstel van de echte indeling; ~40–53 kB vrij heap).
- De eigenaar bekeek de Guition na de flash: "ziet er top uit". De CYD is nog
  niet visueel beoordeeld; geen langdurige stabiliteitsproef uitgevoerd.
