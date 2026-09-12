# Kleuren in versie 0.2.3 — 12 september 2026

De Guition-achtergrond gaat van `#ECECEC` naar `#E7E7E7`; witte tegels blijven
wit. Runtime-detailkaarten gebruiken dezelfde achtergrond.

Iconen gebruiken het [standaardpalet van Home Assistant](https://github.com/home-assistant/frontend/blob/dev/src/resources/theme/color/color.globals.ts):
amber voor lampen/schakelaars, turquoise voor vacuum, cyaan voor ventilatoren,
paars voor covers en lichtblauw voor media. Klimaat volgt de modus. Sensoren
krijgen herkenbare accenten op basis van hun eenheid, uit hetzelfde palet.
Zachte achtergrondcirkels helpen ook in de inactieve toestand het domein herkennen.
Niet-beschikbare tegels blijven grijs.

Voor ingeschakelde lampen worden hue én saturation uit de bestaande HA-status
gebruikt, ook voor de mini-slider. De icoonvoorgrond is iets donkerder voor
leesbaarheid. Ontbrekende kleurattributen gebruiken amber. De add-on hoeft geen
nieuwe attributen of opslagversie te introduceren. Aangepaste kleuren uit
Lovelace-kaarten en CSS-thema's zijn geen entiteitsattributen en worden niet
automatisch geïmporteerd.

De gedeelde icoonkleuren gelden ook voor CYD; diens donkere basisthema blijft
behouden. De bestaande Python- en C++-regressies zijn uitgevoerd.

## Uitgevoerde controles

- 42 Python-tests en alle acht C++-testprogramma's geslaagd.
- ESPHome-builds geslaagd voor Easy Setup en handmatige profielen, voor zowel
  Guition als CYD. Gegenereerde pakketten zijn gecontroleerd met `--check`.
- Guition via OTA bijgewerkt; Home Assistant meldt het scherm online met
  firmware 0.2.3 en status **Gesynchroniseerd**. Bestaande tegels en instellingen
  zijn behouden.
- Interne LVGL-schermopname gecontroleerd: grijze achtergrond, witte kaarten,
  amber sensoraccent, oranje klimaataccent en gekleurde mini-slider zichtbaar.
  Dit controleert de rendering, niet eventuele fysieke paneelstrepen.
- CYD is niet fysiek aangesloten; daarvoor zijn uitsluitend softwaretests en
  builds uitgevoerd. De geïnstalleerde manager 0.2.1 werkt met deze firmware;
  de kleurwijziging vereist geen nieuwe opslagversie of app-installatie.
