# ESP Screens 0.2.0

## Dagelijks bedienen

- Guition: iets donkerder lichtgrijze achtergrond, gekleurde accenten en zwarte
  sliderknoppen. Namen en status hebben elk één eigen regel met afkapping.
- Actietegels tonen minimaal één seconde `Bezig...` en een voortgangsbalk.
  Een ontvangen HA-statuswijziging beëindigt het wachten na die minimumtijd.
  Zonder nieuwe status stopt de indicatie na zes seconden. Dit is feedback op
  een statuswijziging, geen bewijs dat een fysieke actie geslaagd is.
- Tik op een sensor voor numerieke HA-geschiedenis. De app haalt 1, 6 of 24 uur
  op en stuurt 24 tijdvakken; verversing maximaal eenmaal per vijf minuten.
  Ontbrekende/niet-numerieke historie wordt aangegeven. Recorder moet de sensor
  bewaren. Dit is een compacte grafiek, geen energieboekhouding.
- Vacuum: status, accu wanneer als attribuut beschikbaar, start/pauze/dock/zoeken
  en maximaal vier zuigstanden. Media: vorige/play-pauze/volgende en volume.
- Getal-helpers en number-entiteiten krijgen een schuif binnen hun min/max/step;
  select/input_select toont maximaal acht opties. Weather toont conditie en
  temperatuur. Bestaande lamp-, RGB-, climate-, fan- en coverkaarten blijven.

Deze kaarten en tegelopties zijn beschikbaar voor Guition én CYD. De lichte
Guition-stijl is bordgebonden; de CYD behoudt zijn eigen compacte vormgeving.

## Instellingen per tegel

Open in ESP Screens **Tegelinstellingen** onder een gekozen entiteit:

- Bij aantikken: automatisch, bediening openen, alleen bekijken; aan/uit bij
  daarvoor geschikte entiteiten.
- Weergave: normale kaart of **Grote waarde**, bijvoorbeeld temperatuur of kWh.
  De kaart behoudt zijn plek in het raster; er komen geen kleinere aanraakvlakken.
- Mini-schuif: voor lampen, fans, covers, media en getalinstellingen. Een grotere
  waarde en mini-schuif zijn alternatieven, zodat teksten niet overlappen.
- Sensor: geschiedenisperiode 1, 6 of 24 uur.

**Inspector** toont schermverbinding, firmware, laatste afleverstatus, entiteiten,
attributen die het scherm ontvangt en tegelopties. Geen wifi/API/OTA-sleutels.

## Instellingen bij het HA-apparaat

Bij **Instellingen → Apparaten & diensten → ESPHome → jouw scherm** staan onder
Configuratie de getalinstellingen **Helderheid normaal**, **Helderheid standby**,
**Helderheid nacht** en **Standby na** (seconden). Installeer firmware 0.2.0 en laat
HA opnieuw verbinden als ze ontbreken. De overige instellingen staan in ESP
Screens → Scherminstellingen. Wijzigingen via HA worden ook in ESP Screens
opgeslagen; sta HA-acties toe bij de ESPHome-integratie.

## Firmware zonder een tweede beheerpagina

Werk de app bij naar 0.2.0. Deze bevat de officiële ESPHome CLI 2026.6.2.

1. **Nieuw scherm** → bord/naam → **Bewaar profiel in ESP Screens**. Bij een
   eerste installatie vul je wifi in. Bestaande secrets worden nooit vervangen.
   Bewaar de getoonde YAML met de unieke sleutels als eigen back-up.
2. **Firmware & USB** → kies het profiel. De app leest dezelfde map
   `/homeassistant/esphome` die in de HA-configschijf `esphome` heet.
3. Voor een eerste flash: scherm via USB aan de HA-Raspberry, kies expliciet de
   USB-poort en **Bouwen & installeren**. Gebruik één bedoeld bord tegelijk.
4. Koppel het scherm via de gewone HA ESPHome-integratie. Gebruik de API-sleutel
   uit je profiel en sta HA-acties toe. CYD kalibreert bij de eerste start.
5. Voor updates: hetzelfde profiel, **Wifi / OTA**, het IP-adres of de hostnaam
   van dat scherm, **Bouwen & installeren**. Tegels en secrets blijven behouden.

De app voert één CLI-taak tegelijk uit. Validatie en bouwen uploaden niets.
Alleen installeren bouwt én uploadt naar het gekozen doel. Het log maskeert
secrets. De eerste download/build vraagt internet, schijfruimte en tijd. Bouwcache
staat in permanente appdata. Een browser sluiten stopt de build niet.

De bestaande ESPHome Device Builder mag blijven staan, maar is voor deze route
niet nodig. Werk niet gelijktijdig vanuit twee builders aan hetzelfde profiel.
Bestaande eigen YAML blijft leidend; een nieuwe YAML aanmaken is geen updateroute.

## Verfijningen in 0.2.1

Bovenaan je geselecteerde scherm staan **Tegels instellen**, **Algemene
instellingen** en **Inspector**. Open bij een tegel **Bediening & weergave
instellen**. Kies bijvoorbeeld **Kleine slider op deze tegel? → Ja, direct
bedienen**, de klikactie of een grote waarde. Grote waarde en kleine slider zijn
alternatieven; de keuzelijsten werken elkaar direct bij. Klik daarna op
**Opslaan & naar scherm**. Hiervoor hoef je niet opnieuw te flashen.

**Inspecteer deze tegel** toont de actuele HA-status, ondersteunde eigenschappen
en ingestelde weergave. De algemene Inspector toont het hele scherm. Deze knoppen
bedienen geen apparaten.

Firmware 0.2.1 corrigeert het afwijzen van schuifgebaren in de kleine slider.
Alleen een door de slider vastgehouden aanraking mag schuiven; verloren contacten,
ruis en dubbele acties blijven geblokkeerd. De kleine slider heeft alleen een
gekleurde vulling; de grote sliders behouden hun zwarte handvat. Geschiedenis
vermeldt de periode expliciet, bijvoorbeeld **24 uur geleden → Nu**.

De vacuumkaart heeft een robotweergave, status, schoonmaak-/pauzeknop, dockknop en
zuigkrachtkeuze. Knoppen veranderen direct bij indrukken en worden tijdelijk
gedimd na een opdracht. Een ontvangen HA-statuswijziging is terugkoppeling, geen
garantie dat een fysieke schoonmaaktaak voltooid is.

De eerste volledige build op de geteste Raspberry duurde ongeveer 16 minuten.
Laat het firmwarevenster rustig doorwerken; de app blijft bereikbaar. De
OTA-upload zelf duurde circa 9 seconden. Dit is een gemeten voorbeeld, geen
vaste tijd voor ieder systeem.
