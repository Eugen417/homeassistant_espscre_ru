# Publishing updates without replacing user configuration

## Layout of the code and data

- `main` is the only distribution branch for the app **and both** firmware packages.
  The old Guition branch is no longer updated; all board work goes to main.
- `screen_manager/config.yaml` holds the app version. Bump it on every app release.
  A Git push alone isn't enough to offer an existing app an update.
- The app image contains only code. Layouts live in `/data/screens.json`
  (`version: 1`, `screens: {...}`). An update/rebuild preserves this volume data.
- The device's own ESPHome YAML contains the name, Wi-Fi references, and unique API/OTA keys.
  Shared packages contain no secrets, fixed owner entities, or Wi-Fi.
- CYD calibration lives in ESP32 preferences. Preserve the preference key, structure,
  and partition layout, or write an explicit migration.
- The tile protocol uses `v: 1`. Keep older fields and domains usable.
  A changed storage version must get a tested migration with a backup.
  The app refuses unknown versions instead of overwriting the data blank.

## For every release

1. Update the source profiles and shared components. Run
   `python3 tools/generate_packages.py`; never edit `packages/*.yaml` directly.
   The editor is the Vue app in `web/`: after a change under `web/src`, run
   `cd web && npm ci && npm run build` and commit `screen_manager/app/static` with it
   (that folder is the build output; never edit it by hand).
2. Run all Python tests with aiohttp installed, all `tests/*.cpp`, the
   generator with `--check`, and compile both Easy Setup profiles. Do that
   sequentially: profiles with the same `DEVICE_NAME` share one build folder, and a
   parallel build can make an upload pick the wrong `firmware.bin`. Check that no
   secrets are in Git.
3. Test app start, saving, restarting/updating with existing layouts,
   reconnecting to HA, and an ESP restart. Test a new card on real
   hardware. A good build doesn't replace physical touch acceptance.
4. Bump the app version and firmware project version; write the CHANGELOG and concrete
   test results. Only publish compatible changes directly to main.
5. Commit and push main (the only release branch). Create an immutable tag
   `screens-vX.Y.Z` from the same commit. Test the remote YAML in an empty folder:
   all components/fonts must be fetchable via GitHub.
6. The user checks the App store for updates and updates ESP Screen Manager.
   For new screen features: existing ESPHome device → Install → Wirelessly.
   The existing YAML stays in place; `refresh: 0s` fetches current code on every build.

## Local development

Copy only `screen_manager/` to the shared `addons/esp_screen_manager/`.
Reload the App store, install the local version, and rebuild after code changes.
A local test version has a different add-on identity than the GitHub version; the
data doesn't move over automatically. For the final installation, test the
GitHub version and turn off the local version to avoid two writers.

For backend tests on a development machine:

```sh
python3 -m venv .venv-portal
.venv-portal/bin/pip install aiohttp PyYAML
.venv-portal/bin/python -m unittest discover -s tests
```

For the editor (`web/`, Vue 3 + Vite + TypeScript): `cd web && npm ci`, then `npm run dev` serves
http://localhost:5173 with hot reload and proxies `/api` to a server on 127.0.0.1:8099 (SCREEN_DEV or a demo
home). `npm run check` type-checks, `npm run build` writes the page into `screen_manager/app/static`
(clearing `assets/` first). Vite names every file after a hash of its content, which is what keeps a browser
from combining an old script with a new page; `server.py` serves `index.html` as built and `/assets/`.
Everything the page asks for is a relative URL (`api/...`, `./assets/...`), so it works under Home
Assistant's ingress path as well as on a bare localhost. The Python tests read the source through
`tests/editor_sources.py`.

A temporary development server supports `SCREEN_DEV=1`, `HA_API` (ending in
`/api`), `HA_TOKEN_FILE`, and `SCREEN_DATA`. It only binds on localhost. Never put a
token in source code, URLs, or Git. Production uses Supervisor and only accepts
the Ingress proxy address; there is no additional public port.


### Settings compatibility 0.1.2

`settings` is an optional object within a screen layout and within the existing
`v: 1, op: layout` message. A missing object preserves prior behavior. Old
firmware ignores this extra field and keeps receiving tiles. The management page reports
that new firmware is needed. An old browser that only saves tiles doesn't wipe
saved settings. Unknown settings are refused.

Firmware uses a separate preferences key `0x53435231` with a fixed
version-1 structure (eleven int32 fields and a uint32 version). Don't change the key,
structure, or version without a migration. Only changed settings are saved;
periodic repeats cause no flash writes and no new idle timer.

### Compatibility 0.2.0

Storage version and tile protocol stay 1. Optional `tiles[].options`, `o` in
status messages, `history`, and `inbox` in the layout message are additive. An
older browser that doesn't send options keeps the existing options for the same
entity. Older firmware ignores extra fields. New entity domains do
require firmware 0.2.0; when rolling back to an older app, restore the data backup
first, since that app can't yet load new domains. Don't silently
overwrite old storage. Existing preference keys stay unchanged.

The app now uses the official ESPHome container, maps `homeassistant_config`
to `/homeassistant`, and requests UART/USB for the port the user chose.
Config/secrets stay in the device's own HA config folder; builds in `/data`. App updates
don't replace these folders. Native number settings report changes via
`esphome.screen_setting`; the manager validates the inbox, key, and value.

The built-in CLI stores re-downloadable caches under `/data/build` and
`/data/platformio`. Only these folders are excluded from app backups via
`backup_exclude`. `/data/screens.json` and the device's own ESPHome configurations remain
backup data; never exclude `/data` as a whole.

### Compatibility 0.2.7

Twenty runtime tiles require firmware 0.2.7+. The manager checks that version
before saving and sending larger layouts. Existing layouts are
preserved on older firmware. Old app versions don't support more than ten;
first bring the layout back down to ten, or restore their data backup on rollback.
`settings.swipe_pages` is additive in storage version 1. On the wire, it sits outside
the unchanged eleven-field settings object. Existing preferences stay the same;
only swipe uses a new, separate uint32 key `0x53575031`, off by default.

### Compatibility 0.2.9

Guition rotation is additive as `settings.rotation` in app data, restricted to
0/90/180/270. Old management pages that omit the field keep the stored
angle. On the wire, `rotation` sits alongside the unchanged eleven-field settings object.
The firmware stores the angle as a separate uint32 at key `0x524F5431`; the
existing Settings structure and CYD calibration stay unchanged. Only the
Guition activates the rotation callback. The diagnostic entity `Guition schermtype`
makes this capability recognizable, even when the screen is offline or renamed.

### Compatibility 0.2.10

`tiles[].options.background` is an optional palette name within storage version 1.
On the wire, this sits in the existing `o` object. Older firmware ignores the
field; newer firmware falls back to the normal colors for unknown names.
The app only accepts the light palette from `TILE_BACKGROUNDS`. An old
editor that omits `background` keeps the stored color for the same entity;
explicit `auto` restores the default. No changed preference structure or
keys, no new firmware flash needed for later color choices.

An old app release from before 0.2.10 doesn't know this option field. When rolling back
the app, restore the corresponding data backup; don't wipe or ignore
unknown options just to open an old storage file anyway.

### Compatibility app 0.2.15 / firmware 0.2.16

Storage version and tile protocol stay 1. `tiles[].options.background` gets the
extra palette name `none` ("None"): no card behind the tile content. Older
firmware doesn't know the name and falls back to the normal card; because that fails
invisibly for the user, the manager only sends such a layout once firmware
0.2.16 or newer is detected (`min_firmware`), holding it back in the meantime. The firmware only hides
the fill and border (`bg_opa`/`border_opa`); sizes, padding, and the
pressed feedback on the PRESSED state stay the same. The analog clock only changes
the drawing (ticks, numerals, calendar block on single tiles); no
new field is needed for it. No changed preferences or keys.

### Compatibility 0.2.16 / firmware 0.2.17

`FIRMWARE_VERSION` in `screen_manager/app/core.py` is the firmware that belongs to this
app; `tests/test_updates.py` requires it to equal
`SCREEN_FIRMWARE_VERSION` in both board profiles and packages. Bump them together.
A screen with a lower `Schermfirmware` gets an update offer; a build
fetches `main`, so publish firmware and app in the same commit.

Firmware 0.2.17 adds the diagnostic text sensors `Apparaatnaam`
(`${DEVICE_NAME}`) and `IP-adres` (`wifi_info`). The app links a screen
to the profile with the same `esphome.name` via `Apparaatnaam`; older firmware
falls back to a single profile with a matching `friendly_name` and a manually entered
address. Don't rename these sensors without also updating `core.discover`.

Update status lives in `/data/updates.json` (`version: 1`: `auto`, `hosts`,
`results`, `last_round`), separate from `screens.json`. Unknown versions are
refused. The round runs one screen at a time, waits for `Schermfirmware >=`
target version and one minute of stability, and stops on the first error. Only the
nightly round writes a `persistent_notification` in HA.

### Compatibility 0.2.18 / firmware 0.2.18

Storage version and tile protocol stay 1. `tiles[].options.icon` is additive:
`auto` or a name from `screen_manager/app/tile_icons.py`. On the wire, the
manager sends only the resolved codepoint in hex (`F06B5`) in `o.icon`: the chosen
name, or, for `auto`, the `mdi:` icon from the HA attributes if it's in the set.
Older firmware ignores the field; newer firmware checks whether the glyph is in the
font and otherwise falls back to the domain icon. Hence no `min_firmware`.
An old editor that omits `icon` keeps the stored choice.

`tile_icons.py` is the single source list. `tools/generate_icons.py` writes the glyphs
as a YAML anchor into the three MDI fonts of both board profiles and builds
`web/src/assets/tile-icons.woff` for the editor (with left bearing equal to xMin, otherwise
icons sit off-center in the browser); run `generate_packages.py` afterward.
The `MDI_GLYPH_*` substitutions are retired: `TILEn_ICON` in manual
profiles must come from the set. No changed preferences or keys.

### Compatibility 0.2.72 / firmware 0.2.61

Firmware only (both board profiles; `cyd_ui.h` gains a comment); the app raises `FIRMWARE_VERSION`. No protocol,
storage or editor change, and an older app drives this firmware as before.

- The `< Previous` and `Next >` bars on `home_page` call `cyd::touch_guard.accept_repeat(millis(), 11 / 12)` instead
  of `accept()`. `accept()` keeps a 600 ms window per tile so one contact can never fire a tile's action twice; on the
  page bar that window dropped the second and third tap of Next, Next, Next, so page 4 took three waits of about
  half a second. `accept_repeat`, the -/+ keys' rule since 0.2.25, still drops a contact shorter than 40 ms (20 ms on
  the Guition), a contact that moved, a contact that already counted, and a tap within 150 ms of the last accepted
  tap on the same button (bounce). `show_page` already drops a fill still under way when the next page is asked for,
  so nothing else changed. `tests/test_cyd_ui.cpp` covers the page buttons.
- The settings page's own pager (`settings_screen.h`) never used the guard and is unchanged. Tile taps, cards, keys,
  chips and sliders keep `accept()` and its window.

### Compatibility 0.2.71 / firmware 0.2.60

Firmware only (`runtime_model.h`, `runtime_tiles.h`); the app raises `FIRMWARE_VERSION`. No protocol, storage or
editor change, and an older app drives this firmware as before.

- `Tile::hold_slider(now, value)`, called from `commit_slider` for a light (brightness 0-255), fan (percent) and
  media player (volume 0-1), writes the sent value into the attribute (`slider_field`) and keeps what Home Assistant
  reported in `slider_real`. A cover is left out: its position slider follows the blind as it moves. On an off light
  or fan it also calls `optimistic(true)`, like a tap.
- `Tile::slider_reported(now)` runs in the state ingestion right after the attribute is parsed. It ends the hold when
  the report is within 3 % of the target (`slider_span`), when `slider_active()` is false (off, unavailable), or on a
  refusal; otherwise it keeps the sent value in front and notes `slider_state_at` when the reported value moved.
- `Tile::slider_holding(now)`: no report yet, hold while `waiting(now)` or once `answered_at` is set; after a report,
  hold for `SLIDER_SETTLE` (1500 ms) since the last movement; never beyond `SLIDER_HOLD_CAP` (8000 ms) from the send.
  The once-a-second tick calls `release_slider()` (puts `slider_real` back) when a hold has run out, and the refusal
  callback does the same next to `undo_optimistic`.
