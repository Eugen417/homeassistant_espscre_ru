# Acceptatietest en oplevering

Noteer datum, bordvariant/MAC, Git-commit, ESPHome-versie, apparaatnaam en de
namen van de twee meetbestanden. Neem geen wachtwoorden in het rapport op.

## Zonder echte HA-acties

1. Kalibreer met [CALIBREREN.md](CALIBREREN.md); flash de uitkomst en voer een
   nieuwe, onafhankelijke meting met `verify` uit. Een geslaagde fit alleen
   bewijst niet dat de firmwarecoördinaten kloppen.
2. Flash de normale modus. Controleer dat het meetscherm verdwijnt.
3. Controleer elk zichtbaar vak en de randen: geen andere tegel mag reageren.
   Tik rustig en snel achter elkaar, maar verwacht geen herhaling binnen de
   bewuste debounceperiode. Een veeg mag geen tegelactie achteraf veroorzaken.
4. Bij 1–6 tegels: geen paginaknop. Bij 7–10: herhaal minstens twintig keer
   Volgende → tegel → Vorige. De onderste tegel mag niet reageren op de
   paginaknop. Open/sluit overlays; een tik daarin mag niet doorlekken.
5. Draai de ingebouwde renderproef via de versleutelde API:

   ```sh
   python diagnostics/run_ui_test.py --host display-keuken.local --name display-keuken
   ```

   Vereist tien geslaagde paginacontroles, vijftig overlaycycli en voortgang
   van echte draw-callbacks. Dit test geen fysieke touch of HA-acties.
6. Laat het bord minstens 15 minuten aan. Controleer dat het niet herstart,
   dat standby pas na tien minuten zonder aanraking begint en dat de eerste
   tik het scherm wekt zonder een apparaat te schakelen.

## Met Home Assistant

Schakel directe acties pas nu in, flash opnieuw en kies veilige testapparaten.
Controleer elke geconfigureerde tegel met zijn echte HA-entiteit:

- Een tik voert precies één bedoelde actie uit; de terugmelding klopt.
- Veranderen vanuit HA wordt ook op het scherm zichtbaar.
- Lichtslider/kleur, climate-setpoint en ondersteunde modi werken waar gebruikt.
- Vacuumacties en snelheden werken waar ondersteund. Noteer ontbrekende functies.
- Na stroomonderbreking komen wifi en HA terug; naam, kalibratie en tegels blijven.
- Na een korte wifi-/HA-onderbreking herstelt de verbinding zonder USB-flash.

Bewaar logs alleen lokaal. Controleer op resets, watchdogmeldingen, herhaalde
connectiefouten en opvallend trage componenten. Een korte renderproef bewijst
geen stabiliteit over dagen; noteer de werkelijk geobserveerde duur.

## Geautomatiseerde ontwikkelchecks

Vanuit de repo, met de Python-omgeving actief:

```sh
python -m unittest discover -s tests -p 'test_*.py'
c++ -std=c++17 -Wall -Wextra -pedantic tests/test_cyd_ui.cpp -o /tmp/test_cyd_ui
/tmp/test_cyd_ui
c++ -std=c++17 -Wall -Wextra -pedantic tests/test_touch_filter.cpp -o /tmp/test_touch_filter
/tmp/test_touch_filter
python -m esphome config device.yaml
python -m esphome compile device.yaml
```

De C++-commando's hierboven zijn voor macOS/Linux met een compiler. Op Windows
kan de developer een beschikbare C++17-toolchain gebruiken. Flash alleen het
gecontroleerde profiel naar het geïdentificeerde bord.

## Opleverrapport

```text
Bord / MAC:
Git-commit / ESPHome-versie:
Apparaatnaam / HA-entiteiten:
Kalibratiemeting + onafhankelijke verificatie:
Renderproef:
Fysieke paginering/touch/standby:
Werkelijke HA-acties:
Geobserveerde stabiele duur:
Beperkingen / nog niet getest:
Lokale profielbestanden veilig bewaard:
```

Geef de eigenaar de drie lokale YAML-bestanden en de meetbestanden via een
passend privé-kanaal. Gebruik voor een andere eigenaar de schone repository
of starter-ZIP, zodat die eigen sleutels en eigen paneelkalibratie krijgt.
