# Guition 4848S040: onafhankelijke fabrikanttest

Deze test onderscheidt onze ESPHome/UI van de oorspronkelijke paneelaansturing.
Een passende demo is gevonden in het fabrikantpakket dat SpotPear aanbiedt:

- [Verkoper met downloadlink](https://spotpear.com/ESP32-S3-4-inch-LCD-Touchscreen-Display-SHT20-Temperature-Humidity-480x480-RS485-Relay-GC9503-ST7701-FT6336U-GT911-86-TVbox/forum-answer/357.html)
- [Origineel archief](https://cdn.static.spotpear.com/uploads/picture/learn/ESP32/ESP32-S3-4inch/4.0inch_ESP32-4848S040.zip)
- [Community-uitleg voor bouwen met LVGL 8/9](https://github.com/paulhamsh/LVGL_GUITION)

In het archief:

- `1-Demo/Demo_Arduino/1_2_4.0_LvglWidgets/4.0_LvglWidgets/`: broncode.
- `1-Demo/Demo_Arduino/Libraries/Arduino_GFX-master/`: meegeleverde paneeldriver.
- `8-Burn operation/Burn files/4.0_LvglWidgets.bin`: samengevoegde firmware,
  652640 bytes, flashadres 0. Bootloader op 0, partitietabel op 0x8000,
  applicatie op 0x10000.
- SHA256 van die demo: `9662c193fc52407bba2ed1462c582d40148f7d8eedfc806db4e8fff7685abba1`.

Gebruik uitsluitend de Widgets-demo voor de displaytest. De andere meegeleverde
voorbeelden zijn relay- en muziektoepassingen. De Widgets-bron gebruikt alleen
paneel, GT911 en achtergrondverlichting en heeft geen HA-/wifi-configuratie.

## Opvallende referentie-instellingen

De oorspronkelijke bron gebruikt ArduinoGFX, `st7701_type1_init_operations`,
16 MHz pixelklok, RGB-pinnen R=11/12/13/14/0, G=8/20/3/46/9/10,
B=4/5/6/7/15. Horizontaal front/pulse/back=10/8/50; verticaal=10/8/20.
De paneelmodus is 0x3A=0x60, RGB666 op een 16-bits bedrade bus, met 0xCD=0x00.
Rotatie 0 schrijft C7=0 in registerbank 0x10 en MADCTL=0 in bank 0.
De achtergrondverlichting staat via GPIO38 continu aan, zonder PWM.

De gamma- en spanningswaarden lijken op ESPHome's ST7701S-basis, maar de volgorde,
porches en rotatiecommando's verschillen. Dat is aanleiding voor vergelijking,
geen bewijs dat een specifieke wijziging strepen oplost.

## Back-up en terugkeer

Verifieer eerst de chip/MAC en de echte USB-poort. Een volledige flashback-up is
het veiligst. Voor deze specifieke demo worden alleen sectoren binnen de eerste
1 MiB overschreven. Een exacte back-up van dat bereik is voldoende om die
overschrijving terug te draaien als de demo geen andere flashgebieden beschrijft.
Bewaar daarnaast de eigen firmware/YAML. Flashback-ups bevatten persoonlijke
sleutels: alleen lokaal opslaan, nooit in Git.

Gebruik geen erase-all. Herstel na de test de geback-upte sectoren en verifieer de
ESPHome-identiteit, versie, voorkeuren en HA-koppeling. Leg het fysieke resultaat
apart vast: een succesvolle upload zegt niets over het zichtbare beeld.


## Uitgevoerde test op 12 september 2026

- Fabrikantarchief opgehaald en de Widgets-bron plus meegeleverde driver gelezen.
- De eerste 1 MiB (alle door deze demo overschreven sectoren) via USB geback-upt;
  921600/460800 baud gaven communicatieproblemen, 115200 baud werkte.
- Originele Widgets-binary ongewijzigd op adres 0 geschreven; hashverificatie geslaagd.
- USB-bootlog meldt `LVGL Widgets Demo` en `Setup done`, vervolgens GT911-polling.
  Ook meldt de oorspronkelijke demo GPIO-fouten bij het opzetten; daarom blijft
  fysieke bevestiging nodig. De eigenaar bevestigt daarna: de fabrikantdemo is schoon, zonder strepen.
  Daarmee is goede beeldweergave op dit fysieke paneel aangetoond. Dit sluit
  niet iedere hardware-/voedingsfactor uit, maar maakt onze aansturing de
  eerste onderzoekslijn.


## Native ESPHome-kandidaat 0.1.3

De standaard `st7701s`-component krijgt dezelfde 16 MHz-klok en horizontale
10/8/50 en verticale 10/8/20 timing als de fabrikantdemo. SPI gebruikt MODE0;
kleurvolgorde RGB, geen inversie of hardwarematige spiegeling. Na de ingebouwde
initialisatie worden de 16-bits busconfiguratie CD=00 en de standaard registerbank
expliciet gekozen, gevolgd door 3A=60 en de wachttijd na sleep-out. De GT911 blijft
in dezelfde ongemirrorde oriëntatie. De ESPHome RGB-driver wordt niet gekopieerd
of gepatcht; de eerdere extra SDK-cache-/restart-instellingen zijn verwijderd.

De eigenaar meldt dat deze kandidaat schoner is, maar nog enkele vaste verticale
banden toont. Dit is dus geen afgeronde oplossing. De fabrikantdemo blijft de
schone referentie. Verschillen zijn onder meer ESP-IDF/LVGL-versies, framebuffer-
en bouncebuffergebruik, wifi/API, PWM en de extra initialisatiestappen van de
standaarddriver (inclusief software-reset). Native betekent hier de ingebouwde
ESPHome-driver, niet byte-voor-byte dezelfde uitvoering als de fabrikantdemo.

Daarna is alleen de Guition-interface licht vormgegeven: achtergrond F7F7F7,
witte kaarten, rand DDDDDD, donkere tekst en blauwe accenten. Ook de detailkaarten
gebruiken lichte bedieningselementen. Geometrie, paginering, touch, HA-indeling en
opgeslagen instellingen blijven behouden. Deze stijlwijziging is geen strepenfix.

### Lichte interface: softwarecontrole

De Python-suite (33 tests), zeven C++-regressietests en packagegeneratorcontrole
slagen. De eerste lichte firmware is via OTA geïnstalleerd; tien paginacontroles
plus vijftig overlaywisselingen slagen zonder HA-acties. De schermopname toont de
nieuwe lichte kaarten met de bestaande HA-indeling. Lange namen kregen daarna
vaste regelhoogtes met afkapping, zodat titel en status niet overlappen.
Een interne schermopname bewijst geen storingsvrije fysieke paneeluitvoer.

De definitieve lichte versie (config_hash 0x0a70a194) is via OTA geïnstalleerd.
Beide Easy Setup- en beide handmatige profielen bouwen succesvol. De eigenaar
bevestigt vervolgens: ‘Stijl goed, geen banden zichtbaar’. Dit is fysieke
acceptatie van het huidige beeld, geen duurtest na langdurige standby en geen
bewijs dat alleen een driverwijziging de oorzaak heeft opgelost.
