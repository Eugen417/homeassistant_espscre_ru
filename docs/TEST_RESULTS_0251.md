# Test results app 0.2.51 / firmware 0.2.43 (2026-09-16)

Tiles from a Home Assistant event, a sensor that reads back what a screen shows, and the Claude skill
that teaches both. App only: the firmware, the protocol and the stored layouts are unchanged.
See CHANGELOG 0.2.51 and RELEASING "Compatibility 0.2.51".

## Automated

- Python (`.venv-portal`): 193 tests OK. New `tests/test_tile_events.py` (18 tests): finding a screen by
  device name, shown name, area or title; adding, changing, moving, removing and ordering tiles; a
  control or a forecast widening its own tile; a widened tile keeping its spot only when the cell beside
  it is free; the refusals (unsupported entity, unknown page, row or column, an occupied spot, a full
  page, twenty tiles, an entity that is not on the screen); the snapshot a screen publishes; and one
  event end to end through `Manager.tile_loop`, which saves `screens.json`, answers with
  `esp_screens_tile_result` and publishes the layout sensor. A refused event writes nothing.
- `tests/test_claude_skill.py` checks the new section: the four events, the result event, the sensor,
  what each kind of entity can do, the rule to wait for a yes, and both new YAML examples.
- C++ (`clang++ -std=c++17 -Wall -Wextra -Werror -I . -I components/smart_display`): 13/13 PASS,
  unchanged this release.
- `generate_packages.py --check` green; the board profiles and packages are untouched.

## Against the running Home Assistant

Without sending anything to a screen:

- Subscribed to `esp_screens_add_tile` over the websocket, fired that event over the REST API the same
  way Claude in Home Assistant does (`POST /api/events/...`, 200) and received it with its data intact.
- Published `sensor.esp_screens_probe` with tile attributes over the REST API (201), read it back
  unchanged, and deleted it again (200). That is the path `Manager.publish_layouts` uses.

## Not tested / for the owner

- The full round trip with the app running on Home Assistant: update ESP Screens to 0.2.51, install the
  skill again from Settings → Claude, and ask Claude for a tile. The screens themselves need no update.
- Ordering by how often something is used: Claude reads that from Home Assistant's own history.
