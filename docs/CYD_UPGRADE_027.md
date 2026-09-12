# Upgrade bestaand CYD-paneel naar runtime 0.2.7

Bord: ESP32-2432S028, ESP32-D0WD-V3; MAC b0:cb:d8:e7:bd:7c.
Bestaande identiteit CYD 2.8in Display / cyd-2432s028 behouden. USB-poort werd
vooraf met esptool geïdentificeerd; Guition zat op een andere USB-poort.

Eigen profiel: genegeerd easy-cyd-device.yaml, met de bestaande lokale
calibration.yaml en API/OTA/wifi-secretverwijzingen. Een back-up van de eerste
64 KiB (partitie/configuratieruimte) is lokaal opgeslagen. Een volledige snelle
uitlezing gaf een seriële fout; de volledige langzame uitlezing is afgebroken.
Dit is dus geen volledige oude-firmwareback-up. USB-upload op 115200 geslaagd;
verdere correctie via de geverifieerde OTA-host.

De bestaande affine kalibratie is eenmalig in preference-key 0x43594403 gezet.
Daarna gebruikt het definitieve profiel de standaard opstart en bestaande NVS.

## Tijdens de upgrade gevonden

Een tijdelijk on_boot-list in het lokale migratieprofiel verving het on_boot-
object van het pakket. Daardoor ontbrak de normale UI-initialisatie en crashte
het begin van de renderdiagnose. De tijdelijke import is verwijderd nadat de
kalibratie was opgeslagen. In de definitieve gegenereerde C++ zijn runtime-bind,
light_controls::setup en screen_calibration::setup weer aanwezig. De volledige
renderdiagnose slaagt daarna (tien paginacontroles en vijftig overlays).
Gebruik dit tijdelijke migratieprofiel niet voor toekomstige flashes.

HA ontdekte de nieuwe Tegelinstellingen-entity zonder nieuwe API-sleutel. De oude
tien entiteiten zijn overgezet; daarna heeft de gebruiker de indeling gewijzigd.
Die actuele indeling is behouden. Swipe is aan, standby 600 seconden. Bij maximaal
zes tegels blijft de paginering verborgen.

Definitieve build: ESPHome 2026.6.2, 2026-09-12 17:50:49 +0200,
config_hash=0x0a15a59f, broncommit a14a66f.

## Verificatie

- Definitieve build en OTA-upload geslaagd; identiteit en firmwareversie via de
  versleutelde API gecontroleerd.
- Renderdiagnose: tien geslaagde paginacontroles, vijftig overlaycycli en
  `Light sliders: PASS`. De diagnose gebruikte de toen ingestelde twee tegels;
  dit is geen fysieke swipecontrole van vier pagina's.
- Home Assistant en ESP Screen Manager melden het paneel online met versie 0.2.7.
  Wijzigingen van de gebruiker in de tegelindeling blijven behouden.
- Bestaande kalibratie hergebruikt; tijdens deze upgrade is geen nieuwe
  onafhankelijke fysieke kalibratiemeting uitgevoerd.

De 0.2.7-observatie is vijftien minuten voltooid met een levende API-respons elke
dertig seconden, zonder herstart of verbindingsverlies. Vrij geheugen circa
111–119 kB. Door fysieke bediening werd tien minuten onafgebroken inactiviteit
niet bereikt; standby/ontwaken is in deze observatie niet bevestigd.

## Vervolg: tekstlayout 0.2.8

De eigenaar meldde vervolgens afvallende titels, te vroeg afgekorte ondertitels
en een overlappende mini-slider. Stabiliteit alleen bevestigt dus niet de
bruikbaarheid van de layout. De titels hadden geen expliciete breedte en de
ondertitel nam de inhoudsbreedte van de titel over. De compacte kaart had daarnaast
onvoldoende verticale ruimte voor de extra slider.

Runtime 0.2.8 begrenst beide regels op de beschikbare inhoudsbreedte, vergroot die
bij een verborgen icoon en reserveert op de CYD aparte ruimte voor de schuif.
De renderdiagnose controleert nu de echte LVGL-coördinaten op afkapping buiten het
vak en overlapping van tekstregels/slider. Met de actuele vier tegels, waaronder
een lamp met mini-slider: tien paginacontroles en vijftig overlaycycli geslaagd.
De slidertest slaagt ook. Fysieke bevestiging van de nieuwe layout staat nog open.
De vijftien minuten hierboven gelden voor 0.2.7, niet voor deze vervolgbuild.
