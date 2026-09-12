# Firmware 0.2.13 — grote waarde en Guition-backlightfade

## Wijzigingen

- **Grote waarde** (`display: watch`): een klein domeinicoon in een cirkel
  (18 px Guition, 12 px CYD) naast de titel; titel en waarde staan als groep
  verticaal gecentreerd. Het getal gebruikt een eigen groter font
  (Roboto 500, 38 px Guition / 22 px CYD). Een te lange waarde wordt met
  puntjes afgekapt; de eenheid staat als apart label rechts onderaan de regel
  en blijft altijd zichtbaar. De geometriecontrole test ook het eenheidslabel.
- **Guition-backlight**: `components/smart_display/backlight_fade.h` laat de
  LEDC-hardwarefader de duty stappen. ESPHome's software-overgang loopt op de
  hoofdloop, en de volledige LVGL-herteken bij standby onderbrak die zichtbaar.
  Standby dimt nu in 1,5 s, wakker worden duurt 80 ms. Eén script
  (`set_backlight`) is eigenaar van het niveau; een nieuwe aanroep onderbreekt
  de lopende fade vanaf de huidige duty en synchroniseert daarna ESPHome's
  lichtstatus (entity in HA, latere overgangen). Zonder fader (klassieke ESP32)
  valt het script terug op de ESPHome-overgang; de CYD gebruikt dit niet.
- Protocol, opslagversie, preferences en app 0.2.11 zijn ongewijzigd.

## Tests

- 51 Python-tests en alle negen C++-testprogramma's geslaagd.
- `tools/generate_packages.py --check` geslaagd; beide Easy Setup-profielen en
  beide handmatige profielen compileren met ESPHome 2026.6.2.
- USB-flash op de vooraf via `esptool chip_id` geïdentificeerde poorten
  (ESP32 = CYD, ESP32-S3 = Guition). Beide melden daarna firmware 0.2.13 met
  de verwachte compileertijd via `device_info`.
- Renderdiagnose op beide apparaten: tien paginacontroles en vijftig
  overlaycycli geslaagd, inclusief tekst-, icoon-, eenheid- en sliderbegrenzing.
  Guition heap ~6,49 MB, CYD ~57 kB vrij tijdens de proef.
- De eigenaar bevestigt de nieuwe grote waarde en het rustige uitfaden op de
  Guition: "super ik ben helemaal blij". Er is geen langdurige stabiliteitsproef
  uitgevoerd.

## Incident tijdens het flashen

De eerste Guition-flash schreef per ongeluk het handmatige profiel
(`guition-device.yaml`, vaste tegels) in plaats van de Easy Setup-firmware.
Beide profielen heten `guition-wallbox` en delen daardoor
`.esphome/build/guition-wallbox`; een gelijktijdig gestarte regressiebuild van
het handmatige profiel liet de upload het verkeerde bestand kiezen. De Guition
verdween daardoor tijdelijk uit ESP Screen Manager. Herstel: sequentieel opnieuw
bouwen, uploadlog op het pad van `firmware.bin` controleren en de compileertijd
via `device_info` vergelijken. Bewaarde tegels, instellingen en sleutels waren
niet geraakt. Zie de nieuwe regel in AGENTS.md en docs/RELEASING.md.
