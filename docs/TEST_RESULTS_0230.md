# Testresultaten app 0.2.30 / firmware 0.2.25 (2026-09-14)

Het statusbolletje in de schermlijst van de editor is groen voor online schermen.
De firmware krijgt alleen een nieuw versienummer.

## Wat veranderde

- `app.js`: de statusregel onder de schermnaam zet het bolletje in een eigen
  `span.dot` (met `.online` als het scherm online is); `style.css` kleurt
  `.screen-main .dot.online` groen (`#2e9d62`). Offline houdt de gedempte tekstkleur.
- `SCREEN_FIRMWARE_VERSION` 0.2.25 in beide bordprofielen, pakketten en
  `core.FIRMWARE_VERSION`; bench-kopieën in `.esphome/` bijgewerkt.

## Geautomatiseerd

- Python: 84 tests OK. `generate_packages.py --check` groen.
- C++ (clang++ -std=c++17 -Wall -Wextra -Werror -I.): alle `tests/*.cpp` PASS.
- ESPHome compile: `easy-guition-device.yaml` SUCCESS in 43 s (RAM 31,9%, flash 18,3%),
  `easy-cyd-device.yaml` SUCCESS in 44 s (RAM 30,9%, flash 76,0%).
- Lokale add-on (devserver) met twee nep-schermen: berekende kleur van het bolletje
  `rgb(46, 157, 98)` voor online, `rgb(105, 118, 137)` voor offline.

## Nog niet geverifieerd

- Het bolletje in de echte add-on op de Yellow na **Bijwerken** in de add-on store.
