# Test results app 0.2.67 / firmware 0.2.58 (2026-09-17)

GitHub issue #7 asked for a specific action on tap, such as `cover.toggle` instead of the cover card. Max wanted the
answer taken from Home Assistant rather than from lists in ESP Screens, the same for names, words and icons, and
existing screens untouched. This release offers what Home Assistant says each entity can do (On / off for covers among
it), lets a tap perform any action Home Assistant offers under its own names, and shows Home Assistant's numbers, words
and icons. See CHANGELOG 0.2.67 and the compatibility note in docs/RELEASING.md. Built on 0.2.66 (`d60a0f1`).

## Home Assistant, read first

- Home Assistant core: `cover.toggle` needs OPEN+CLOSE and stops a moving cover; `media_player.toggle` needs
  TURN_ON+TURN_OFF; `climate.toggle` either. `get_services_for_target` exists since 2025.12 (PR 157334). `get_services`
  has carried only untranslated names since 2025-10-24; the names come from `frontend/get_translations`.
- Home Assistant frontend: the tile card's tap defaults to more-info and hides Toggle where `canToggleState` says no;
  the button row disables a button only when it is `unavailable`, not `unknown`.
- Frontend `src/common/entity/compute_state_display.ts` and `compute_attribute_display.ts`: a state's word is the
  integration's for the translation key, then the domain's for the state's device class, then the domain's, else the raw
  state; an attribute value's (a robot's fan speed) the same under `state_attributes.<attribute>`.
- Frontend `src/data/icons.ts` (`getEntityIcon`) and `src/common/entity/state_icon.ts`: the entity's own icon first (the
  registry's, which Home Assistant also writes into the state's `icon`), then the integration's icon for the translation
  key, then the icons the frontend picks in code (a tracker on a router or Bluetooth, an input_datetime with only a date
  or a time, an update, the sun), then the domain's icon for the state's device class, else `_`. Within one entry: the
  state's icon, then for a number the highest `range` step not above it (below the lowest step the default), then the
  default. Home Assistant 2026.9.2 has range icons for two sensor classes: battery (a step every 10 %) and wind
  direction.
- Max's dashboard (read only, nothing saved): Tap behaviour, Icon tap behaviour and Add interaction (hold, double tap).
- Max's Home Assistant 2026.9.2 (read only): `media_player.sonos_badkamer` has `supported_features` 8321599, without
  TURN_ON or TURN_OFF, while ESP Screens offered On / off for every media player.

## Automated

