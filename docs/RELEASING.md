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
2. Run all Python tests with aiohttp installed, all `tests/*.cpp`, the
   generator with `--check`, and compile both Easy Setup profiles plus the
   existing manual profiles. Do that sequentially: profiles with the same
   `DEVICE_NAME` share one build folder, and a parallel build can make an upload
   pick the wrong `firmware.bin`. Check that no secrets are in Git.
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
`static/tile-icons.woff` for the editor (with left bearing equal to xMin, otherwise
icons sit off-center in the browser); run `generate_packages.py` afterward.
The `MDI_GLYPH_*` substitutions are retired: `TILEn_ICON` in manual
profiles must come from the set. No changed preferences or keys.

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
