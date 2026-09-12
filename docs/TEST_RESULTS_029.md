# 0.2.9 — Guition-rotatie

De beheerpagina biedt kwartslagen voor de Guition. De native ESPHome 2026.6.2
LVGL-component draait beeld en touch samen. De paneelinitialisatie, GT911-transform
en CYD-kalibratie zijn niet aangepast. De aparte opgeslagen hoek verandert de
bestaande settingsstructuur en preference-keys niet.

- 48 Python-tests geslaagd, inclusief ongeldige hoeken, behoud na appherstart en
  opslaan vanuit een oude browser, bordherkenning bij offline status en behoud
  van het elfvelden-wireprotocol.
- Alle acht C++-testprogramma's geslaagd; JavaScript-syntax gecontroleerd.
- Beide runtime- en beide handmatige firmwareprofielen gebouwd.
- Beide gepubliceerde remote pakketten vanuit een lege map opgehaald en met
  fictieve wifi/apparaatinstellingen gevalideerd; geen lokale componenten nodig.
- Guition-runtime op het geïdentificeerde apparaat via OTA geïnstalleerd.
- Renderdiagnose na opdrachten voor 0°, 90°, 180° en 270°: per stand tien
  paginacontroles en vijftig overlaycycli geslaagd. Deze controle bedient geen
  HA-apparaten en is geen fysieke touchmeting.

De definitieve Guition-build bevat daarnaast een uitlezing van de gewenste én
werkelijk door LVGL gebruikte hoek via `ui_state`. Na de definitieve OTA-flash
en herstart meldden beide 270°: de voorkeur bleef behouden en werd toegepast.
Daarna is de testhoek weer naar 0° teruggezet. Fysieke touchacceptatie is
afzonderlijk nodig; de bestaande `tools/verify_gt911.py --rotation` ondersteunt
alle vier de hoeken.

## Productiecontrole

ESP Screen Manager is met back-up bijgewerkt van 0.2.7 naar 0.2.9 en gestart.
De tien Guition-tegels en vier CYD-tegels zijn behouden. In de echte HA-Ingress-
pagina verschijnt de keuzelijst bij de Guition en niet bij de CYD. Via die
keuzelijst 90° gekozen en opgeslagen: de Guition meldt `rotation=90 actual=90`.
Daarna via dezelfde pagina teruggezet naar 0°. De fysieke touchcontrole is aan
de eigenaar gevraagd, maar is nog niet bevestigd.