- Python (`.venv-portal`): 377 tests OK (337 on 0.2.66).
  - New `tests/test_ha_capabilities.py` (12): target filters and field filters on real action descriptions, what the
    editor offers per entity, refusing only new settings, lenient loading of a newer app's setting, the per-entity cache
    and its invalidation, the fallback for a Home Assistant that answers "Unknown command", tile events, and `GET
    /api/capabilities`. `test_extended` and `test_cover_card` follow the new rules.
  - New `tests/test_tap_actions.py` (12): what a stored action may hold (names of any integration, field limits, no
    target keys, no NaN), dropping a stale action, the wire form (text as data, numbers, booleans and lists as
    `from_json` templates that decode back to the value), tile events and the layout sensor, Home Assistant's names and
    per-entity fields in the list (a cover without SPEED gets no speed field), the values a `state` selector offers (a
    Sonos source list), leaving out actions that only return data, what a save refuses (not offered, a missing required
    field, an unknown field, data-only actions) and what it keeps (a saved action Home Assistant dropped later), the
    translations fetch and its fallback, Claude's tile event, and `GET /api/entity-actions`.
  - New `tests/test_state_words.py` (9): rounding by display precision (half up, no thousands separator, "-0.0",
    non-numbers and missing precisions unchanged, sensors only), Home Assistant's word picked as its frontend does with
    real entries of Home Assistant 2026.9.2, words only where the screen showed the raw state and never for numbers or
    binary sensors, the tile message carrying the rounded value and `x.w`, the top bar and its editor preview ("Vacuum
    and mop", while Returning, Auto and Motion stay), a history timeline's legend and heading words, a vacuum card's
    chips ("Off (raised brush)", while Normal stays, values unchanged), and everything as before without translations.
  - New `tests/test_ha_icons.py` (7): the frontend's order with real entries (a closed blind, a playing speaker, an open
    door, an integration's icon first, the device class from the state), batteries and wind directions by their range
    steps (45 % `battery-40`, 5 % `battery-alert`, -1 and `unavailable` `battery-unknown`, 2.1e2 degrees
    `arrow-top-right`), the icons the frontend picks in code, only glyphs the fonts carry reaching the screen (none for
    weather, sun and screen tiles), every default, state and range icon of those entries in the fonts, the tile, the top
    bar and the editor list showing the same icon, and everything as before without Home Assistant's icons.
- C++ (`clang++ -std=c++17 -Wall -Wextra -Werror -I.`): 17/17 PASS. `test_tile_controls` transcribes `event()` of
  firmware 0.2.56 and routes 1020 combinations (22 domains x auto/detail/toggle/none/action x 5 states x tap/hold, and
  `action` without an action) through the new `tap_route`: every one the same, `busy` included; only a short tap with
  `toggle` on the 16 domains that could not have it now sends `<domain>.toggle`, and a tile with an action gets
  `CUSTOM`. `valid_action` takes names of any integration. `test_runtime_model`: a scene, button or input button that
  never ran is available; an unknown light is not.
- `generate_icons.py --check` (150 pickable icons, 309 glyphs) and `generate_packages.py --check` green.

## Existing screens stay as they are

- `local_actions` against Home Assistant's own `get_services_for_target` on Max's installation: identical action lists
  for 30 of 30 entities (covers, media players, lights, climate, scripts, scenes, buttons, numbers, selects, a timer).
- Four stored layouts (two production screens saved from the bench, the demo's two screens) load the same with the old
  `validate_layout`, the new one and the new one with `stored=True`, and all 49 tile messages encode byte for byte the
  same as with `core.py` of 0.2.65 and of 0.2.66 when Home Assistant gives no words or icons.

## Firmware on the Mac (host builds, driven over the ESPHome API)

`tap_host.py` sends a layout to host builds of both packages (own device names, API ports 6167 and 6168), with every tap
choice in the state messages themselves, taps and holds tiles like a finger does, and captures the Home Assistant
actions the screen sends. The test client answers the screen's calls like Home Assistant does
(`send_homeassistant_action_response`). **Guition 26/26, CYD 26/26**, on builds with the final fonts.

| Check | Guition | CYD |
|---|---|---|
| Tap on a garage door with On / off: `cover.toggle` with its `entity_id`, no card | pass | pass |
| Hold that garage door: its card opens, nothing is sent | pass | pass |
| Tap on a cover without On / off: its card opens, nothing is sent | pass | pass |
| Tap on a lamp, a switch: `light.toggle`, `switch.toggle` | pass | pass |
| Tap on a running timer, a scene: `timer.pause`, `scene.turn_on` | pass | pass |
| Tap on an input button that never ran (`unknown`): `input_button.press` (before: ignored, "Unavailable") | pass | pass |
| A saved On / off on a speaker: still `media_player.toggle` | pass | pass |
| Tap on a sensor: its card opens, nothing is sent | pass | pass |
| Tap on the skylight set to Set cover position 50: `cover.set_cover_position`, `entity_id` in data, `position: {{ "50" \| from_json }}` in data_template, a call id, no card | pass | pass |
| Home Assistant refuses: the tile says Refused, and its state again after four seconds | pass | pass |
| Holding the skylight opens its card and sends nothing | pass | pass |
| Home Assistant succeeds: the tile keeps waiting for the state, no Refused | pass | pass |
| No answer: the call is watched, and let go after eight seconds without Refused | pass | pass |
| A status sensor with `x.w` shows "Rinsing"; without a word its state as before ("spinning"); a cover that opens "Opening" | pass | pass |
| No errors in the log | pass | pass |

## ESP Screens

The real add-on from this tree on a demo home whose fake Home Assistant answers from Max's real action descriptions,
English action names, translations and icon resources, in the browser pane:

- Opening the kitchen screen asks `GET /api/capabilities` once for its 15 tiles (200).
- Sonos with a saved On / off: the choice stays, with the warning "Home Assistant can't turn this on and off, so a tap
  does nothing. Choose another option." Choosing Automatic removes On / off and the warning; saving: PUT 200.
- TV (on and off): On / off offered. Garage door: On / off offered, no small slider; choosing On / off on this screen's
  firmware 0.2.33 shows "The screen switches this from firmware 0.2.58: press Update on the screen. Until then a tap
  opens its card."
- Skylight (position only): no On / off, a small slider; made double-width it takes Position slider, and the direct
  control offers only Position slider and None.
- Cooker hood light with a saved small slider, washing machine status with a saved graph: both kept, with a warning.
  Clicking the warning text changes nothing (clicking the old hint inside a label pressed the first choice: reproduced
  with the old markup).
- The skylight, On tap → Perform action: the list shows Set cover position and Home Assistant's general actions, each
  with its id and description; open, close and toggle are not there because the skylight lacks them. The screen's old
  firmware adds "The screen performs an action from firmware 0.2.58: press Update on the screen. Until then a tap works
  as Automatic."
- Choosing Set cover position shows Position (%) with "Target position." and "Home Assistant needs Position." until 50
  is typed. Saving: PUT 200, and the skylight's state message carries
  `{"tap":"action","act":{"s":"cover.set_cover_position","t":[["position","{{ \"50\" | from_json }}"]]}}`.
- The living room's Sonos: searching "source" leaves Select media player source; its Source field offers the speaker's
  own sources (Not set, TV, NPO Radio 2, Qmusic) and "Home Assistant needs Source." until one is chosen. The state
  message then carries `"act":{"s":"media_player.select_source","d":[["source","Qmusic"]]}`.
- The ceiling light, Turn on light: Color is a colour picker, Color temperature and Brightness are numbers, Color name a
  list, the object fields text with Home Assistant's examples; Transition, Effect and Flash are not offered because the
  light lacks those features. Orange at 80 % sends `"t":[["rgb_color","{{ \"[255,136,0]\" | from_json
  }}"],["brightness_pct","{{ \"80\" | from_json }}"]]`. Every field has Home Assistant's field name as its accessible
  name.
- `GET /api/inventory` (with a phone battery and tracker added) gives the battery at 64 % `battery-60`, the phone
  tracked by the router `lan-connect`, the playing Sonos `speaker-play`, the TV that is off `television-off`, the closed
  garage door `garage`, the skylight `window-open`, the cooker hood light that is off `lightbulb-off`, the living room
  climate `thermostat`, the washing machine status Home Assistant's `eye`, and no icon for the weather, which the screen
  draws. The kitchen screen's mockup draws the same icons on its tiles.
- Inspector → Read current data: every tile's status in Home Assistant's words (Partly cloudy, Active, On, Playing,
  Home, Off, Closed, Open); numbers, a scene's time and states without a word (the demo washing machine's rinsing, an
  input button's unknown) as they come.
- The entity list: "front door" finds the door sensor but no longer the lock; the input button has the Action badge and
  shows under Actions. Saving the changed kitchen screen: PUT 200, "Saved. Your screen is being updated."
- No console errors apart from the live update stream that dropped while the demo server restarted.

## Real Home Assistant, read only

The app's own `HomeAssistant` class and lookups against Max's Home Assistant 2026.9.2:

- 79 action domains and 1571 action names; `get_services_for_target` answered, eight entities in 30 ms from the Mac and
  0.2 ms from the cache. Sonos: no On / off, volume and playback controls. Curtains: On / off, buttons and position.
  Cooker hood light (on/off only): On / off, no small slider.
- Perform action: Curtains list Close cover, Open cover, Set cover position (Position, required), Stop cover, Toggle
  cover, then the general actions. Sonos Badkamer: 29 actions, its media player actions first, Select media player
  source among them with Source as a choice of its own stations. Bathroom light: Toggle light, Turn off light and Turn
  on light with Transition, Color, Color temperature, Brightness, Effect and more for its colour modes.
- Words: of 743 entities a tile or the top bar can show, 11 read differently in the top bar, all as Home Assistant shows
  them: a washing machine's "Stop" is "Stopped", an Oral-B brushing mode "Daily_clean" is "Daily clean", a Voice
  satellite's "no_wake_word" is "No wake word", a select's "0" is "Off". The Roborock S8's chips stay as they were:
  every option it offers has a short label of the screen's own.
- Icons: of the same 743 entities, 458 now get Home Assistant's icon on the screen, where they had the screen's own
  before. 52 keep their own icon and 3 are drawn by the screen (weather, sun). The other 230 keep the screen's icon as
  before: 154 whose own icon the fonts lack, and 76 whose integration's icon they lack (Sonos, Roborock, WLED, Oral-B,
  EZVIZ and others). All 29 battery sensors follow their level: 35 % `battery-30`, 55 % `battery-50`, 100 % `battery`,
  unavailable `battery-unknown`.

## Builds

Check profiles with the generated packages and local components, ESPHome 2026.6.2 (the app's own); no errors or compiler
warnings.

| Profile | RAM | Flash | vs 0.2.66 |
|---|---|---|---|
| CYD | 23.9% (78,284 B) | 79.1% (1,452,291 B) | +344 B RAM, +28,476 B flash |
| Guition | 25.6% (83,860 B) | 21.0% (1,707,603 B) | +344 B RAM, +42,688 B flash |

The RAM is the action answers (a watch list of four calls) and the words; the icons cost about 25 KB (CYD) and 39 KB
(Guition) of flash, and the CYD keeps 383 KB free in its app partition.

ESPHome 2026.8.1 (what Device Builder installs now) builds both profiles too: CYD flash 79.6% (1,461,375 B), Guition
21.2% (1,720,695 B). Its newer compiler warns about `JsonVariant` loops and `%d` with `int32_t`, all in code 0.2.66
already had.

## Not tested

- Physical screens: not flashed (another session was working on the bench screens). A real tap on a cover with On / off,
  a real Perform action and refusal, and the new icons on the panels themselves are Max's check after the update.
- The page in Home Assistant's Ingress frame, and the app on the Yellow (the capability lookups there are slower than
  from the Mac).
