# 0.2.8 — compacte tekst en mini-slider

## Wijziging

Tekstregels krijgen de beschikbare kaartbreedte in plaats van een van de
titelinhoud afgeleide breedte. LVGL kan lange titels daardoor met puntjes afkappen.
Bij mini-sliders wordt de ruimte van het verborgen icoon ook voor tekst gebruikt.
De compacte CYD-kaart heeft minder verticale padding en een apart spoor onder
de twee tekstregels. De normale grote detail-sliders zijn niet gewijzigd.

Opslagversie, tegelprotocol, API/OTA-sleutels en preferences zijn ongewijzigd.
ESP Screen Manager 0.2.7 kan deze firmware al bedienen. Alleen de app updaten
verandert de fysieke layout niet: daarvoor moet de schermfirmware worden gebouwd.

## Uitgevoerde controles

- 46 Python-tests geslaagd.
- Alle acht C++-testprogramma's geslaagd, gecompileerd met C++17 en warnings.
- Gegenereerde pakketten gecontroleerd; geen handmatige pakketwijzigingen.
- Beide runtime- en beide handmatige builds met ESPHome 2026.6.2 geslaagd.
  CYD runtime via OTA geüpload; HA meldt versie 0.2.8 en vier behouden tegels.
- Op de CYD: tien paginacontroles, vijftig overlaycycli en de interne slidertest
  geslaagd, met vier bestaande tegels waaronder een mini-slider.
- Daarna drie minuten aaneengesloten API-observatie voltooid zonder disconnect
  of herstart, met circa 118 kB vrij geheugen. Geen stabiliteitsclaim over dagen.
- De paginacontrole toetst nu de echte LVGL-coördinaten inclusief padding en
  fontafmetingen: horizontale tekstgrenzen, afstand tussen beide tekstregels en
  vrije ruimte boven de slider. Dit vervangt geen fysieke touchtest.

De actuele gebruikersindeling is behouden. De fysieke beoordeling van de nieuwe
tekstlayout is bij het opstellen van dit rapport nog niet bevestigd.
