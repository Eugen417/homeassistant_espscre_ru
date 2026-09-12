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
