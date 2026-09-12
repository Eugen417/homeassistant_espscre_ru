# Firmware 0.2.12 — switches

## Oorzaak en wijziging

ArduinoJson 7 leegt de uitvoerstring bij `serializeJson`. De oude code zette
`state` eerst in die string en serialiseerde daarna de attributen eroverheen.
Een simpele switch met gelijke attributen kreeg daardoor bij aan/uit dezelfde
revision. De firmware bleef zes seconden op een bevestiging wachten, ook als
de nieuwe status al ontvangen was.

Attributen worden nu apart geserialiseerd, waarna de status wordt toegevoegd.
Switches/input_boolean hebben 150 ms minimale feedback in plaats van één seconde;
zonder bevestiging blijft de bestaande begrensde wachttijd behouden. Overige
kaarten houden hun minimale feedback. De opdracht wordt meteen verzonden en de
tegel wordt direct ververst. Logs meten de tijd tot een gewijzigde HA-status;
dit is een statusbevestiging, geen bewijs van een fysiek relaiscontact.

Lang indrukken opent een native LVGL-switch, zonder HA-opdracht bij openen.
De bediening toont de door HA gemelde stand en blokkeert tijdens een lopende
opdracht of onbeschikbaarheid. Uitgeschakelde switchiconen zijn grijs; eigen
pastel tegelachtergronden blijven behouden. Beide borden gebruiken dezelfde code.

## Metingen en tests

- Voor de correctie: HA-events tijdens bediening door de eigenaar: printer-
  stroomschakelaar 56 ms van servicecall tot statuswijziging; de instelling
  voor stroomuitvalgeheugen 142–151 ms. De oude schermwachttijd is daarmee
  niet gelijk aan de verwerkingstijd van de HA-switch.
- 51 Python-tests geslaagd.
- Alle negen C++-testprogramma's geslaagd. De feedbacktest controleert een
  aan/uit-wijziging met identieke attributen, een onveranderde status, timeout,
  input_boolean en de millis-overloop.
- Pakketgenerator gecontroleerd; protocol, opgeslagen tegels en preferences
  ongewijzigd. App 0.2.11 blijft bruikbaar; dit is een firmware-update.

## Controle op de schermen

- Beide runtimefirmwares gebouwd met ESPHome 2026.6.2 en via OTA geïnstalleerd
  op de vooraf geïdentificeerde Guition en CYD. Beide melden firmware 0.2.12.
- Op beide apparaten: tien paginacontroles en vijftig overlaycycli geslaagd,
  inclusief bestaande controles voor tekst, sliders en pastelkleuren.
- Guition: echte korte tikken geven voor de printer statusbevestiging op het
  scherm na 305 en 434 ms. De grote switch geeft 204 en 365 ms. De switch voor
  stroomuitvalgeheugen geeft 531 en 667 ms. Dit zijn volledige terugmeldtijden
  gemeten door de firmware, inclusief HA en de manager-synchronisatie.
- Interne LVGL-schermopname gecontroleerd: naam, HA-stand en grote schakelaar
  zichtbaar. De eigenaar bevestigt snelle bediening, goed detailmenu en de
  grijze uitstand: “yes, allemaal top!”.
- De actuele indelingen en instellingen voor beide schermen zijn na OTA exact
  gelijk aan de momentopname vóór deze update. Geen appdata of sleutels gewijzigd.

De fysieke bevestiging betreft Guition. CYD is gebouwd, geflasht en met de
renderdiagnose getest; de nieuwe switchbediening is daar niet apart fysiek
bevestigd. Er is geen langdurige stabiliteitsproef uitgevoerd voor deze patch.

## Aanvullende indeling

Mini-sliders behouden hun domeinicoon in een compacte cirkel naast titel en
waarde. Daarvoor zijn afzonderlijke kleine MDI-fonts toegevoegd (18 px CYD,
26 px Guition), zonder schaling via extra renderbuffers. CYD centreert zowel
het tekstblok als de icooncirkel; met een slider gebeurt dat in de ruimte boven
de slider. De geometriecontrole test nu ook icoonbegrenzing en centrering.

De eerste uitgebreide Guition-test vond te brede tekst bij wisselen van een
watch/slider-tegel naar een gewone tegel in hetzelfde vak. LVGL gaf nog de oude
x-positie terug. De berekening gebruikt nu de gevraagde nieuwe positie. Na die
correctie slagen op beide apparaten opnieuw tien paginacontroles en vijftig
overlaycycli, inclusief icoonbegrenzing, tekstbreedte, centrering en sliderafstand.
Alle vier definitieve firmwareprofielen bouwen succesvol. Onder de zware
Guition-renderproef zijn er nog waarschuwingen voor lange renderbewerkingen;
die proef is geen bewijs van constante frame latency in alle situaties.
