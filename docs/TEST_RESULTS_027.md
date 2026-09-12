# Paging en swipe — 0.2.7

Runtime ondersteunt twintig tegels, zes per pagina (vier pagina's). De bestaande
zes zichtbare LVGL-tegels worden hergebruikt; tien extra kaarten worden niet
gealloceerd. Logische entity-index en mini-sliderbinding volgen de actieve pagina.
Handmatige profielen behouden hun oude tien posities.

Swipe is opt-in via swipe_pages. LVGL LV_EVENT_GESTURE handelt horizontale gebaren
af; lv_indev_wait_release plus de TouchGuard voorkomt een actie bij loslaten.
Mini-sliders stoppen GESTURE_BUBBLE. Standby, kalibratie en detailmenu's blokkeren
paginagebaren. Vorige/Volgende blijven bruikbaar, zonder doorrollen aan de grenzen.

De bestaande settings-preference blijft exact gelijk. Swipe krijgt aparte
uint32-preference 0x53575031. Op de draad blijft settings elf velden houden;
swipe_pages is een optioneel top-level veld dat oude firmware negeert. De manager
bewaart het in settings en behoudt het bij een save vanuit een oudere browser.
Meer dan tien tegels opslaan/synchroniseren vereist firmware >=0.2.7. Bij oudere
firmware blijft een grotere indeling bewaard zonder ongeldig doorsturen.

Controles tot nu toe: 46 Python-tests, acht C++-tests en alle vier ESPHome-builds
(Easy/manual, Guition/CYD) geslaagd. Test twintig entities/21 berichten, overflow,
firmwaregate, oude instellingen op de draad en contact consumeren geslaagd.
Fysieke swipeacceptatie en live HA-update volgen hieronder.

## Live acceptatie

- Add-on bijgewerkt op HA naar 0.2.7; firmware via OTA geïnstalleerd.
- Tijdelijk twintig echte entiteiten ingesteld; tien extra sensoren hadden tap=none.
  Swipe aan en standby tijdelijk 600 seconden voor de test.
- Eigenaar bevestigt: “swipe enzo werkt goed”. Guition fysiek getest; CYD is niet
  aangesloten en blijft voor swipe opt-in.
- De aangepaste ui_self_test doorloopt alle beschikbare pagina's: tien controles
  met count=20 en vijftig overlaywissels PASS. Guition vrije heap circa 6,52 MB
  inclusief PSRAM; renderframes lopen door. Tijdens de stresstest enkele
  ui_refresh-meldingen rond 56–58 ms (boven de algemene 50 ms-waarschuwingsgrens).
- Definitieve vier firmwarebuilds opnieuw geslaagd na uitbreiding van de diagnose.
  Statische RAM-buildcijfers: Guition 91.564 bytes, CYD 88.436 bytes. Dit zijn geen
  runtime-heapmetingen voor de niet-aangesloten CYD.
- Na test de tijdelijke sensoren verwijderd: oorspronkelijke tien tegels exact
  behouden, standby terug naar de gekozen 60 seconden, swipe blijft aan.

Bij downgraden van de add-on naar een versie met maximaal tien tegels eerst de
indeling terugbrengen tot tien of de bijbehorende oudere gegevensback-up herstellen.
Swipe werkt volgens de native LVGL-gebaarafhandeling:
https://lvgl.io/docs/open/9.0/overview/indev