- `slider_value(t)` and the tile's `%` label read the attribute, so every slider and label agrees without a change of
  its own. Firmware before 0.2.60 keeps snapping to each report; the app sends nothing new.

### Compatibility 0.2.70 / firmware 0.2.59

Firmware only (`runtime_model.h`, `runtime_tiles.h`, `tile_controls.h`); the app raises `FIRMWARE_VERSION`. No
protocol, storage or editor change, and an older app drives this firmware as before.

- `Tile::waiting(now)` replaces `awaiting_action`: pending, not confirmed, not `local_feedback`, and within `BUSY_CAP`
  (3000 ms), or within `BUSY_AFTER_ANSWER` (800 ms) of Home Assistant's "it worked" answer (`answered_at`, set by the
  action-response callback). It guards what a finger may do: a second tap, a slider, a key, a chip, the -/+ edit.
- `Tile::loading(now)` is what gets drawn: `waiting` once `BUSY_GRACE` (400 ms) has passed. Home Assistant answered
  every command in 342-599 ms on Max's installation, so a command that lands shows nothing at all; the busy sheet, the
  card's "Command sent..." and its greyed keys only appear for a slow one. The old rule (always busy for 1000 ms,
  150 ms for a switch, then up to 6000 ms without confirmation) is gone, and with it the switch's own window.
