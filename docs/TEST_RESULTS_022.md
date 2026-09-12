# Controle van versie 0.2.2 — 12 september 2026

De optie **Na standby terug naar pagina 1** sluit nu ook de runtime-detailkaart
boven het tegelraster. Dit geldt onder andere voor vacuum, sensorgeschiedenis,
media en keuzelijsten. De actieve bedieningsentiteit wordt daarbij gewist.

- 42 Python-tests en acht C++-testprogramma's geslaagd.
- Easy Setup en handmatige profielen voor Guition en CYD gebouwd met ESPHome
  2026.6.2; gegenereerde pakketten gecontroleerd.
- Firmware 0.2.2 via OTA op de Guition geïnstalleerd.
- Stofzuigermenu via de bestaande diagnosefunctie geopend zonder HA-actie.
  Voor standby: `dimmed=0`, `runtime_detail=1`.
- De ingestelde natuurlijke standby na 60 seconden laten aflopen, zonder
  de gebruikersinstellingen te wijzigen. Daarna: `dimmed=1`, `page=0`
  (eerste pagina), `runtime_detail=0` (detailmenu gesloten).

Deze test controleert de navigatietoestand op het apparaat. Een CYD was niet
fysiek aangesloten; daarvan zijn builds en regressies gecontroleerd.
