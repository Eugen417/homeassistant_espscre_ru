# Testresultaten app 0.2.37 / firmware 0.2.31 (2026-09-14)

Nieuw: de cheatsheet **Alerts** in ESP Screens (knop bovenaan). Home Assistant toont bij
ESPHome-apparaatacties alleen veldnamen en typen, geen uitleg of keuzelijsten; de pagina
vult dat gat. Geen firmwarewijziging.

## Wat veranderde

- `core.py`: `alert_reference()` (velden, limieten per bord, kleuren, aanbevolen en extra
  iconen, event en eindes) en `alert_service(node, action)` voor de actienaam
  `esphome.<apparaatnaam>_show_alert`. `server.py` stuurt de referentie mee in de volledige
  inventaris en zet per scherm `alert_action` en `dismiss_action`.
- `static/index.html`, `app.js`, `style.css`: dialoog met mock van de kaart, sprongknoppen,
  per scherm de actienamen met kopieerknop en firmwarestatus, YAML-voorbeeld per scherm,
  veldentabel met limieten, iconenlijst met zoekveld (tikken kopieert de naam), kleurstalen,
  gedrag, events met een wacht-op-de-knop-voorbeeld en tips. `copyText` kreeg een label.

## Geautomatiseerd

- Python: 103 tests OK; nieuw `tests/test_alerts_reference.py` houdt de referentie gelijk
  aan beide profielen (velden en typen van `show_alert`, `ALERT_*_MAX`, event en eindes),
  `tile_palette.h`, `alert_overlay.h` en `tile_icons.py`, en controleert de inventaris via
  de aiohttp-testclient (`alerts` alleen in de volledige inventaris, actienamen per scherm,
  `None` zonder apparaatnaam).
- `node --check app.js` schoon; `generate_packages.py --check` onveranderd groen.

## Handmatig

- Lokaal met een nep-Home Assistant (twee schermen: firmware 0.2.31 en 0.2.29): dialoog
  geopend, alle secties nagelopen op 1280 px en op telefoonbreedte, zoekveld gefilterd,
  geen consolefouten. Niet getest in Ingress op een echte HA; de pagina gebruikt dezelfde
  inventaris-API als de rest, dus daar verandert niets aan.