- `Tile::optimistic(on)` writes the new stand into `state` and keeps the old one in `optimistic_prev_on`, as Home
  Assistant's `ha-control-switch` flips before the command goes out. A tap that sends `<domain>.toggle` on a light,
  switch, input boolean or fan uses it, and so does a card's switch. `observe` clears the flag, so a state message
  always wins; `undo_optimistic` puts the old stand back on a refusal or when the wait runs out (`end_wait`, which
  also drops a chip's sent value).
- Between the manager's messages nothing redraws a card, so `watch_busy` creates one 200 ms LVGL timer while any tile
  waits: it brings the sheet up after the grace, ends waits that ran out, and deletes itself. `Widgets::busy_drawn`
  keeps a card from being drawn again for nothing.
- `action()` asks for an answer by default now, so the cards' keys, sliders, chips and the -/+ edit get the same
  refusal and "it worked" handling a tap has had since 0.2.67. At most four answers are watched at a time, as before.
- Firmware before 0.2.59 keeps its own timing; the app sends nothing new, so any app drives either firmware.

### Compatibility 0.2.67 / firmware 0.2.58

App (new `ha_catalogue.py`, `core.py`, `server.py`, `header_bar.py`, `history_card.py`, `tile_icons.py`, the editor and
the Claude skill), `runtime_model.h`, `tile_controls.h`, `runtime_tiles.h` and `text.py`; the app raises
`FIRMWARE_VERSION` and both profiles' icon fonts grow. The storage version and the tile protocol stay 1. A tile gains
the tap choice `action` with the option `action`; its state message gains the options `act` and `x.w`, and `o.icon` now
carries most tiles' icon. Without Home Assistant's answers everything behaves as before.

Capabilities (what the editor offers):

- What the editor offers per entity comes from Home Assistant. `HomeAssistant.fetch_services` keeps the whole
  `get_services` answer (it kept only `esphome`), and every `service_registered`/`service_removed` refreshes it after
  the usual second; only an `esphome` one still wakes the sync loop. `HomeAssistant.entity_actions` asks
  `get_services_for_target` per entity (Home Assistant 2025.12+, core PR 157334) and keeps the answer until the action
  list, the registry or the entity's `supported_features`/`device_class` change. A Home Assistant that answers "Unknown
  command" is remembered, and `ha_catalogue.local_actions` evaluates the `target.entity` filters of the descriptions
  instead: every condition of a filter must hold, a `supported_features` entry is a mask the entity must support
  completely, any entry will do. On Max's Home Assistant 2026.9.2 both answers were identical for 30 of 30 entities.
- `ha_catalogue.capabilities` turns the action list into what the editor may offer: `toggle` (`<domain>.toggle`),
  `inline` and `controls` (the Home Assistant action each of our widgets sends, `INLINE`/`CONTROLS`, with a field that
  must pass its `filter`: a light's small slider needs `light.turn_on` with `brightness_pct`), and `displays` (graph for
  a sensor `history_card.kind` draws as a line, forecast for weather with the daily bit or no features yet). `GET
  /api/capabilities?entity=...` returns it for up to 40 entities, null when unknown (no action list, no state).
- `Manager.check_supported` runs before `Manager.save` for the editor's PUT and for tile events. It refuses a tap,
  slider, control or display Home Assistant doesn't support only when that setting is new or changed for the tile; what
  a tile already has keeps saving, and nothing is refused while the capabilities are unknown.
- `validate_layout` no longer limits `tap: toggle` to six domains (only `screen.*` has none); `check_supported` takes
  that place. `validate_layout(data, stored=True)` loads the app's own data leniently: a tile whose options fail
  validation (a newer app's setting after a downgrade) keeps them as stored instead of stopping the app. The editor and
  tile events still validate strictly.
- Firmware: `tile_controls::tap_route` decides what a tap or hold does and `runtime_tiles::event` carries it out. `tap
  == "toggle"` on a short tap sends `<domain>.toggle` for every domain before the domain's own card; before, the 13
  runtime-card domains opened their card first, so a cover could not toggle. `tests/test_tile_controls.cpp` transcribes
  `event()` of firmware 0.2.56 and checks 800 combinations (22 domains x 4 tap choices x 5 states x hold): every one
  routes as before, including `busy`; only a short tap with `toggle` on the 16 domains that could not have it changes.
- Firmware before 0.2.58 opens the card for `toggle` on those domains; the editor says so under On tap and keeps the
  choice. The editor keeps showing a stored choice that Home Assistant no longer supports, with a warning, and hides it
  otherwise; a tile that becomes wide gets the first supported direct control when the domain's default isn't there.
  Hints under a field sit beside the label (`withHint`): a click inside a `<label>` pressed its first button.
- `renderResults` skips `tile: false` entities (the top bar's extra domains), and `input_button` joins the `domains`
  table and the Actions filter.
- `Tile::available()` counts `unknown` as available for `scene`, `button` and `input_button`: their state is the moment
  they last ran, and Home Assistant's button row disables only an `unavailable` one. Since the first runtime tiles such
  a tile said Unavailable and ignored taps until it had run once from Home Assistant; it now says Never run.

Perform action:

- Stored: `options.tap = "action"` with `options.action = {"action": "domain.action", "data": {...}}`.
  `core.validate_tap_action` takes an action name of any integration, at most 8 fields, no target keys (`entity_id`,
  `device_id`, `area_id`, `floor_id`, `label_id`: the target is the tile's entity), JSON values without NaN, each at
  most 400 bytes on the wire and 800 bytes together. `validate_layout` drops `action` when the tap is anything else and
  refuses it on `screen.*`. App 0.2.67 loads such a layout leniently; 0.2.66 and older stop on it (a downgrade only).
- On the wire `screen_options` sends `o.act = {"s": action, "d": [[key, text]], "t": [[key, template]]}` and never the
  stored `action`. Text goes as ESPHome action data; every other value as `{{ "<json>" | from_json }}`, which Home
  Assistant's ESPHome integration renders with `render_complex` back into a number, a boolean, a list or an object
  (ESPHome's action data is text only).
- The list: `HomeAssistant.fetch_services` also reads `frontend/get_translations` (English, `services`) into
  `service_names`; `get_services` has carried only services.yaml's legacy text since 2025-10-24. `GET
  /api/entity-actions?entity=` answers `ha_catalogue.action_choices`: the actions of `entity_actions`, the entity's
  domain first, then its integration, then the rest, each with Home Assistant's name and description and the fields
  whose `filter` fits the entity (sections flattened). An action that must return data (`response.optional` false) is
  left out. A `state` selector gets the entity's values for that attribute (`<attribute>_list`, `<attribute>s`,
  `available_<attribute>s`, `supported_<attribute>s`), so a Sonos source is a choice.
- `Manager.check_supported` refuses a new or changed action that Home Assistant doesn't offer for the entity, that only
  returns data, that sets a field Home Assistant doesn't have for it, or that leaves a required field empty. A saved,
  unchanged action keeps saving. Tile events take `action` and `data` (`tap` follows unless given); the layout sensor
  carries `tap` and `action`.
- Firmware: `receive` parses `o.act` into `Extra::action`, `action_data` and `action_templates` (a valid action name, at
  most 8 pairs, keys up to 32 and values up to 400 bytes), so only a tile with an action holds them.
  `tile_controls::tap_route` returns `CUSTOM` for a short tap with `tap == "action"` and an action; without one it taps
  automatically, as older firmware does with the unknown value (the legacy test now routes 1020 combinations). Holding
  still opens the card. `runtime_tiles::perform` sends `entity_id` plus the data, and the templates as `data_template`.
- Answers: `text.py` defines `USE_API_HOMEASSISTANT_ACTION_RESPONSES`, which ESPHome otherwise sets only for a YAML
  `homeassistant.action` with `on_success`/`on_error`. A tap's action (the tile's own, On / off, the automatic taps from
  `event()`, not the sliders and keys) gets a call id through `watch_call`, at most four at a time. A refusal sets
  `Tile::refused_at`: the tile says Refused for four seconds and stops waiting. A success changes nothing: the tile
  waits for the new state as before. ESPHome keeps a callback until its answer arrives and has no timeout, and Home
  Assistant never answers when the device may not perform actions or is older than 2025.10, so `expire_calls` ends a
  call after eight seconds through `handle_action_response(id, false, "no answer")`, which shows nothing.

Numbers, words and icons:

- `core.rounded_state` rounds a sensor's state in `state_message` by the entity registry's display precision
  (`header_bar.precision_of`: `display_precision`, else `suggested_display_precision`), half up like the frontend,
  without thousands separators because the firmware parses the number, and "-0.0" as "0.0". No precision keeps the state
  as it is, as Home Assistant does. Numbers, climate and other domains are unchanged.
- `HomeAssistant.fetch_services` also reads `frontend/get_translations` (English) for `entity_component` (about 63 KB on
  Max's installation) and `entity` (about 215 KB) into `state_words`, on connect and when actions register.
  `core.state_word` picks a word as the frontend's `computeStateDisplay` does: the integration's word for the entity's
  translation key (`component.<platform>.entity.<domain>.<key>.state.<state>`), then the domain's word for the state's
  device class, then the domain's word; `core.attribute_word` does the same for
  `state_attributes.<attribute>.state.<value>` (`computeAttributeValueToParts`). `ha_catalogue.screen_word` sends a word
  as `x.w` only for cover, media player, vacuum, select, input select and non-numeric sensors, folded to the tile fonts'
  glyphs (`header_bar.clean_text`), at most 32 bytes, and only when it differs from the state.
- The same words, add-on only (any firmware): `header_bar.value` for a select, input select or input text and for a
  state without a word in `STATES` (which stays: Returning, Auto, Up); `history_card.state_label` for timeline legends
  and heading words, after `STATES`; `ha_catalogue.chip_words` for a vacuum card's `l` labels of a mode or water select
  option and of a fan speed (`attribute_word` `fan_speed`) that `VACUUM_LABELS` lacks; the inspector's `word`. Without
  translations every message is as before.
- Firmware keeps `x.w` in `Extra::state_word` (heap only for such tiles). The tile value uses it after the screen's own
  words (Unavailable, a light's brightness, a climate's setpoint, person, sun, timer, scene, script, button, binary
  sensor, On, Off, Cleaning, Docked) and before the unit; `detail_state` after the vacuum words and Unavailable. Wide
  cover and media tiles keep `tile_controls::status_text`, and the cover card its own status line.
- Binary sensors keep firmware 0.2.53's words (Motion / No motion where Home Assistant says Detected / Clear) until Max
  decides. Older firmware ignores `x.w`.
- Icons: `tile_icons.HA_DEFAULTS` adds the 125 icons the fonts lacked that Home Assistant 2026.9 shows for the tile and
  top bar domains: every default, state and range icon of `frontend/get_icons` (`entity_component`) and those its
  frontend picks in code (`stateIcon`: a tracker on a router or Bluetooth). `circle` and `radiobox-blank` are not in
  this repo's MDI font. `tools/generate_icons.py` puts them in both profiles' three icon fonts and the editor font (309
  glyphs); they are not pickable. `HomeAssistant.fetch_services` reads `frontend/get_icons` (`entity_component`,
  `entity`) into `tile_icons.use_ha_icons`. `tile_icons.ha_default_icon` follows the frontend's `getEntityIcon`
  (src/data/icons.ts): the integration's icon for the translation key, then `stateIcon`, then the domain's icon for the
  state's device class or else `_`; within one, the state's icon, then a range step for a number (`Number()`, the
  highest step not above it), then the default. `default_glyph` returns a codepoint only for a glyph the fonts carry,
  never for weather, sun or `screen.*`. `core.tile_icon` uses it after a chosen icon and the entity's own `icon`
  attribute, so `o.icon` now carries most tiles' icon; `header_bar.auto_icon` and the editor's entity list (`discover`)
  use the same before their tables. Firmware before 0.2.58 drops a codepoint its fonts lack (`has_icon_glyph`) and shows
  its own default. Without icon resources (a Home Assistant before 2024.2, a refused request) everything is as before.

### Compatibility 0.2.66 / firmware 0.2.57

Camera images on the Guition (docs/CAMERA.md). Storage, keys, preferences and the settings block are unchanged; the
layout message is unchanged apart from two tile domains, and the `show_alert` action keeps its seven fields.

- New tile domains `camera` and `image` in `core.DOMAINS` and the firmware's `valid_entity()`. `min_firmware()` asks
  0.2.57 for them, and `Manager.save` refuses them for a screen whose board is not in `camera_feed.BOXES` (the CYD)
  before it asks for firmware, so a CYD never hears "update first". The editor hides them for other boards; the top
  bar refuses them (`header_entity`). An `image` tile carries `x.last` like a scene. Older firmware never gets these
  tiles because of `min_firmware`.
- New protocol, all optional: the screen fires `esphome.screen_camera` {inbox, entity} when a camera opens; the app
  answers through `screen_message` with op `camera` {t: `full`|`alert`, e, u}. An empty `u` means no image. For an
  alert the app sends op `camera` with `t: alert` and an empty `u` to screens that can show it (`camera_feed.can_show`:
  a Guition on 0.2.57+) BEFORE `show_alert`, so the card opens with room, and the link after it. `show_alert` has no
  camera field on purpose: Home Assistant requires every field of an ESPHome action, so a new field would break every
  automation calling `esphome.<screen>_show_alert` directly. The camera is only a field of the
  `esp_screens_show_alert` event (`core.alert_camera`, `ALERT_CAMERA_FIELD`).
- The app publishes port 8098 (`config.yaml` `ports`) and serves `GET /camera/<token>.bmp` without the ingress guard.
  Tokens are 24 random URL-safe characters, per camera and size; live links expire 120 s after their last use, an
  alert's still after 30 min; at most 64 links. Only a camera on the screen's layout or in an alert of the last
  30 min gets one. The address is Home Assistant's default adapter from the `network` websocket command, else the
  host of `network/url`; `SCREEN_CAMERA_URL` / `SCREEN_CAMERA_PORT` override it (Docker). A port that cannot be
  bound logs an error and leaves the rest of the app running.
- Images are 24-bit BMP, not JPEG: ESPHome 2026.6.2 decodes a JPEG in one loop call (623 ms for 480x270 on the
  Guition) and a BMP per downloaded chunk (`buffer_size: 4096`). Boxes: full 480x480, alert 392x220, proportions kept.
- Fetching follows the screen: serving a live link starts the next snapshot fetch (`CameraFeed.frame(fresh=True)`),
  one at a time per camera, so the picture changes at the screen's pace. The screen loads every 4 s start to start with
  at least 800 ms free after a load (`camera_view.h`), never under a finger and never in standby. An alert fetches its
  own snapshot (`now=True`), never one kept from an earlier view. A failing camera is asked again after 5-30 s.
- Firmware (Guition profile only): `http_request` (4 s timeout, no TLS), two top-level `online_image`s
  (`camera_image`, `alert_image`, the form 2026.6.2 knows; 2026.7+ reads it with a deprecation note), a hidden LVGL
  `image` seed so `LV_USE_IMAGE` is compiled, the `alert_image_frame` in the alert card, `preview_camera` for
  diagnostics and renders, theme roles `CAMERA_PAGE/INK/NOTE` and paint `camera`. The load hooks only call `set_url()`
  for a new link, since it forgets the ETag. `runtime_tiles.h` builds the full view on `lv_layer_top()`; closing
  frees the image on the next tick. `LV_USE_IMAGE` guards the drawing, so the CYD (no image widget) still compiles;
  the CYD profile binds no hooks and never opens a camera.
- Swipes, the settings hold and Back to page 1 treat an open camera like an open card; `close_cards` (standby,
  Back to page 1, a new layout) closes it, and a new alert closes it first.
- `guition_diagnostics.h` snapshots composite the top layer, so `capture_ui.py` shows alerts and the camera.
- `backlight_fade.h` is gone. It derived from `esphome::ledc::LEDCOutput` to read its channel and bit depth, and
  ESPHome 2026.8 declared that class `final`, so the Guition package no longer compiled in ESPHome Device Builder
  2026.8+ (the app's own ESPHome 2026.6.2 still built it). `set_backlight` now only calls ESPHome's light transition
  (1.5 s to standby, 80 ms to wake); a new call interrupts a running transition as ESPHome does. Both profiles and the
  camera code were compiled with ESPHome 2026.6.2 and 2026.8.1 for this release.

### Compatibility 0.2.65 / firmware 0.2.56

Both board profiles and the editor page; the app raises `FIRMWARE_VERSION`. Storage, tile protocol, preferences, keys
and entities are unchanged.

- Two clocks. `last_touch_ms` stays the standby time: a touch, Wake, `wake_display`, an alert ending
  (`alert_dismiss`) and Auto standby on restart it, exactly as before. The new non-restored `last_use_ms` is Back to
  page 1: the boot, the three touchscreen triggers (`on_touch`, `on_update`, `on_release`), `open_settings` from Home
  Assistant and the UI self test write it, nothing else. Before, Wake reset the one clock both rules read, so an
  automation pressing Wake on every motion held a card or page 2 up indefinitely.
- The Back to page 1 check moved from the one-second interval into the script `back_to_page_1_when_due` (same
  condition, now on `last_use_ms`, still paused while `display_dimmed`). The interval runs it, and so does Wake
  right after `wake_display` on a dimmed screen: a page whose time ran out during standby goes home in the same pass,
  before LVGL draws the old page at full brightness. `ui_state` logs `use_idle_ms` beside `idle_ms`.
- `wake_display` is unchanged and still restarts only the standby clock: a tap on the dim overlay is a touch through
  the touchscreen triggers anyway, and an alert blocks the rule while it is up.
- Older firmware keeps counting Wake, an alert ending and Auto standby on as a touch for Back to page 1. The Claude
  skill says that from 0.2.56 Wake is no touch; an installed skill shows as outdated until it is installed again.
- `static/app.js` no longer selects `inventory.screens[0]` in `refresh()` or `applyLive()`; the only way into a screen
  is its button. `renderScreens()` shows `#choose` ("Choose a screen") while nothing is selected and screens exist,
  `#empty` while there are none; both start `hidden` in `index.html`, so neither flashes before the first inventory.
  A screen picked before the full inventory arrives still gets the 0.2.58 catalogue redraw.

### Compatibility 0.2.64 / firmware 0.2.55

Both board profiles, the Claude skill and the editor's alert tips; the app raises `FIRMWARE_VERSION`. Storage, tile
protocol, preferences and keys are unchanged.

- `alert_title` gets `height: ${ALERT_TITLE_H}`, one line of its `headline` font: 32 px on the Guition, 21 px on the CYD.
  That is FreeType's size height of Roboto 500 at 27 and 18 px (`hhea` ascender minus descender, scaled and rounded),
  which ESPHome 2026.6.2 hands to `lv_font_t.line_height`. LVGL 9.5's `lv_label_refr_text` only puts the dots of
  `long_mode: DOT` on a label whose wrapped text is taller than the label, and a label without a height is as tall as
  its wrapped text, so a long title wrapped and ran into `alert_subtitle` (y 68 / 44). With one line of height LVGL
  breaks that line anywhere and ends it with "...". A title that fits draws the same pixels as before.
- `tests/test_alert.py` computes the line height from the font file and fails when `ALERT_TITLE_H` differs from it or
  the title reaches the subtitle: a new `headline` size needs a new `ALERT_TITLE_H` on that board.
- Older firmware keeps wrapping a long title over the subtitle.
- `claude_skill.text()` and the alert tips in `static/index.html` said the CYD shows about forty characters of a title.
  FreeType's advances of Roboto 500 give 20-23 characters before the "..." on the Guition (304 px at 27 px) and 24-25
  on the CYD (212 px at 18 px), so both now say about twenty. An installed skill shows as outdated until it is
  installed again.

### Compatibility 0.2.63 / firmware 0.2.54

Firmware (`components/smart_display`, new `theme.h`), both board profiles and the app's settings. The tile protocol
and the layout message are unchanged; nothing new on the wire.

- **Colours in one table.** `theme.h` holds every colour the firmware chooses: roles with a light and a dark value,
  the named card colours (`SWATCHES`, moved out of `tile_palette.h`, which now looks them up) and Home Assistant's
  state colours (`theme::ha`). Headers ask for roles; the profiles give widgets paints (`styles: paint_card`), shared
  styles without a colour that the profile's `theme::paints` list fills (`theme::fill()`, code in flash rather than a
  table in RAM). `tests/test_theme.py` fails on a hex colour anywhere else (the light's colour picker and the value
  card's colour wheel keep their hues) and on a paint that is not defined, filled once and used in every profile and
  package; `tests/test_theme.cpp` checks the values.
- **The light look is the old one.** Host renders of every page, card, alert and settings page against firmware
  0.2.53 are pixel-identical, apart from merges below one panel step: the text of the simple cards (`#202020` to
  `#1B1B1B`) and of some secondary words (`#5F6368` to `#616161`), the colour card's page (`#F7F7F7` to `#F8F8F8`,
  what the eye saw through the old sheet), an unavailable tile's circle and the off brightness track (to `#F1F1F1`),
  a pressed round key (`#DADADA` and `#D9D9D9` to `#DDDDDD`) and the tints of the vacuum's halo and the cover sliders,
  now computed from their state colour (within two steps of the old hand-picked values), and the two fixes below.
- **Two redraws that drew differently**, found by comparing a screen that switched looks with one that booted in it,
  and both older than 0.2.54. `settings_screen::draw()` ends in `refresh()`, whose `move_knob()` read the new track's
  coordinates before LVGL had laid the page out (still 0 wide), so every switch that was on showed its knob at the
  left until the next refresh; the knob is now placed from the size `pill()` gave the track (`knob_x()`), and a
  change of look, which draws the page again, no longer throws the Dark mode switch's own knob to the left. And
  `render_slot`'s palette gave the sun path's fill the tile's accent (orange) whenever the palette changed, while
  `render_sunpath` sets the sun's yellow on every render, so the sunlit area changed colour by itself a minute later;
  the sun path keeps its own fill now.
- **Dark mode** is `settings_screen::dark_mode` beside the other settings outside the frozen block, with a uint32
  preference of its own (`0x44524B31`, "DRK1"); `screen_settings::Settings` stays version 1. It is a row under
  Brightness (five rows: no pager on either board), the template switch `Dark mode` (config, `restore_mode: DISABLED`)
  and the app's key `dark_mode` (`SETTING_RULES`, `SETTINGS_BESIDE_BLOCK`, `SETTING_ENTITIES`). A screen that gets its
  settings with the layout never has it: `settings_view` leaves it out, the layout message never carries it.
  `DARK_MODE_MIN_FIRMWARE` is 0.2.54.
- **A change of look** runs through `set()` -> `apply_screen_settings` -> `theme::set_dark()`: the profile's and the
  firmware's own paints are refilled, LVGL is told once (`lv_obj_report_style_change(nullptr)`), and `theme::redraw`
  (set in `on_boot`) draws again what code painted by state: the tiles' palettes and the top bar
  (`runtime_tiles::restyle`), an open card in place, the colour card (`light_controls::restyle`), the settings page,
  an alert that is up (`alert_card_color`) and the climate card and its mode picker. The same pass, so no frame
  mixes the looks; at boot `set_dark()` runs before the first frame.
- **Profiles:** the substitutions `BG_TOP_COLOR`, `BG_BOTTOM_COLOR` and `ACCENT_*` are gone (an override that set them
  has no effect any more), `style_bg_grad` is `style_page` (one fill, no gradient between two equal stops), the four
  unused `*_disabled` styles and the colour card's `color_detail_bg` and `color_detail_scrim` are removed, and
  `open_value_overlay` lost its three colour parameters (the value card's slider takes paints).
- App 0.2.63 with firmware 0.2.53 or older: no Dark mode row, everything else as before. Firmware 0.2.54 with an
  older app: Dark mode works on the screen and in Home Assistant; the older editor does not show the row.
- A layout of a screen that gets its settings with the layout is stored with `dark_mode` among its settings. An app
  from before 0.2.63 refuses that key: restore the data backup when rolling back, as for every settings key before.

### Compatibility 0.2.62 / firmware 0.2.53

Firmware (`components/smart_display`), the icon fonts and the top bar's icon. Storage, tile protocol, preferences and
keys are unchanged; nothing new on the wire.

- A binary sensor's tile (`render_slot`) and card (`detail_state`) turn `on`/`off` into words with
  `tile_controls::binary_state_text(device_class, on)`, from the `device_class` attribute every state message has
  carried since app 0.2.23. `BINARY_WORDS` is a copy of the app's `header_bar.BINARY_STATES` (top bar, history card
  `states` and `words`); `tests/test_binary_words.py` keeps them equal, so change a word in both in one release.
- The screen maps the class itself instead of the app sending words: it works with every app since 0.2.23 (screens
  build from `ref: main`, so new firmware can meet an older app), costs the CYD no RAM per tile (the table lives in
  flash, the class was already kept), and adds no field for older firmware to ignore.
- `history_words` reads `history.words` only for the card's own entity. A card waiting for its history used to take
  the previous card's words (a motion sensor opened after a door read "Open", a switch too).
- Off looks off, as Home Assistant's frontend does it (checked on the `dev` branch and on a live dashboard):
  `light/icons.json` gives `mdi:lightbulb`, and `mdi:lightbulb-off` while off; anything off takes
  `--state-inactive-color` (grey); an entity's own icon never changes. `icon_for` draws F0E4F for a light without
  `tile.icon` while it is off (its own `if`, so `test_tile_icons` still reads the domain default), `render_slot` greys
  lights and binary sensors that are not active, as it did for switches, people and timers, and
  `header_bar.auto_icon` gives an off light the same bulb. `lightbulb-off` joins `tile_icons.FIXED`: one glyph more in
  the three icon fonts of both boards and in the editor font.
- App 0.2.62 with firmware 0.2.52 or older: tiles unchanged; an off light's top bar item shows without an icon, since
  that firmware drops a glyph its font lacks (`has_icon_glyph`).

### Compatibility 0.2.60 / firmware 0.2.52

`components/smart_display` and both board profiles; the app only raises `FIRMWARE_VERSION`. Storage, tile protocol,
preferences and keys are unchanged.

- `Tile::slider_active()` (runtime_model.h) decides whether a slider shows the tile's colour, after Home Assistant's
  `stateActive()` and its tile features: grey for an unavailable entity, a light or fan that is off and a media player
  that is `off` or `standby`; a cover keeps its colour when closed (`hui-cover-position-card-feature.ts`) and a number
  with a value is active. `style_panel` (sliders of direct controls) and `render_slot` (small sliders, with a third bit
  in the palette cache key) use it. `Tile::active()` is unchanged: it still greys the circle of an off switch, person or
  timer and gives the value overlay its `was_on`.
- Older firmware keeps the grey sliders; nothing else differs.

### Compatibility 0.2.59 / firmware 0.2.51

App, both board profiles and `components/smart_display`. Storage, tile protocol, preferences and keys are
unchanged. One new event from the screen and one new message to it; each side ignores what it does not know.

- The history card replaces the sensor card (24 bars from the tile's own graph samples) and the switch card (a large
  toggle) for `sensor`, `binary_sensor`, `switch`, `input_boolean`, `person`, `number` and `input_number`
  (`history_card` in runtime_tiles.h; `render_history_detail`). Opening it, or choosing a range, fires the event
  `esphome.screen_history` with `inbox`, `entity` and `hours` (1, 24 or 168) through `send_homeassistant_action` with
  `is_event`, like `esphome.screen_setting`: no permission to call actions is needed. The card asks again after 30 s
  without an answer and says "No history available" after 8 s, so it stays usable against an app from before 0.2.59.
- The app answers with one `{"v":1,"op":"history"}` message through the screen's `screen_message` action
  (`Manager.card_history_loop` → `answer_history`, only for an entity on that screen's layout and a screen that
  takes whole messages). Numbers (`kind: "line"`): `values` (24 time-weighted averages, null without data), `dom`,
  `yt` (axis values and their text, from `history_card.axis`), `dec`, `unit`, `hi` and `lo` ([value, unix time]).
  States (`kind: "timeline"`): `slots` (96), `states` ([words, colour, seconds], at most six and "Other"), `seg`
  ([slot, legend index or -1, begin, end, seconds], the real times of the run's state in seconds after `start`),
  `words` ([raw state, words] for the heading), `began` and `active`. Both carry `start`, `end`, `off` (the UTC offset
  at `end`) and `xt` (round times for the time axis). The largest possible timeline is about 3.6 KB.
- Data: a day or a week from `recorder/statistics_during_period` (hourly mean, min and max, or state for a total),
  starting an hour before the range; an hour, an entity without statistics and every timeline from
  `/api/history/period` with `minimal_response` and `no_attributes` (the call the tile graphs already use, capped at
  2 MB). Answers are cached per entity and range for 30 s, 2 min or 10 min, and screens asking at once share one
  fetch. A fetch that fails sends and keeps nothing.
- The screen keeps only the answer it waits for (`history_asked_entity`, `history_asked_hours`), draws the line
  as a monotone cubic through the averages and through the highest and lowest moment (`history_view::monotone`,
  `through`), and the timeline as rectangles in one draw event. The value now joins the highest or lowest moment
  when it lies beyond the history. A transparent area with `LV_OBJ_FLAG_PRESS_LOCK` reads PRESSED and PRESSING
  and only changes the heading's labels; nothing on the graph moves. Range keys are detail commands 160-162 left
  out of `detail_actions`, so a command waiting on Home Assistant does not disable them.
- `runtime_tiles::small_font` (axis labels and legend) is set to `sublabel` in `on_boot` of both profiles. The
  tile option `history_hours` still sets the tile's graph; a card opens on 1 hour when it is 1, else on 24 hours.
- Host redraw of the line card is about 70 ms, like the cover and weather cards (fill about 6 ms, line about 12 ms).
- `render_weather_detail` starts at y 84 on the Guition (was 62, from before the round back button of 0.2.43, which
  reaches y 76). The CYD keeps 38: its days card has no room to spare, and its back button overlaps the card by 10 px
  as before.

### Compatibility 0.2.58 / firmware 0.2.50

App, both board profiles and `components/smart_display`. Storage, tile protocol, preferences and keys are
unchanged; the state message gains one attribute and one extra key that older firmware ignores.

- Cover card: `runtime_tiles::event` sends a tap on a cover to `show_detail`, like the other runtime cards, instead
  of `detail()` (the board's value overlay in `cover_position` mode, which stays in the profiles for now). The
  `preview_runtime_card` action of both profiles does the same. `render_cover_detail` draws what
  `tile_controls::cover_card` offers from `supported_features` (position 4, tilt position 128, open/close/stop
  1/2/8, tilt open/close/stop 16/32/64; unknown features get the three keys): a vertical `lv_slider` per movement
  in a white card, its value and name below it (beside it on the CYD), and a row of pill keys. The position
  slider's range is reversed (1000 down to a stub below 0) so the fill hangs from the top by how far the cover is
  closed; the tilt slider has a transparent fill, a margin past both ends for its handle, and slats drawn behind
  it. Track and fill share one radius, so neither draws into a layer. A slider sends `cover.set_cover_position` or
  `cover.set_cover_tilt_position` on RELEASED after `touch_guard.accept_slider`. Keys use detail commands
  70 + `tile_controls::Command` through `key_action`; a disabled key is left out of `detail_actions`, so the
  card's second tick does not enable it again. The tick keeps `cover_status_line` ("Open · 60% · Tilt 40%").
- `Extra::tilt` holds `current_tilt_position`; the app adds it to `ATTRS`. A cover's device battery travels as
  the extra `bat` (and `chg`) the vacuum card introduced: `core.device_power` and `cover_related` pick the
  device's `battery` and `battery_charging` sensors, `Manager.related_entities` watches them, and `tile_message`
  passes the device entries for covers too. The card draws the battery at the top right with `vacuum_battery`.
- `apply_screen_settings` publishes a setting number that has no state yet: `number::Number::state` starts at
  0, so a standby or night brightness of 0 never differed from it and Home Assistant kept "unknown". Seen on
  Studio 1 (night brightness) after updating to 0.2.49.
- `discover_screens` ignores `unknown` and `unavailable` in the diagnostic sensors: the device name pattern
  accepted "unavailable" while a screen restarted, and `publish_layouts` wrote `sensor.esp_screens_unavailable`.
- Editor: the first full inventory redraws the top bar, tiles and entity list when the live stream opened a
  screen before the catalogue arrived. `index.html` is served with `?v=<content hash>` on `static/app.js` and
  `static/style.css`. The responses already carried `Cache-Control: no-store`, yet Safari showed an old script
  with the new page after the update to 0.2.57.

### Compatibility 0.2.57 / firmware 0.2.49

App, both board profiles and `components/smart_display`. Storage version 1, tile protocol `v: 1`, preference
keys and records are unchanged, and the eleven-key `settings` block is not widened.

- Firmware 0.2.49 offers every setting as a template entity (`entity_category: config`) over the existing
  preference records (`0x53435231` settings, `0x53575031` swipe, `0x484F4D31` back to page 1, `0x524F5431`
  rotation), so a screen keeps its values across the update without a migration. New: switches Night mode,
  24-hour clock, Back to page 1, Back to page 1 on standby and Swipe between pages (`restore_mode: DISABLED`,
  state from a lambda); number Back to page 1 after (30-3600 s, step 30); datetimes of type time Night starts
  and Night ends; select Rotation (`0°` to `270°`, Guition only). The four numbers and Auto standby are
  unchanged entities. `restore_value` cannot be combined with a lambda, which is why the entities keep no copy.
- Every writer goes through `settings_screen::set(key, value)`: the rows of the settings page and the
  `set_action`s, `turn_on_action`s and `turn_off_action`s of the entities. It clamps the way the page steps,
  pulls both dim levels down with the brightness, and stores, applies and reports (`changed()`) only a real
  change; it returns `SetResult::same` or `SetResult::unknown` otherwise. The switches publish from their
  lambda in `loop()`. `apply_screen_settings` publishes the numbers, times and the select when they differ; it
  runs after every change, every minute and on every Home Assistant time sync, which the API requests right
  after a client connects, so the entities have a value before Home Assistant subscribes to states. A host
  build with `time: platform: host` gets no such sync and publishes them at the next whole minute.
- `screen_message` has `supports_response: optional`. With a call id (a caller that sets `return_response`)
  it answers `{"status": <inbox state>, "rev": <layout revision>}` through `api.respond`; without one it
  neither answers nor logs "Cannot send response". The layout message still takes `settings` and the extra
  keys, so app 0.2.56 and older keep working with firmware 0.2.49 as before.
- `runtime_tiles::event` handles the built-in cards before the `fresh()` check: the settings card opens without
  Home Assistant, the clock card still does nothing. `settings_screen::refresh()` only writes a label text, knob
  colour or card opacity that differs, so a value from Home Assistant does not redraw unchanged rows.
- The app finds the setting entities per ESPHome device in the entity registry (`setting_entities`, by domain
  and `original_name`, disabled entries included). A device with one of `OWNED_SETTINGS_MARKERS` owns its
  settings; the five older entities cannot tell. For such a screen `layout_message` leaves out `settings`,
  `swipe_pages`, `auto_home`, `auto_home_seconds` and `rotation`; `save()` ignores `settings` from the editor
  (values stored before the update stay unused in `screens.json`); `esphome.screen_setting` events are
  ignored; `settings_view` reads the values from Home Assistant states (None while unavailable, keys only for
  entities the device has); `PUT /api/screens/{inbox}/settings` calls `number.set_value`,
  `switch.turn_on`/`turn_off`, `time.set_value` (`HH:MM:00`) and `select.select_option` (`90°`), brightness
  first, and only for values that differ.
- Older firmware keeps its settings in the layout. A reported setting is stored with
  `store_settings(..., on_screen=True)`, without the tile check of `save()` that refused a layout with a
  removed entity; `sent[inbox]['layout']` follows the stored settings and `sent[inbox]['rev']` keeps the
  revision the screen holds, so nothing goes back and the next ping still matches. `sync_one` keeps that
  revision while it does not send the layout message. Failures in the event loop are logged.
- Answers: `HomeAssistant.fetch_services` collects the ESPHome actions for which `get_services` lists
  `response`, again after `service_registered` or `service_removed` in the `esphome` domain. For those screens
  the keepalive ping, and one ping right after a batch that sent a layout, wait up to 5 s for the answer.
  A status from `RESEND_STATES` or starting with `Error` schedules a full resend 30 s after the last full send,
  doubling per failure up to the 120 s guard; any other status resets the count. A timeout waits for the next
  ping; a refusal whose message names `response` makes that action ping without answers from then on. The
  text entity keeps its state for older apps and for the screen list. Home Assistant 2026.9.1 lists the
  action with `response: {optional: true}` and passes the answer back.
- `Manager.run()` clears `published` together with `sent` while Home Assistant is offline, so the layout
  sensors made over REST are written again after a restart. `tile_message` asks `weather.get_forecasts` only
  for the kinds in `supported_features` (1 daily, 2 hourly; twice daily is not used); an entity without the
  attribute is asked for both, as before.
- Editor: `SETTING_GROUPS` in `app.js` mirrors the groups, labels, steps and duration ladder of
  `settings_screen.h` (a test compares them). Changes are debounced (150 ms for switches and chips, 600 ms
  for steps) and sent as one PUT per screen; the panel keeps its own value for up to 4 s or until Home
  Assistant reports it. Save no longer sends `settings`. The top bar's clock format is the `clock_24h` setting.

### Compatibility 0.2.56 / firmware 0.2.48

`components/smart_display/settings_screen.h` and both board profiles; the app only raises `FIRMWARE_VERSION`.
Storage, tile protocol, preferences and keys are unchanged.

- `settings_screen::attach_hold(page, x, y, w, h, below)` moves the hold strip and its fill line to the index of
  `below`, directly under it. Both profiles pass `id(brightness_overlay)`, the first of the YAML cards on
  `home_page` (brightness, colour, climate, climate mode, then the dim wake overlay). Before, `on_boot` created
  the strip after them, so LVGL's hit test found the transparent strip (x 32-448 on the Guition, 72 px high)
  before a card's back button (x 16-76) or its action at the top right. The dim wake overlay was not affected:
  `apply_screen_settings` moves it to the foreground whenever the screen is dimmed. The runtime detail card
  and the settings page move themselves to the foreground and were not affected either.

### Compatibility 0.2.55 / firmware 0.2.47

App, board profiles and `components/smart_display`. Storage, tile protocol, preferences and keys are unchanged.

- The app: `Manager.run()` compares the validated settings of an `esphome.screen_setting` event with the stored
  ones and skips `save()` when they are equal. Before, every event saved the layout, `save()` dropped
  `sent[inbox]`, and the next pass resent layout, header and every tile state.
- The four setting numbers (`setting_brightness`, `setting_standby_brightness`, `setting_night_brightness`,
  `setting_standby_seconds`) return before `settings_preference.save` and `setting_event` when
  `screen_settings::current` did not change (`Settings::operator==`). The Auto standby switch already did.
  No state is published in that case: `apply_screen_settings` publishes numbers only when they differ.
- `runtime_tiles::receive`, op `layout`: `layout_changed()` and `refresh_all()` run only for changed or moved
  tiles, the first layout, a rotation or settings change, or another `pages` count or title. A repeat still
  sets `inbox`, `keepalive_seconds`, `layout_rev` and `last_received`; the tile states after it redraw their
  own tiles through `refresh_tile`.
- Guition `on_boot` wraps the read callback of the LVGL touch input (ESPHome's `LVTouchListener`):
  `cyd::GhostTouch` repeats the previous read when a pressed read lands exactly on the native (0, 0), mapped
  with `LvglComponent::rotate_coordinates`, and logs `touch: GT911 stray contact at (0,0) ignored`.
  `on_update` skips (0, 0) before `touch_guard` and `edge_swipe`. The CYD keeps its own XPT2046 `TouchFilter`.
- `cyd::release_jump` logs (tag `slider`, WARN) when LVGL's recalculation on release (`update_knob_pos(obj,
  false)` for RELEASED in LVGL 9.5) moves a dragged slider to an end, more than a quarter of its range away
  from where it was held: the three colour-card rows and the tile sliders.
- Measured on Studio 1 (firmware 0.2.43, 2026-09-16): `automation.studio_1_scherm_wakker_houden_bij_licht`, a
  state trigger on a group of six lamps without `to:`, set `number.studio_1_standby_after` to its current
  value after every colour change. The resulting `esphome.screen_setting` event triggered a full resync
  (layout plus 13 states) 0.34 s later. Both red commits (`hs_color [0, 100]`) left 75-85 ms after such a
  resync reached the screen. Earlier Studio 1 logs show a stray (0, 0) as the last sample in 4 of 90 traced
  edge touches. ESPHome 2026.6.2's GT911 driver writes 0 to the status register before it reads the
  coordinates; that ordering is a candidate for an upstream fix.

### Compatibility 0.2.54 / firmware 0.2.46

Board profiles; the app raises `FIRMWARE_VERSION` and adjusts the Auto standby row of the Claude skill.
Storage, tile protocol, preferences and keys are unchanged.

- The Auto standby switch no longer clears `sleep_requested`; `wake_display` is the only place that does.
  The one rule lives in `apply_screen_settings`: a dimmed screen wakes when standby is off unless
  `sleep_requested` is set, whichever path turned standby off (the switch, ESP Screens, the settings page).
- `dim_display` has no condition any more; its callers decide (the 1 s interval checks `standby_enabled`,
  the Sleep button sets the flag). It sets `display_dimmed`, runs `apply_screen_settings`, then `go_home`
  with `home_on_standby` on, otherwise `settings_screen::close()` and `close_cards`. Runtime detail cards
  now close on standby in both cases, as the light, colour and climate overlays already did.
- `close_cards` is `runtime_tiles::dismiss()`, which on_boot sets on both boards (hide the detail card,
  clear `active_entity`, hide the four overlays). The fallback after it and the four `lvgl.widget.hide`
  actions ran for nothing: a `return` in a lambda does not skip the actions that follow it.
- The `back_light` light is `internal: true`, so Home Assistant loses `light.<screen>_display_backlight`
  (CYD: `light.<screen>_power_display_backlight`); it stays in the registry as unavailable until deleted.
  Nothing in the app, the tools or the tests used it; the firmware keeps the light under the same id.
- `apply_screen_settings` loses the `!runtime_tiles::enabled` clock-label branch: on_boot sets `enabled`
  before anything can run the script. The `runtime_tiles::enabled` checks before preference saves stay,
  because `load_settings()` makes that preference right after `enabled` is set.

### Compatibility 0.2.53 / firmware 0.2.45

Firmware and board profiles; the app raises `FIRMWARE_VERSION` and extends the Claude skill. Storage,
tile protocol, preferences and keys are unchanged.

- Both profiles add two template buttons, `wake_button` ("Wake", `mdi:gesture-tap`) and `sleep_button`
  ("Sleep", `mdi:power-sleep`), without an entity category: they are controls. Wake runs `wake_display`
  on a dimmed screen (the path the tap on `dim_wake_overlay` takes) and only sets `last_touch_ms` on a
  screen that is on. Sleep is ignored during a touch calibration; otherwise it sets the new global
  `sleep_requested`, runs `alert_dismiss` (reason `remote`, a no-op without an alert) and runs
  `dim_display`. Both log a line with tag `standby` at INFO.
- `sleep_requested` (bool, not restored) lets `dim_display` dim with `standby_enabled` off and stops
  `apply_screen_settings`, which runs every minute, from waking a dimmed screen while standby is off.
  `wake_display` clears it (a tap, Wake, an alert, `open_settings`, `touch_diagnostics`), and so does
  the Auto standby switch when it really turns standby off; a switch that was already off leaves it.
  Nothing is saved, so the buttons cost no flash writes.
- `core.WAKE_SLEEP_MIN_FIRMWARE` is 0.2.45. `claude_skill.text()` lists both buttons in "Standby and
  brightness" with one more YAML example (nine in total), so an installed skill shows as outdated until
  it is installed again. The skill description stays under 200 bytes.

### Compatibility 0.2.52 / firmware 0.2.44

App and firmware. Storage version and tile protocol stay 1; everything is additive, so an old screen
with this app, and a new screen with an older app, both keep working.

- **Two settings, beside the frozen block.** `settings.auto_home` (bool, default `true`) and
  `settings.auto_home_seconds` (30-3600, default 120) are new keys in `SETTING_RULES`, stored in
  storage version 1. On the wire they travel as their own top-level keys of the layout message, next to
  `swipe_pages` and `rotation`; the eleven-field `settings` object is unchanged, because firmware before
  0.2.44 refuses any other size. `core.SETTINGS_BESIDE_BLOCK` is the one list of such keys and
  `layout_message` uses it. Older firmware ignores the two keys.
- **Firmware storage.** A new preference key `0x484F4D31` holds `HomeTimeout` (two uint32: enabled,
  seconds). The Settings structure `0x53435231`, swipe `0x53575031`, rotation `0x524F5431` and the CYD
  calibration are untouched. `runtime_tiles::persist_settings()` saves all of them; ESPHome compares each
  blob with NVS before writing, so unchanged records cost no flash write.
- **Settings page on the screen.** `components/smart_display/settings_screen.h`: one `constexpr` table of
  pages and rows, drawn into a root object created on open and deleted on close. Opened by holding the
  top bar (`attach_hold`, substitutions `SETTINGS_HOLD_X/W/DRIFT_PX`; on the Guition the strip keeps
  clear of both swipe bands), by the `screen.settings` tile, or by the API action `open_settings`
  (`page` 0-4, negative closes and goes home). A change is saved, applied (`apply_screen_settings`) and
  reported with the existing `esphome.screen_setting` event; the manager's handler accepts every key in
  `SETTING_RULES`, the new ones included. Row taps use the board's own drift limit
  (`TOUCH_MOVE_LIMIT_PX`), the hold its own.
- **Back to page 1 by itself.** The 1 s interval runs the new `go_home` script (settings page closed,
  cards closed, page 1) when `auto_home` is on and nothing touched the screen for `auto_home_seconds`,
  while a card, the settings page or a later page is showing. Standby now always closes the settings page;
  an alert closes it too.
- **`screen.settings` tile.** A second built-in entity next to `screen.clock`. `min_firmware` asks for
  0.2.44, so the manager does not send a layout with it to older firmware. `Tile::is_clock()` now marks
  the clock-only paths (minute redraw, second hand) that used `builtin()`; the editor mockup takes the
  icon from `tile_icons.BUILTIN_TILES`.
- **Glyphs.** `monitor`, `information-outline` and `restart` join `tile_icons.FIXED` (183 glyphs).
- **API.** `open_settings` is a new ESPHome action; Home Assistant lists it as
  `esphome.<screen>_open_settings` after the device reconnects with 0.2.44. The `IP address` text sensor
  gained `id: wifi_ip`; its entity is unchanged.
- **Docs.** docs/SETTINGS.md is the recipe for adding a setting; the Claude skill gains a section on the
  page (one more YAML example), so an installed skill shows as outdated until it is installed again.

### Compatibility 0.2.51 / firmware 0.2.43

App only; no firmware change, no protocol change, no storage change. Layouts keep their shape, so an
older app reads what this one writes.

- `core.TILE_EVENTS` maps four Home Assistant events to an action: `esp_screens_add_tile` (add or
  change), `esp_screens_remove_tile`, `esp_screens_move_tile` and `esp_screens_order_tiles`.
  `apply_tile_event(layout, action, data)` returns the new layout and raises `ValueError` with the
  sentence the log and the answer carry; `match_screen` finds the screen by device name, the name Home
  Assistant shows, the device, the area or the layout title, and needs no name when one screen is paired.
  `tile_options` turns the event's fields into tile settings (`color` is the pastel background) and makes
  a tile with a control, a forecast or a sun path double-width. `event_page`/`event_slot` accept `page`
  (from 1), `row` (1-3) and `column` (left/right), or `slot` as the editor counts it. `place_tile`,
  `free_slot` and `pack_page` keep a double-width tile in the left column and leave gaps alone.
- `HomeAssistant` subscribes to those events, queues them in `tile_events`, and gained `fire()` (the
  websocket `fire_event`) and `set_state()` (the REST API, the only way an app can publish a state).
  `Manager.tile_loop` handles one event at a time, saves through `Manager.save` so validation, the entity
  check and the push are the editor's, and answers with `TILE_RESULT_EVENT` (`ok`, `screen`, `entity`,
  `error`).
- `Manager.publish_layouts` writes `sensor.esp_screens_<node>` per screen after every sync pass and after
  every tile event: the state is the tile count, the attributes carry `title`, `pages` and `tiles`
  (`entity`, `name`, `page`, `row`, `column`, `slot`, `size`, `controls`, `display`). States made over the
  REST API disappear when Home Assistant restarts; the sync loop writes them again.
- The Claude skill gains "Tiles on a screen" with the four events, the fields, the controls and displays
  per domain (generated from `CONTROLS`/`DISPLAYS`), how to read the sensor first, and the rule to
  confirm before changing a screen. Its description changed, so an installed skill shows as outdated
  until it is installed again. `tests/test_tile_events.py` covers the logic and one event end to end;
  `tests/test_claude_skill.py` checks the section and the two new YAML examples.

### Compatibility 0.2.50 / firmware 0.2.43

Firmware, board profiles and the package generator; the app only raises `FIRMWARE_VERSION`. Storage,
protocol, preferences and keys are unchanged.

- Slider fills: `runtime_tiles::slider_handle` and the light card rows in `light_controls.h` give
  `LV_PART_INDICATOR` the track's radius (`height * 12 / 42`, round at 20 px or less). A smaller fill
  radius makes LVGL 9.5's `lv_bar` draw the fill into an ARGB8888 layer of its own size on every redraw
  and clip it to the track (a 128×30 fill: 15 KB). When that allocation failed, the refresh retried until
  the task watchdog reset the CYD (log: `lv_draw_buf_create_ex: No memory: 115x30, cf: 16`).
  `tests/test_layer_free.py` rejects layer styles (`opa_layered`, transform rotation/scale/skew, blend
  mode, bitmap masks, `clip_corner`, drop shadow, blur) and a fill radius that differs from its track in
  both profiles, both packages and the firmware headers. `opa` itself makes no layer in LVGL 9.5.
- `runtime_model.h`: `Tile` keeps the shared fields inline. `Extra` holds climate modes and action, select
  options, forecast/hours/wind/feels, sunrise/sunset/timer, media title, vacuum speeds, choices, room and
  charging. It lives on the heap only while a state message fills something in (`Tile::set_extra`,
  `extra()`, `edit_extra()`, `extra_ptr()`). `options` and `fan_speeds` are vectors (no
  `option_count`/`fan_speed_count`), and `history` holds 24 samples or none. `revision`/`pending_revision`
  are FNV-1a fingerprints (`Fingerprint`, also an ArduinoJson writer) instead of copies of state,
  attributes and extras. On the ESP32 `runtime_tiles::model` shrinks from 24,296 to 8,216 bytes.
- The old manual profile is gone from both board profiles: the 80 `homeassistant` sensor and text
  sensors of the fixed tiles, `smartdisplay_action`, the tile tap and long-press handlers,
  `publish_action`, `do_tile_action`, the manual refresh in `ui_refresh`, the YAML vacuum card with
  `open_vacuum_overlay`/`vacuum_action`/`vacuum_fan_speed`, `light_controls::subscribe`, the 331
  `TILE*` substitutions, and `DYNAMIC_TILES`/`TILE_COUNT`/`DIRECT_ACTIONS`/`TIME_24H`/`ORIENTATION`.
  `runtime_tiles::enabled` is set true at boot and the `DIRECT_ACTIONS` guards became
  `runtime_tiles::fresh()`. The runtime detail hook lost its unreachable vacuum branch. Deleted:
  `device.example.yaml`, `guition-device.example.yaml`, `tools/new_device.py`, `tools/export_bundle.py`,
  `TILE_CONFIGURATION.md`, `docs/TILES.md`, `docs/ACCEPTANCE.md`, `CYD_STABILITY.md`, `TEST_RESULTS.md`,
  and the five stale header copies in the repository root (`cyd_ui.h`, `light_controls.h`,
  `alert_overlay.h`, `backlight_fade.h`, `guition_diagnostics.h`) that had drifted from
  `components/smart_display/`; `tests/test_cyd_ui.cpp` and `tests/test_light_controls.cpp` were
  compiling those copies and now include the component headers, as both profiles do.
  `tools/generate_packages.py` no longer strips or substitutes anything for the manual profile: the
  board profile's own defaults are the package's, and a `homeassistant` sensor in a profile is refused.
  The entity "SmartDisplay Action" disappears from every screen. The USB touch calibration
  (`tools/calibrate.py`, `docs/CALIBRATING.md`, `calibration.example.yaml`, `CALIBRATION_ON_BOOT`,
  the measurement page) and the on-screen calibration are unchanged.
- `esphome config` of both Easy profiles is byte-identical before and after this cleanup, apart from
  two reordered lines: the firmware does the same thing, with the same calibration.
- `tile_scroll` and `dim_wake_overlay` (and the manual vacuum rows) get `pressed: bg_opa: TRANSP`. The
  theme's obj pressed style (`bg_opa` 45%) lit up the transparent area on a tap between tiles and on the
  wake tap.
- Both profiles add the debug sensor `min_free` ("Heap Minimum Free":
  `heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL)`, every 300 s).

### Compatibility 0.2.49 / firmware 0.2.42

Firmware and the CYD board profile; the app only raises `FIRMWARE_VERSION`. Storage, protocol,
preferences and keys are unchanged.

- home-like-2432s028.yaml adds `init_sequence` to the mipi_spi display: `0xB4 0x07` (frame inversion)
  and `0xB1 0x00 0x10` (119 Hz). ESPHome appends these after its built-in ILI9341 sequence, so the
  panel init otherwise stays the ESPHome default. Found on 2026-09-15 by sending panel commands at
  runtime on the bench CYD; VCOM (`0xC5`) sweeps changed contrast, not the stripes, so it stays default.
- guition-4848s040.yaml only raises `SCREEN_FIRMWARE_VERSION`.

### Compatibility 0.2.48 / firmware 0.2.41

Storage, protocol, preferences and keys are unchanged.

- Both profiles add a template switch `setting_auto_standby` ("Auto standby", entity category config,
  `restore_mode: DISABLED`, state from `screen_settings::current.standby_enabled` through its lambda).
  Its actions change `standby_enabled`, save the settings preference, send the existing
  `esphome.screen_setting` event (`standby_enabled`, 1/0; the app already stores bool settings from it)
  and run `apply_screen_settings`, which wakes a dimmed screen when standby is off. Turning it on sets
  `last_touch_ms`, so the standby time counts from then.
- `claude_skill.text()` gains "Standby and brightness" (the switch, the four numbers, an example
  automation); `core.AUTO_STANDBY_MIN_FIRMWARE` is 0.2.41. The skill description stays under 200 bytes.
  An installed skill shows as outdated until it is installed again.

### Compatibility 0.2.47 / firmware 0.2.40

Firmware and board profiles; the app only raises `FIRMWARE_VERSION`. Storage, protocol, preferences and
keys are unchanged.

- guition-4848s040.yaml gets the CYD's climate card (0.2.46): the arc, `climate_detail_bg`/`_scrim`,
  `climate_mode_pill` and the `climate_number` font are gone; `setpoint_digits` is Roboto 400 at 64 px.
  The scripts `climate_adjust_target`, `climate_target_send` and `climate_card_refresh` match the CYD,
  except that the Guition's refresh hides the ··· key and lays the card out per case (no, one or two
  settings rows, with or without mode keys) from one table, applied only when that case changes.
- New `components/smart_display/climate_card.h`: `climate_card::show(rows, count)` draws the fan
  ('f') and swing ('s') rows into `climate_settings_card` through `lv_async_call`, so a row can redraw
  itself from its own tap; taps go through `cyd::touch_guard` and the profile's `climate_card::choose`
  hook, which runs `set_climate_fan_mode` / `set_climate_swing_mode` with the index in the entity's list
  (`cyd::list_item`). At most six choices per row. Both scripts now refresh the card at once.
- Both profiles hide the mode key row when a device offers at most one mode besides off (and no ···),
  and centre the setpoint card instead.
- Glyph `arrow-oscillating` (F1C91), Home Assistant's swing icon, joins `tile_icons.FIXED`.

### Compatibility 0.2.46 / firmware 0.2.39

Storage version, tile protocol (`v: 1`), preferences and keys are unchanged. Additive wire fields only.

- A vacuum's state message carries new extras in `x` (`core.vacuum_extras`): `mode` and `water`
  (`e` select entity, `s` state, `o` options, `l` labels, `r` one role letter per mode: v/m/b/a),
  `fan` (the suction speeds to offer, `o`/`l`), `bat` (0..100), `chg` (1 while charging) and
  `room`. Options are capped at six, labels at 24 bytes. With a mode select, speeds and water
  levels that a mode covers (`off`, `custom`, `smart_mode`, …) leave the rows, and `custom` leaves
  the mode row unless it is the current mode. `a.fan_speed_list` stays capped at four, so firmware
  before 0.2.39 draws its old suction row; firmware 0.2.39 with an older app does the same.
- `core.vacuum_related` finds the selects, the battery sensor (`device_class: battery`), the
  charging binary sensor (`battery_charging`) and the room sensor on the vacuum's device, by
  translation key first (entity ids follow the HA language) and entity id ending second; disabled
  entities are skipped. `Manager.device_entries` indexes the registry per device (rebuilt with the
  registry object), `related_entities` adds these ids to `watched_entities` and to the `sync_one`
  reuse check, so a select change resends the vacuum's message.
- Firmware: `Tile::choices` (`Choice`: kind m/w/s, entity, current, roles, values, labels, `sent`),
  `room`, `charging`; the state revision now includes `x`, so a select change confirms a pending
  command. Chips send `select.select_option` (or `vacuum.set_fan_speed`), mark the vacuum tile
  pending and redraw the card through `lv_async_call` after the event. `tile_controls::vacuum_rows`
  / `vacuum_role` / `shown_value` / `settle_suction` / `choice_action` are covered by
  tests/test_tile_controls.cpp. Detail commands: 10-15 suction, 50-55 mode, 60-65 water;
  `detail_actions` holds 32 buttons. `tick()` redraws an open vacuum card when its state text changes.
- CYD climate card (home-like-2432s028.yaml): the arc is gone. New widgets `climate_setpoint_card`,
  `climate_mode_keys`, `cmkey_*`, font `setpoint_digits` (Roboto 400, 40 px, digits . - °), globals
  `active_climate_current`, `active_climate_action`, `active_climate_target_known`, scripts
  `climate_card_refresh` and `climate_target_send` (restart mode, 700 ms, entity and value as
  parameters); `climate_target_preview`/`climate_target_commit` are removed. `tile_controls::mode_color`
  holds HA's mode colours (also used by `domain_accent`). Glyph `dots-horizontal` (F01D8) joins
  `tile_icons.FIXED`.
- Both profiles: the 1 s interval clears `climate_edit_pending` when the climate card is closed and,
  on runtime tiles, replays the held-back state through `detail_update`, instead of running
  `ui_refresh` every second while the flag stayed set.

### Compatibility 0.2.45 (firmware stays 0.2.38)

App only: storage version, tile protocol, preferences and keys are unchanged.

- Alerts for every screen: on connect the app also subscribes to the Home Assistant events
  `esp_screens_show_alert` and `esp_screens_dismiss_alert`. The reader only queues them;
  `Manager.alert_loop` handles them in arrival order, apart from the sync loop, and calls
  `esphome.<node>_show_alert` / `_dismiss_alert` at once on every screen from `discover_screens`
  that is online, has a device name and reports firmware 0.2.31+ (one call per node).
  `core.alert_data` types the seven fields the way Home Assistant validates ESPHome actions (text,
  an int clamped to 0..86400, a bool) and leaves an unusable value empty instead of failing the call.
  The log line `<event>: N of M screens (not: …)` names skipped and failed screens.
- `claude_skill.py` builds SKILL.md from the alert reference and `tile_icons`, byte for byte the same
  on every call. `POST /api/claude-skill` writes `$HA_CONFIG/.claude/skills/esp-screens/SKILL.md`
  (default `/homeassistant`, the start folder of the Claude Code apps) only on request;
  `GET /api/claude-skill.zip` returns `esp-screens/SKILL.md` for claude.ai, whose description limit
  is 200 characters. The full inventory's `claude_skill` compares the file with the current text.
- Editor: the header tools and the updates block moved into `#settings-view`, shown for the hash
  `#settings`; element ids and their handlers are unchanged.

### Compatibility 0.2.44 / firmware 0.2.38

Firmware only: `show_detail()` creates no `detail_status` for a large vacuum card (the hero badge
carries the state; `tick()` already guards on a null status).

### Compatibility 0.2.43 / firmware 0.2.37

Firmware only: storage version, tile protocol, preferences and keys are unchanged.

- `runtime_tiles::screen_awake` is a hook both profiles set to `!id(display_dimmed)`; without it
  the runtime counts as awake. `second_hand()` draws part 18 (points 28-29) of an analog clock
  card and `tick()` moves it once a second while awake, hiding it otherwise; the minute redraw
  stays as it was.
- `slider_handle()` now sets the track and fill radii (12/42 and 8/42 of the height, round ends
  under 20 px), a handle of 4 px (2 px on small strips) an eighth of the height from the end, and a
  shortest fill of a third of the height (`slider_stub`). `slider_bar()` hides fill and knob for
  an off light or fan (`slider_bar_shown`); tracks are the fill colour at 20 % over the card.
  `light_controls::Row::off` does the same for the colour card's brightness row.
- `commit_slider` floors a light's brightness at 3 (1 %); the card tap turns it off.
- Overlay top bar: the profiles' `*_close_button`, `climate_mode_overlay_x`, `vacuum_overlay_x` and
  the new `overlay_back_button` are one round `arrow-left` (glyph `F004D`, added to
  `tile_icons.FIXED`) button at the top left, 60/40 px (Guition/CYD) at (16,16)/(10,8);
  `climate_power_button` is the same size at the top right. `color_done_button` and
  `vacuum_overlay_ok` are gone; `show_detail()` draws the same bar for the runtime detail card.
- `render()` shows "Connecting to Home Assistant..." / "Waiting for ESP Screens..." until the
  first layout arrives.

### Compatibility 0.2.42 / firmware 0.2.36

Firmware only: storage version, tile protocol, preferences and keys are unchanged. What moved
inside `runtime_tiles.h` (measurements in docs/SWIPE_PROFILE.md):

- `render()` is split into `render_slot()`. A page switch (`show_page` to another page) places
  the page, shows every card as a skeleton (a sheet in the card colour over its content area,
  `Widgets::veil`) and draws two cards per LVGL refresh (`fill_cards`, `FILL_STEP_CARDS`,
  `LV_EVENT_REFR_READY`, 40 ms fallback); a new switch cancels the fill. `apply_page` (same page:
  keepalives, re-packing) still draws everything at once. `check_tile_geometry` finishes a fill
  still under way and logs `page fill still under way at the check`.
- `refresh_tile(index)`, `refresh_header_only()` and `refresh_all()` mark what the next
  `render()` draws; a plain `refresh()` from the board YAML draws every card, as before.
- A slot hides custom parts and control panels instead of deleting them and reuses them for the
  same kind (`hide_extra`, `hide_panel`); `end_extra`/`end_panel` still delete on a kind change.
- Guarded style setters `set_color`/`set_number`; card content sizes from the styles
  (`tile_width`, `content_width`, `content_height`) instead of `lv_obj_update_layout()`.
- Card sliders: `slider_handle()` narrows the knob and moves it back into the fill and sets the
  range to `-below..1000`, so 0 still shows one round end; `slider_event` keeps values at 0 or
  above, `commit_slider` clamps as before. Off cards get a grey fill.
- The busy sheet is sized from the requested card width and kept as the tile's last child.
- `show_page` ignores a request past the first or last page when that page is already shown.
- `tick()` redraws clock cards when the minute changes.
- `light_controls::setup(parent, font, width, height, icon_font)`: the icon font is new and
  optional; both board profiles pass `materialdesign_icons_mini` (the glyphs palette,
  thermometer and lightbulb are already in the tile icon set). Rows, sliders, `open()` and
  `self_test()` keep their behaviour; the brightness range starts below 1 for the same round end.
- `swipe_profile.h` and the `swipe_test` message exist only with `-DSWIPE_PROFILE=1`; release
  builds and packages never set it.

### Compatibility 0.2.41 / firmware 0.2.35

Storage version and tile protocol stay 1. Firmware 0.2.35 only raises
`SCREEN_FIRMWARE_VERSION`, so the owner can test the update path from 0.2.34: no new
entities, names, preferences or keys. App 0.2.41 changes the README, the App store
description (`screen_manager/README.md`, images through absolute `raw.githubusercontent.com`
URLs because the App store does not resolve relative paths) and `FIRMWARE_VERSION`.

### Compatibility 0.2.40 / firmware 0.2.34

Storage version and tile protocol stay 1; no new message fields, preferences or keys.
Firmware 0.2.34 changes only text: every on-screen label, log line and status reply is
English, and so are the entity names. The status replies the manager reacts to are now
`Ready for tile configuration`, `Loading tiles`, `Resend needed`, `Synced` and
`Error: …`; `RESEND_STATES` in `server.py` and `diagnostics/*.py` accept both
languages, so screens on firmware 0.2.33 and older keep working unchanged.

The entity names are part of the compatibility contract. Home Assistant's ESPHome
integration builds the unique ID from the entity name (`<MAC>/0/text_info/Tile settings`),
so a renamed entity gets a **new entity ID** and the old one is removed as soon as the
screen reconnects (seen on HA 2026.9.1: `text.cyd_2_8in_display_tegelinstellingen` →
`text.cyd_2_8in_display_tile_settings` within the same second). Renamed: `Tegelinstellingen` →
`Tile settings`, `Schermfirmware` → `Screen firmware`, `Apparaatnaam` → `Device name`,
`IP-adres` → `IP address`, `Guition schermtype` → `Guition screen type`, `Opgestart` →
`Last boot`, `Helderheid normaal/standby/nacht` → `Normal/Standby/Night brightness`,
`Standby na` → `Standby after`, `Touch kalibreren` → `Calibrate touch`.

`core.discover_screens` matches both names (`NAME_*` pairs). Because the manager keys
`screens.json` and `updates.json` by the inbox entity ID, `Manager.follow_renamed_inboxes`
(run whenever the screen list is rebuilt) moves a screen's layout, update address and last
result to its new inbox ID: first by the HA device an inbox was seen on during this run,
otherwise (after a restart) when a stored ID's device part (`core.inbox_prefix`) matches
the entity ID prefix of one of the device's ESPHome entities (`core.device_prefixes`; the
unchanged `Node Status`, `Restart` and `Wifi Signal` entities keep the old prefix). An existing
layout on the new ID is never overwritten; layouts of other screens are never taken.
`Manager.aliases` maps old to new IDs so `save()`, the inspector and the updater
(`Updater.current_id`, `Updater.renamed`) follow a screen that is renamed mid-update: the
Update button and the nightly round verify the new ID and record success there. Don't rename
these entities again without keeping this path. Tests: `tests/test_renamed_inbox.py`.

### Compatibility 0.2.38 / firmware 0.2.32

Storage version and tile protocol stay 1. A layout may have an optional object
`header: {items: [...]}` (six max): `{type: clock|analog|date}` or
`{type: entity, entity, content: state|last_changed, icon: auto|none|<name>, show:
always|active}`. Without `header`, the firmware's prior always-on behavior applies (the clock from
`show_clock`); an old editor that omits `header` keeps the stored bar. When
saving, `settings.show_clock` follows the clock in the bar, so older firmware only shows the
time if it's in the bar. Besides the tile domains, entities in the bar may
also be `device_tracker`, `zone`, `lock`, `alarm_control_panel`, `counter`, `event`,
`input_datetime`, `input_text`, `water_heater`, and `humidifier`; the inventory
marks those with `tile: false` so the tile picker skips them.

On the wire, right after the layout message, the manager sends `{v:1, op: header, items}`
with, per item, `k` (`clock`, `analog`, `date`, `text`, `ago`), `i` (codepoint),
`t` (text, already formatted — in English since app 0.2.40 — and limited to the glyphs of
`sublabel_big`), `e` (unix time for `ago`), and `c` (accent color). Items with
`show: active` that aren't active are not sent by the manager. Only firmware 0.2.32+
gets this message (`HEADER_MIN_FIRMWARE`, based on the reported screen firmware): older
firmware would refuse `op: header` with "Fout: ongeldig bericht" (that firmware still reports in Dutch). The message is
compared separately; a changed value sends only the bar, not the full layout.
The firmware replies with the same status as a tile status, so the
inbox entity doesn't flip on every value.

Firmware: `header_bar.h` (LVGL-free: parsing, "5 min ago", date, distances,
placement) and `runtime_tiles::draw_header()`. The profile's time label still
exists as a reference (right margin, draw order) but is hidden in runtime
mode. Values use `sublabel_big`, icons `materialdesign_icons_mini`;
`sublabel_big` gained `'`, `#`, `*`, `=`, `;`, `²`, `³`, `µ`, `–`, and common
accented characters; `tests/test_header_bar.py` keeps `header_bar.GLYPHS` matching that list.
The editor (`barLayout` in `app.js`) and the firmware (`header_bar::gaps/place`) share
the same integer math; the test checks that. `tools/render_topbar.py`
renders the real `draw_header()` on a Mac/Linux machine (ESPHome host + SDL2) to
a PNG, without a screen. No changed preferences or keys.

### Compatibility 0.2.35 / firmware 0.2.30

Firmware only (Guition). `on_boot` removes `LV_OBJ_FLAG_CLICKABLE` from `home_page` and
`tile_scroll`: a press outside the tiles no longer has an LVGL object (no pressed style,
no redraw, no events); the edge swipe runs via the touchscreen triggers and doesn't need
that, `lv_indev_wait_release` also works without an active object. Trace lines `swipe …`
(`veeg …` before firmware 0.2.34) at INFO only while `edge_swipe.armed()`. Otherwise unchanged.

### Compatibility 0.2.34 / firmware 0.2.29

Firmware only. LVGL 9.5.0's `send_event()` in `lv_indev.c` sends the input device
PRESSED, RELEASED, CLICKED, LONG_PRESSED, and KEY, but no PRESSING; an
`lv_indev_add_event_cb` therefore gets no position updates during a press. `cyd::EdgeSwipe`
therefore works again on the native touch points from `on_touch`/`on_update`/`on_release`
(`configure(width, height, band, travel)`, `begin(x, y, rotation)` with ESPHome's
rotation mapping, `update()`, `end()`); the indev callback is gone. Otherwise unchanged.

### Compatibility 0.2.33 / firmware 0.2.28

Firmware only. `TouchGuard::configure(0, ...)` disables the movement limit
(`moved_` is then never set; LVGL's press-lost decides whether a tap goes through). The
Guition profile sets `TOUCH_MOVE_LIMIT_PX` to 0, the CYD keeps 56. Otherwise unchanged.

### Compatibility 0.2.32 / firmware 0.2.27

Firmware only. `cyd::EdgeSwipe` works on LVGL pointer coordinates: `configure(band, travel)`,
`begin(x, y, width)` with `lv_display_get_horizontal_resolution()`, `update()` with the 45° rule,
plus `inward()`/`sideways()` for the log. The Guition registers a single `lv_indev_add_event_cb`
(PRESSED/PRESSING/RELEASED) in `on_boot`; the touchscreen triggers now only feed the
`TouchGuard`. Both board profiles get the `close_cards` script (calls
`runtime_tiles::dismiss()`, otherwise hiding overlays and clearing `active_entity`); the
cards' close buttons and background taps, and `wake_display`, use it. Protocol, storage,
preferences, and keys unchanged.

### Compatibility 0.2.31 / firmware 0.2.26

Storage version and tile protocol stay 1. `tiles[].slot` is additive: the absolute slot
of a tile (page × 6 + row × 2 + column, 0–47, `MAX_SLOTS` in `core.py`); a
double-width tile sits on an even slot and also covers the slot to its right. Empty slots
are allowed. `validate_layout` requires slots for all tiles or for none: without
slots (old editor, old storage), the layout gets the slots from the old
order-based packing (`pack_slots`, the same as `pack()` in the firmware), so
nothing changes on the screen; with slots, overlap, odd double-width, and out-of-range are
refused, and the tiles are sorted by slot. `save()` repeats the packing after
restoring saved options (which can widen a tile). `pages` (1–8) is
an optional, additive field: pages the user wants to keep empty.

On the wire, the layout message gets `slots` (one slot per entity, in the same
order) and `pages`. Firmware 0.2.26 (`Model::set_layout` with positions, `place()`
alongside the old `pack()`, `MAX_PAGES` 8) draws the tiles at those slots; a change
to slots only (`moved`) repositions the pages without clearing tile statuses.
Older firmware ignores both fields and packs the entities in order; since the
entities are sorted by slot, that's the old gap-free display, hence no
`min_firmware`. The editor reports this (`has_gaps`/`hasGaps`) for firmware < 0.2.26. No
changed preferences or keys.

### Compatibility 0.2.30 / firmware 0.2.25

Editor only: the status dot in the screen list is its own `span.dot`
(green with `.online`). Firmware only gets a new version number; protocol,
storage, preferences, and keys unchanged.

### Compatibility 0.2.29 / firmware 0.2.24

Firmware only (Guition). `cyd::EdgeSwipe` (in `cyd_ui.h`, tested in
`test_cyd_ui.cpp`) replaces the LVGL gesture on `home_page` on the Guition: `begin()`
in `on_touch` with the LVGL rotation, `update()` in `on_update` for the first contact;
on a hit, `touch_guard.consume()`, `lv_indev_wait_release()` on all indevs, and
`show_tile_page`. Substitutions `EDGE_SWIPE_BAND_PX`/`EDGE_SWIPE_TRAVEL_PX`;
`DISPLAY_W`/`DISPLAY_H` determine the rotation mapping (ESPHome: 90° `x=y, y=W-x-1`,
270° `x=H-y-1, y=x`, 180° mirrors both). The CYD keeps the LVGL gesture. The
`swipe_pages` setting remains the toggle; protocol, storage, and keys unchanged.

### Compatibility 0.2.28 / firmware 0.2.23

Firmware only. `cyd::TouchGuard` gets `configure(move_limit_px, min_press_ms)`
(called in `on_boot` from the substitutions `TOUCH_MOVE_LIMIT_PX` and
`TOUCH_MIN_PRESS_MS`; without the call, the old 18 px and 60 ms apply), tracks only
the contact id the touch started with, measures displacement as distance to a
reference point settled over four measurements, and reports via `reason()` why a tap
was rejected; `runtime_tiles::allowed()` logs that at INFO with tag `touch`.
Protocol, storage, preferences, and keys unchanged.

### Compatibility 0.2.27 (firmware stays 0.2.22)

App only. `POST /api/install` (YAML download) has been removed; `POST
/api/firmware/profiles` writes the profile, fills in missing Wi-Fi keys in
`secrets.yaml` (only the missing lines, verified by re-parsing the result;
an invalid file is left untouched), and, given `target` (a reported USB port),
immediately starts `install`. Port and build slot are checked before
writing. The response contains `api_key`; the page shows it once
for the HA pairing. The job object gets `stage` (`config`, `compile`, `upload`).
`profile_meta` provides extra `screen` (profile uses this project's board package)
and `api_key`; `/api/inventory` gets the additive list `pending`, with
such profiles that have no paired screen. Storage, protocol, preferences, and keys
unchanged.

### Compatibility 0.2.26 / firmware 0.2.22

Storage version and tile protocol stay 1. The layout message gets the additive field
`keepalive` (seconds, 5–3600): the interval at which the app repeats the entire layout
(`KEEPALIVE_SECONDS` in `server.py`, 120 s since 0.2.20). The firmware derives its
data-flow monitoring from that (two rounds plus 60 s) and then reports
"ESP Screens not active"; "HA not connected" now comes from ESPHome's own
API connection status (`api_is_connected()`), no longer from the age of the last
message. Without the field, firmware 0.2.22 assumes 120 s; firmware up to 0.2.21 ignores
the field and keeps its fixed 95 s, which, with a 120 s keepalive, causes the known
"HA not connected" flicker. So only change `KEEPALIVE_SECONDS` together
with a firmware flash of existing screens, or keep it under 40 s. No changed
preferences or keys.

### Compatibility 0.2.24 / firmware 0.2.20

Internal only: `Tile::forecast` and `Tile::hours` are vectors (previously fixed arrays in
each tile); protocol and storage unchanged. The wizard YAML gets `power_save_mode: none`
under `wifi:`; the shared packages contain no Wi-Fi block, so existing screens
only change once the user adds the line themselves.

### Compatibility 0.2.23 / firmware 0.2.19

Storage version and tile protocol stay 1. `tiles[].options.controls` is additive:
`none` or a set from `CONTROLS` in `core.py` per domain (climate `setpoint`/`mode`,
switch/light/fan `toggle` plus `brightness`/`speed`, vacuum/cover/timer `buttons`,
cover `position`, media_player `volume`/`playback`, number `stepper`/`slider`,
select `stepper`, scene/script/button `run`). On the wire, the manager sends in
`o.controls` only the set the card actually shows: double-width only,
default display, and without a mini-slider; without a choice, the domain's first
set; with `none`, nothing. Status messages get the attributes `device_class`,
`hvac_action`, and `is_volume_muted` (the only boolean included). Older
firmware ignores the field and shows the regular wide card, hence no
`min_firmware`; the tile panel reports from which firmware it works. An old
editor that omits `controls` keeps the stored choice (like `background`).
The firmware builds the panel lazily per set (`layout_panel`), sends −/+ after 700 ms
as a single `set_temperature`/`set_value`, and leaves the local value in place until HA
reports it (or 10 s). The icon fonts get 18 fixed control glyphs
(`tile_icons.FIXED`). No changed preferences or keys.

Also additive in the same release: `x.hours` (up to eight hours of `t`, `c`, `h`,
`p`, `r`) and `p`/`r` per day in `x.days` for the weather card; `x.last` (unix time)
for scenes, scripts, and buttons; the attributes `humidity`, `wind_speed`,
`wind_speed_unit`, `apparent_temperature`, `fan_modes`, `swing_modes`, `fan_mode`,
and `swing_mode`. `supported_features` may exceed the one-million limit. Older
firmware ignores all these fields. The busy status uses LVGL's spinner; both
board profiles include a hidden `spinner` for that (`busy_spinner_seed`),
otherwise ESPHome compiles `LV_USE_SPINNER` as 0.

### Compatibility 0.2.12 / firmware 0.2.14

Storage version and tile protocol stay 1. Additive: `tiles[].options.size`
(`single`/`wide`), new `display` values (`forecast` for weather, `graph` for
sensors, `digital`/`analog` for `screen.clock`), the built-in entity
`screen.clock`, and the HA domains `sun`, `timer`, and `person`. Status messages
get an optional object `x` with values computed by the manager
(forecast days, sun times in the HA timezone, timer end time as epoch).
Older firmware ignores `size`, new displays, and `x`, but refuses unknown
domains across the whole layout; the manager therefore only sends such a layout once
firmware 0.2.14 or newer is detected (`min_firmware`), holding it back in the meantime. Forecasts
come via `weather.get_forecasts` with `return_response`; without a response,
the weather card stays the regular card. No changed preferences or keys.

### Compatibility 0.2.39 / firmware 0.2.33

Storage version and tile protocol stay 1; no new fields in `screens.json`, no
changed preferences or keys. Everything is additive and, per screen, gated on the
reported screen firmware:

- **Transport.** Firmware 0.2.33 has the API action `screen_message` (one string
  `message`, the complete JSON message, 4096 bytes max). The manager calls
  `esphome.<device_name>_screen_message` as soon as the screen reports 0.2.33+
  (`TRANSPORT_MIN_FIRMWARE` in `core.py`, `Manager.transport`); below that, the
  base64 chunks in the inbox text entity are still used. The action publishes the
  result to the same inbox entity, so the status on the management page stays the same.
  ESPHome's API frame on the ESP32 is 32 KiB; `receive()` refuses anything above 4096.
- **Keepalive.** The layout message gets `rev` (twelve hex characters, `core.revision`
  of the layout message without `rev`); older firmware ignores the field. Firmware
  0.2.33 stores the revision and replies to `{v:1, op:"ping", rev, keepalive}` with
  `Synced`/`Loading tiles` (revision matches) or `Resend needed`
  (otherwise, or after a restart; firmware before 0.2.34 says `Gesynchroniseerd`/`Tegels laden`/`Indeling opnieuw nodig`). A ping refreshes the feed window (`last_received`)
  just like a layout does. The manager pings every `KEEPALIVE_SECONDS` (120 s) and only repeats
  everything after `FULL_REPEAT_SECONDS` (3600 s), or immediately when the inbox reports a status
  from `RESEND_STATES` (`Ready for tile configuration`, `Resend needed`, `Loading tiles`,
  and the Dutch originals of older firmware), with a 120 s guard after the last full send.
  Firmware below 0.2.33 still gets the full repeat every 120 s as before.
  A repeated, unchanged layout message now replies `Synced` instead
  of `Layout received`; `diagnostics/send_layout.py` accepts both, in both languages.
- **Incremental.** `HomeAssistant.dirty` collects the changed entity IDs;
  `Manager.sync_one(..., dirty=...)` builds only those tiles and, if one of them is in
  the top bar, the bar too. `dirty=None` (first round, new registry, was
  offline) builds everything and sends the diffs; `force=True` sends everything. `sent`
  stores `{'layout','header','states','rev'}` per screen. `Manager.screens()`
  replaces `inventory()[0]` in the loop and the updater: only the ESPHome screen entities
  are read, cached on registry identity plus the status of those entities.
  `inventory()` (full) remains for the editor and `save()`.
- **History.** `Manager.history_loop` (its own task in `main()`) fetches one
  `recorder/statistics_during_period` call per window (1/6/24 hours) (`hour` for 24 hours,
  `5minute` below that, `types: [mean, state]`; `mean`, otherwise `state`), and, per
  sensor with no rows, falls back to `GET /history/period`, marking changed
  entities dirty. The `history` field in the status message is unchanged (24
  values, `None` until the first). A sensor tile goes out the door without
  `history` the first time and gets it seconds later; the firmware then shows the graph.
- **Diagnostics (firmware).** The `Uptime` sensor (seconds, every 15 s) is gone;
  replaced by `Opgestart` (`Last boot` since firmware 0.2.34; `platform: uptime`, `type: timestamp`, device
  class timestamp, one value per boot; `time:` is already present). Deliberately a new
  entity: Home Assistant's entity registry keeps the unit `s` for the old
  one and then refuses a timestamp ("has a unit of measurement ... non-numeric
  device class: timestamp", seen on 2026-09-14). The ESPHome integration removes
  the old `sensor.<screen>_uptime` itself as soon as the screen connects with the
  new firmware. Also `debug: update_interval` 300 s, and the four template numbers set to
  `update_interval: never`, published from `apply_screen_settings` when the value
  differs from their last state (so also once after boot).
