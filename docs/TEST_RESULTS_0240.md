# Test results app 0.2.40 / firmware 0.2.34 (2026-09-15)

ESP Screens in English: the screens, the management page, the Home Assistant entities
and the docs. See CHANGELOG 0.2.40 and RELEASING "Compatibility 0.2.40".

## What changed

- Text only on both boards and in the app: every on-screen label, status reply, log
  line, editor string and guide. Labels follow Home Assistant's English names (climate
  modes Heat/Cool, actions Heating/Cooling, Away, Returning to dock, weather conditions),
  dates use English abbreviations (Mo 14 Sep), the top bar writes 21.5 °C and 1,249 W.
- Entity names in English (Tile settings, Screen firmware, Device name, IP address,
  Guition screen type, Last boot, Normal/Standby/Night brightness, Standby after,
  Calibrate touch). Home Assistant gives renamed ESPHome entities new entity IDs.
- App: `core.NAME_*` pairs and `RESEND_STATES` accept both languages;
  `Manager.follow_renamed_inboxes` moves a screen's layout and update history to its new
  inbox ID (`core.inbox_prefix`, `core.device_prefixes`, `Manager.aliases`,
  `Updater.current_id`/`renamed`).
- Docs use Home Assistant 2026.9's menu names (Settings → Apps, App store, Settings → Tools);
  guides renamed to `docs/TILES.md`, `CALIBRATING.md`, `ACCEPTANCE.md`, `TROUBLESHOOTING.md`.

## Review method

- Token-level diff of every changed Python, JavaScript, C++ and YAML file with string
  literals and comments neutralised (YAML compared as parsed trees, lambdas as C++): 33
  code changes, all intended (dual matching, singular hour/year, number format, a log
  filter). Every changed string that sits in a comparison, key or subscript was checked.
- No Home Assistant identifier (entity ID, `domain.service`, state or attribute name,
  `mdi:` icon, event type, YAML key in doc examples) disappeared from any translated string.
  UI terms and state labels were checked against the English translations of the running
  HA 2026.9.1 (`frontend/get_translations` and the frontend's own `en` files).
- The 0.2.39 test suites were run against the new code: 15 Python and 4 C++ failures,
  every one an assertion on Dutch text; nothing behavioural.
- A dictionary sweep of all string literals and every `text:` value in both board
  profiles found Dutch labels the translation had missed (page navigation, climate
  card, vacuum status, calibration hints, one entity name); fixed.

## Automated

- Python (`.venv-portal`): 139 tests OK, 9 new in `test_renamed_inbox.py` (slugs and
  prefixes, discovery in both languages, a reflash moves layout plus update address and
  result, lookups and saves through the old ID, inventory, inspector and save endpoints
  called with the old ID, restart after a reflash adopts by device
  prefix, other screens' orphaned layouts stay put, an existing layout on the new ID is
  never overwritten, an update round that renames mid-update records success under the new
  ID, a failing nightly round still notifies).
- C++ (clang++ -std=c++17 -Wall -Wextra -Werror -I.): 13/13 PASS.
- `generate_packages.py --check` green.
- ESPHome 2026.6.2 compile SUCCESS: `easy-cyd-device.yaml` (RAM 31.5%, flash 77.3%),
  `easy-guition-device.yaml` (RAM 32.5%, flash 18.6%), manual `device.yaml` and
  `guition-device.yaml`.

## Hardware and Home Assistant (Home Assistant Yellow, HA 2026.9.1)

- Local add-on 0.2.40 (SMB + Supervisor), GitHub add-on 0.2.39 stopped with its data kept
  (Supervisor backup first).
- Guition on firmware 0.2.33 (Dutch entities) with app 0.2.40: discovered by its Dutch
  names (firmware, board, node, IP), layout sent, status `Gesynchroniseerd`, top bar in
  English number format, update offer 0.2.34 with profile and address.
- USB flash of the CYD with English entities: HA removed `text.cyd_2_8in_display_tegelinstellingen`
  and created `text.cyd_2_8in_display_tile_settings` in the same second (also the other
  renamed entities); the inbox reported `Ready for tile configuration`.
- USB flash of the Guition to 0.2.34 while app 0.2.40 ran: log
  `Screen text.guition_wallbox_tegelinstellingen now reports as text.guition_wallbox_tile_settings; moved its layout`;
  all 10 tiles, 2 pages, settings and top bar back on screen within seconds.
- Both boards on 0.2.34 (`device_info` compile times match the builds), status `Synced`.
- Rollback: USB flash of the CYD back to 0.2.33 while app 0.2.40 ran; the app moved the layout
  back to `text.cyd_2_8in_display_tegelinstellingen` (status `Gesynchroniseerd`, 8 tiles).
- **Update** button on the Yellow for that 0.2.33 CYD (profile pointed at a local copy of this
  commit's package): ESPHome inside the add-on built and uploaded over Wi-Fi (23 min, first build
  with a fresh toolchain); the screen came back as `text.cyd_2_8in_display_tile_settings` on 0.2.34,
  the round followed it through verify and settle and recorded "Updated to firmware 0.2.34." under
  the new ID; layout intact, no update offered afterwards.
- Guition LVGL captures of every card type and detail card (demo layout, direct controls,
  weather forecast, sensor graphs, person, timer, sun path, climate, vacuum, media,
  cover, fan, number, select, scene, script), the climate mode picker and light controls:
  all English, nothing clipped except long weather names in the five-day list (ellipsis, as before).
- Firmware self-tests: CYD 10 page checks and 50 overlay cycles PASS; Guition page checks
  PASS with 18 tiles, light sliders PASS.
- Alerts through Home Assistant: `show_alert` with a timeout (event `timeout`), empty
  title falls back to "Notification", a new alert replaces the old one (`replaced`),
  `dismiss_alert` (`remote`); every ending arrived as `esphome.screen_alert`.
- Settings: `number.guition_wallbox_normal_brightness` 100 → 90 → 100 in HA; the app
  stored each value within a second.
- Editor through ingress: screens, top bar sheet (date preview "Tu 15 Sep"), tile sheet,
  Firmware & USB, New screen, Alerts cheatsheet (action names, fields, icons), saving a
  new title reached the screen and was restored; no console errors.

## Not tested / for the owner

- The physical look of the alert card: the LVGL capture does not include the top layer.
- Touch on the physical screens after the flash (the owner's check).
- The Supervisor reports an add-on as `error` after a stop; 0.2.39 does the same, not changed here.
