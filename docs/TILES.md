# Configuring tiles for your Home Assistant

Edit `device.yaml`, within the existing `substitutions:`. You don't need to
touch the base code for the examples below. Every change requires a rebuild
and reflash. Go through the [installation](../README.md) first.

## Collecting entity IDs

In HA, open **Settings → Tools → States** (**Developer tools → States** on older
HA versions). Find the entity you want and
copy the full ID, for example `light.kitchen`. Check its state and
attributes. The tile's title is free to choose; the entity ID must be exact.
Under **Settings → Tools → Actions**, test the intended action with that entity.
An action can control the real device: choose a suitable test.
See [HA actions](https://www.home-assistant.io/docs/scripts/perform-actions/).

First make a list of position, title, entity ID, tap action, and long-press action.
Use existing HA scripts/scenes for multiple devices or complex logic.

## Positions and limits

| Positions | Intended use in this firmware |
|---|---|
| 1–6 | Main tiles; light, switch, scene, script, fan, cover, and climate |
| 6 | Also the vacuum card; the card is specifically tied to tile 6 |
| 7 and 9 | Extra light tiles with brightness/color control |
| 8 and 10 | Extra status/action tiles; no full number/climate card |

`TILE_COUNT: "1"` through `"10"` determines how many **consecutive** positions are
visible. Up to and including six, Previous/Next stay hidden; from seven on, 1–6 sit
on page one and the rest on page two. Free scrolling has been replaced by fixed
pages. Leave unused positions on the neutral example entities.
Don't change grid dimensions to add tiles.

## Key settings

| Setting (`N` = tile number) | Meaning |
|---|---|
| `TILEN_ENTITY` | Entity the action is performed on |
| `TILEN_STATE_ENTITY` | Entity for status/attributes; usually the same one |
| `TILEN_TITLE` | Short title, ideally about 12 characters max |
| `TILEN_TYPE` | Domain, for example `light`, `scene`, `climate` |
| `TILEN_TAP_ACTION` | `auto`, `toggle`, or `custom` for the examples below |
| `TILEN_TAP_SERVICE` | Explicit HA action when using custom, for example `script.turn_on` |
| `TILEN_LONGPRESS` | `none`, `slider`, or (tile 6 only) `vacuum` |
| `TILEN_LONGPRESS_SLIDER` | `auto`; picks a suitable control based on type |
| `TILEN_LABEL_ON/OFF` | Text for a simple on/off status |

The example config also fills in the remaining fields. Leave those as they are unless
you deliberately use them. For a scene/script, `STATE_ENTITY` can follow a real light or
switch; a scene itself has no reliable on/off status.

## Color, white temperature, and brightness

An RGB/color light with `LONGPRESS: "slider"` opens the light card directly.
It contains up to three horizontal sliders:

- **Color:** a continuous rainbow (0–360° hue, 100% saturation).
- **White temperature:** warm to cool, within the minimum/maximum Kelvin values
  the light reports in Home Assistant.
- **Brightness:** 1–100%; on/off stays available via the tile.

The color and white sliders only appear if `supported_color_modes` supports
them. A dimmable-only light keeps the regular brightness card.
No extra tile settings are needed. For a light, use the same
`ENTITY` and `STATE_ENTITY`. The color card follows `hs_color`, `color_temp_kelvin`,
`min_color_temp_kelvin`, and `max_color_temp_kelvin` of the controlled entity.
If the temperature range is missing, the white slider waits for HA data.

When opened, the card takes over the last received light values. Dragging shows
the chosen value locally; only releasing sends a single command. Color selects the
color mode, white temperature selects the white mode. Brightness doesn't change the
color mode. The rainbow controls hue; pastel/saturation isn't a separate control.
The hue list is sent to HA via ESPHome `data_template`; see the
[official API documentation](https://esphome.io/components/api/#homeassistantaction-action).

The shared implementation lives in `light_controls.h`. Both screen profiles
use it; the Guition has larger controls, the CYD a compact card.
`diagnostics/run_ui_test.py` also checks the slider events with swapped-in
callbacks: no real HA actions. For a safe 40-second preview card:

```sh
python diagnostics/control_ui.py light_controls_preview --host guition-wallbox.local --name guition-wallbox
```

For the CYD, use its own host and name. During this preview card,
light commands are suppressed and the card disappears on its own. Then test the
actual light: long press, choose a color, choose white, adjust brightness,
close, and reopen. Check the physical light and the values in HA.

## Examples

Change these keys in the existing map; don't paste `substitutions:` a second time.

Dimmable light at position 1 (tap toggles, long press opens the control):

```yaml
  TILE1_ENTITY: "light.kitchen"
  TILE1_STATE_ENTITY: "light.kitchen"
  TILE1_TITLE: "Kitchen"
  TILE1_TYPE: "light"
  TILE1_TAP_ACTION: "auto"
  TILE1_LONGPRESS: "slider"
```

A non-dimmable light/switch gets `LONGPRESS: "none"`. Color control
requires that the light supports the relevant color mode.

Scene at position 3:

```yaml
  TILE3_ENTITY: "scene.evening"
  TILE3_STATE_ENTITY: "light.kitchen"
  TILE3_TITLE: "Evening"
  TILE3_TYPE: "scene"
  TILE3_TAP_ACTION: "auto"
  TILE3_LONGPRESS: "none"
```

Script at position 4:

```yaml
  TILE4_ENTITY: "script.all_off"
  TILE4_STATE_ENTITY: "script.all_off"
  TILE4_TITLE: "All off"
  TILE4_TYPE: "script"
  TILE4_TAP_ACTION: "auto"
  TILE4_LONGPRESS: "none"
```

AC at position 5 (long press opens climate):

```yaml
  TILE5_ENTITY: "climate.living_room"
  TILE5_STATE_ENTITY: "climate.living_room"
  TILE5_TITLE: "AC"
  TILE5_TYPE: "climate"
  TILE5_TAP_ACTION: "toggle"
  TILE5_TAP_SERVICE: "climate.toggle"
  TILE5_LONGPRESS: "slider"
```

Check `min_temp`, `max_temp`, `target_temp_step`, `hvac_modes`, and the
supported fan/swing modes in HA. This card is primarily for a **single
setpoint**, not a thermostat that only uses a temperature range.
Test your integration's modes individually; a different AC unit
may have different capabilities.

Robot vacuum at position 6 (tap starts; long press opens the card):

```yaml
  TILE6_ENTITY: "vacuum.robot"
  TILE6_STATE_ENTITY: "vacuum.robot"
  TILE6_TITLE: "Vacuum"
  TILE6_TYPE: "vacuum"
  TILE6_TAP_ACTION: "custom"
  TILE6_TAP_SERVICE: "vacuum.start"
  TILE6_LONGPRESS: "vacuum"
```

The vacuum card uses start/pause/stop/return/locate and the
speed names `quiet`, `balanced`, `turbo`, `max`. Compare these with the
supported actions and `fan_speed_list` in HA. The speed buttons need to be
adjusted in the base code if your integration expects different names.
This is not a universal card for every robot model.

## Enabling and checking actions

After calibration and HA pairing, set `DIRECT_ACTIONS: "true"`. In the
ESPHome integration, grant permission for Home Assistant actions. Don't use an
HA automation that also runs the same screen action again separately; that can
cause a double toggle. With `DIRECT_ACTIONS: "false"`, the action sensor can
still publish events for your own automations.

Flash `device.yaml` and test one tile at a time: correct status, correct physical
action, feedback from HA, and long press. Also check that external
changes in HA appear on the screen. Then run the
[acceptance test](ACCEPTANCE.md).

The icon fonts contain the tile icon set from `screen_manager/app/tile_icons.py`;
pick `TILEn_ICON` from there. To add a different icon, add it there and run
`tools/generate_icons.py`. The bundle includes the fonts locally. The older, more extensive
[TILE_CONFIGURATION.md](../TILE_CONFIGURATION.md) also describes the removed
scroll profile as a historical record; don't use it as hardware/layout instructions for this board.
