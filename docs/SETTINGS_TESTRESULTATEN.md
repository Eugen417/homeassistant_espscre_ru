# Scherminstellingen 0.1.2 — 12 september 2026

- 33 Python-tests geslaagd: invoergrenzen, oude indelingen, behoud van instellingen
  bij herstart en opslaan vanuit een oudere UI, protocol en HTTP/CSRF.
- Zeven native C++-tests geslaagd, inclusief grenzen, nachttijden over middernacht,
  ontbreken van tijdsync en ongeldige opgeslagen settings.
- Easy Setup-firmware voor CYD en Guition gebouwd met ESPHome 2026.6.2.
- Beide handmatige YAML-profielen gevalideerd en volledig gebouwd.
- Browser: standby 20 minuten opslaan en herladen; helderheid naar minimum zetten
  verlaagt hogere dimniveaus mee; waarden blijven behouden na herladen.
- Guition via bestaande versleutelde OTA bijgewerkt naar 0.1.2.
- Op het echte scherm: 55% helderheid direct toegepast; eigenaar bevestigt fysiek
  dimmen. Standby na 60 seconden ondanks herhaalde indelingsberichten. Ongeldige
  helderheid 0 geweigerd zonder de geldige instellingen te veranderen.
- Guition herstart: 55%, 60 seconden en verborgen klok uit preferences teruggelezen.
  Daarna oorspronkelijke instellingen teruggezet (100%, 600 seconden).

CYD was niet aangesloten; de fysieke instellingenacceptatie betreft alleen Guition.
Dit is geen lange stabiliteitstest en geen oplossing voor de bestaande paneelstrepen.
De RGB-driver is bytegelijk aan de oorspronkelijke ESPHome-driver; experimentele
wijzigingen aan timing, spiegeling en reset zijn niet onderdeel van deze release.
