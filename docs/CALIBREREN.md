# Een nieuw paneel kalibreren

Dit protocol is voor **320×240, LVGL-rotatie 90°, swap_xy=false,
mirror_x=true, mirror_y=false**. Andere oriëntaties, GT911-touch en andere
controllers vereisen een apart profiel. Verander rotatie of mirroring niet
halverwege de meting.

De filter in deze repo stabiliseert de XPT2046-metingen. Kalibratie corrigeert
vervolgens offset, schaal en scheefstand. Kalibratie repareert geen defect
paneel, losse verbinding, instabiele voeding of willekeurige uitschieters.

## Voorbereiden

1. Gebruik `device.yaml` van `tools/new_device.py`, met een eigen
   `calibration.yaml` die begint als identiteit. De persoonlijke correctie
   in het historische basisprofiel mag niet worden overgenomen.
2. Controleer de datakabel en de USB-poort met `python -m serial.tools.list_ports`.
3. Flash vanaf de reporoot:

```sh
python -m esphome -s CALIBRATION_ON_BOOT true run device.yaml --device <USB_POORT>
```

4. Controleer fysiek het **donkere scherm met vijf witte + tekens**.
   Het blijft aan en de tegelacties zijn hier niet toegankelijk.
5. Stop de ESPHome-loglezer met Ctrl+C. Laat het bord en de USB-kabel zitten.
   Slechts één programma mag tegelijk de seriële poort gebruiken.

Wifi/HA zijn voor deze USB-route niet nodig. Als het bord al op wifi staat,
kan het scherm ook via de versleutelde API worden geopend:

```sh
python diagnostics/control_ui.py touch_diagnostics --host display-keuken.local --name display-keuken
```

Dat verandert de startmodus niet. Gebruik voor een nieuwe installatie de
USB-route hierboven, zodat de kalibratiemodus ook na een reset actief blijft.

## Meting A: gegevens verzamelen

```sh
python tools/calibrate.py capture --port <USB_POORT> --device-name display-keuken --output measurements-before.json
```

De wizard begeleidt **één doel tegelijk**:

1. Lees in de terminal welk kruisje aan de beurt is.
2. Druk op Enter en tik daarna alleen dat kruisje **drie keer**.
3. Houd elke tik circa een halve seconde vast en laat tussendoor volledig los.
4. Wacht op de volgende terminalprompt. Ga niet zelf alvast naar een andere hoek.

Volgorde: linksboven, rechtsboven, rechtsonder, linksonder, midden.
Gebruik een geschikte stompe stylus of een kleine, bewuste vingertik; geen
scherpe punt. De doelen staan op `(20,20)`, `(299,20)`, `(299,219)`, `(20,219)`
en `(160,120)`.

De wizard leest alleen touchlogs met `calibration=1`. Hij flasht niets, maakt
geen netwerkverbinding en stuurt geen HA-acties. Een onvolledige meting wordt
niet als geldig bestand opgeslagen. Bij een verkeerde tik: Ctrl+C en opnieuw
beginnen. Gebruik een nieuwe bestandsnaam als een meting al bestaat.

## Een correctie berekenen

```sh
python tools/calibrate.py fit measurements-before.json --output calibration.yaml --replace
```

Het script:

- neemt per hoek de mediaan van de drie ruwe ADC-metingen;
- bepaalt een affine correctie uit de vier hoeken;
- controleert het **niet voor de fit gebruikte middenpunt**;
- weigert te grote fouten (>12 px), spreiding (>18 px) en onbruikbare meetgeometrie;
- bewaart de bestaande `calibration.yaml` als een gedateerde `.bak` voordat
  een nieuwe, complete versie wordt geplaatst.

De output bevat alleen de vier basisgrenzen en zes affine coëfficiënten in
een ESPHome-substitutions-map. `device.yaml` importeert deze als calibration-
package. Verander de coëfficiënten niet op gevoel en voeg geen tweede correctie
in de driver toe. De `raw=`-logs bevatten de gefilterde **fysieke ADC-waarden**,
ook als al een correctie is ingesteld; opnieuw kalibreren telt dus niet dubbel op.

Een geslaagde fit op bestaande data is nog geen geslaagde fysieke test.

## Meting B: de geflashte uitkomst onafhankelijk testen

Flash de correctie, nog steeds met het meetscherm:

```sh
python -m esphome -s CALIBRATION_ON_BOOT true run device.yaml --device <USB_POORT>
```

Sluit de loglezer met Ctrl+C en verzamel **nieuwe tikken**:

```sh
python tools/calibrate.py capture --port <USB_POORT> --device-name display-keuken --output measurements-after.json
python tools/calibrate.py verify measurements-after.json
```

`verify` gebruikt de echte `native=`-coördinaten van de firmware, draait die
naar de schermcoördinaten en vergelijkt ze met de doelen. Hij voert **geen
nieuwe fit** uit. Alle vijf punten moeten slagen. Gebruik niet per ongeluk
het bestand van vóór de flash voor deze controle.

Lukt dit niet? Bekijk welk punt faalt, controleer de volgorde/druk, meet opnieuw
en lees [PROBLEMEN.md](PROBLEMEN.md). Verhoog niet alleen de tolerantie om de
test groen te krijgen. Rapporteer een resterende hardwareafwijking eerlijk.

## Terug naar normale bediening

```sh
python -m esphome run device.yaml --device <USB_POORT>
```

Hier is **geen** `-s CALIBRATION_ON_BOOT true` opgegeven. In `device.yaml` blijft
de instelling `false`. Controleer de werkelijke tegels en navigatie volgens
[ACCEPTATIE.md](ACCEPTATIE.md).

Als de kalibratiemodus uitsluitend via de API was geopend, kun je hem sluiten:

```sh
python diagnostics/control_ui.py end_touch_diagnostics --host display-keuken.local --name display-keuken
```

Bewaar beide meetbestanden en `calibration.yaml` bij dit fysieke scherm.
Na vervangen van het paneel opnieuw beginnen; geef alleen de neutrale
starterbestanden aan een volgende eigenaar.
