# Testresultaten app 0.2.27 (2026-09-14, firmware 0.2.22 blijft)

Alleen de app: "Nieuw scherm" is één venster van aansluiten tot flash, ontbrekende
wifi gaat in `secrets.yaml`, en de koppeling in Home Assistant (buiten de add-on)
is zichtbaar gemaakt met een knop en een kaart per nog niet gekoppeld profiel.

## Geautomatiseerd

- Python: 84 tests OK (`.venv-portal`, aiohttp 3.13.3, libyaml). Nieuw of aangepast:
  - `test_wifi_reuse`: ontbrekende `wifi_ssid`/`wifi_password` worden aangevuld met
    behoud van commentaar en andere secrets (ook een lege `wifi_ssid: ""` wordt
    vervangen, niet gedupliceerd; waarden met `: `, `\` en `"` overleven de
    round-trip); een ongeldig `secrets.yaml` blijft onaangeraakt en er komt dan geen
    profiel; `install()` weigert een onbekende poort en een bezet bouwslot vóór er
    iets geschreven is; `profile_names()` herkent eigen profielen (`screen`) en levert
    de API-sleutel, een handmatig ESPHome-profiel niet.
  - `test_portal`: `POST /api/firmware/profiles` met een onbekende `target` geeft 400
    zonder profiel; zonder `target` maakt het profiel en geeft `api_key` (32 bytes)
    zonder job; `/api/inventory?light=1` toont het profiel in `pending` met
    `installed: false`; `/api/firmware` lekt de wifi-waarden niet.
  - `test_updates`: metadata-cache met de extra velden.
  - `test_rich_cards.HourlySync` was datumgebonden (vaste 13 september) en faalde
    sinds 14 september op main; de uurvoorspelling in de managertest volgt nu de
    echte klok.
- `py_compile` van de add-on; `node --check` van `app.js`.

## In de browser (in-app browser tegen de echte server, nep-CLI)

`server.py` in `SCREEN_DEV`-modus met een nep-`esphome` op het pad (schrijft
compile-/uploadregels, faalt op een vlag) en een nep-USB-poort (`Firmware.ports`
gepatcht). Doorlopen en per stap bekeken:

- Nieuw venster: bordkeuze, naam → apparaatnaam (`Keuken Guition` → `keuken-guition`),
  wifi-velden zichtbaar zolang `secrets.yaml` ontbreekt; na de eerste installatie
  "Wifi komt uit je ESPHome secrets.yaml" en geen velden meer. `secrets.yaml` bevat
  daarna precies `wifi_ssid`/`wifi_password` (0600), het profiel het bordpakket.
- Installeren via USB: fase "Firmware bouwen…" → "Firmware naar … schrijven…" (uit
  `job.stage`), log inklapbaar, daarna "Klaar." met API-sleutel, kopieerknop, de
  vier koppelstappen en de knop **Open Apparaten & diensten**; CYD-tekst noemt de
  kalibratie, Guition niet.
- Mislukte build: "Installeren mislukt", log open met de foutregel, **Opnieuw
  proberen** herstart alleen de job (geen tweede profiel) en slaagt na het weghalen
  van de vlag.
- Zonder poort: "Later · alleen het profiel bewaren", knop "Profiel bewaren",
  klaar-scherm met de vervolgroutes.
- Zijbalk: per niet-gekoppeld profiel een kaart ("geïnstalleerd, maar nog niet in
  Home Assistant" voor een in deze sessie geflasht profiel, anders "nog niet in Home
  Assistant … Firmware & USB → bestand"), met **Open Apparaten & diensten** (navigeert
  het bovenste venster naar `/config/integrations/dashboard`; op de dev-server een
  404, in HA de integratiepagina) en **Kopieer API-sleutel**.

## Nog niet geverifieerd

- De echte ESPHome-CLI en USB-detectie (`/dev/serial/by-id`) in de add-on-container
  op de Yellow, en de eerste echte flash via dit venster.
- Of de knop **Open Apparaten & diensten** vanuit het HA-ingress-iframe naar de
  integratiepagina springt (zelfde origin, dus verwacht wel).
- Of Home Assistant het scherm na de flash ontdekt en de kaart in de zijbalk
  verdwijnt zodra `Tegelinstellingen` in het register staat.
