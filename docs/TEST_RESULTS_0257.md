# Test results app 0.2.57 / firmware 0.2.49 (2026-09-16)

A code check (2026-09-16) found that settings changed on a screen could jump back: ESP Screens sent its own
copy with every full update, missed changes while it or Home Assistant restarted, refused to store a
reported setting when a tile's entity was gone, and resent the whole screen for every step of a held − or +.
This release makes the screen the owner of its settings (entities in Home Assistant), lets the screen answer
the add-on's pings, fixes the settings tile without Home Assistant, the layout sensors after a Home Assistant
restart and the hourly forecast request, and replaces the editor's settings form with cards. See CHANGELOG
0.2.57 and the compatibility note in docs/RELEASING.md.

## Automated

- Python (`.venv-portal`): 245 tests OK. New: `tests/test_screen_owned_settings.py` (entities in both
  profiles and packages, `SETTING_ENTITIES` against the entity names, the editor view and calls for a screen
  that owns its settings, unknown values while offline or without an entity, the layout message without
  settings, answers and their retries, the `Refused` fallback, forecast kinds, layout sensors after a
  restart, the settings tile before `fresh()`, the editor panel against `settings_screen.h`).
  `tests/test_quiet_resync.py` now expects a reported change to be kept without a resend and with the held
  revision; `tests/test_settings_page.py` checks every row writes through `set()`.
- C++ (`clang++ -std=c++17 -Wall -Wextra -Werror -I. -I components/smart_display`): 14/14 PASS.
  `test_settings_screen` adds `set()`: an unchanged value stores nothing, every key clamps like the page,
  brightness pulls both dim levels down, rotation is refused where the board cannot turn.
- `generate_packages.py --check` green.
- Caught by these tests before release: with a reported setting stored without a resend (new in this
  version), `sync_one` would have recorded the new layout's revision although the screen still held the old
  one, and the next ping would have caused a full resend after all. `sync_one` now keeps the held revision
  when the layout message is not sent.

## Builds (ESPHome 2026.6.2)

Check profiles with the generated packages and local components (same substitutions, API and Wi-Fi blocks
for both versions of the CYD).

| Profile | RAM | Flash | vs 0.2.56 |
|---|---|---|---|
| CYD | 23.4% (76,540 B) | 76.6% (1,406,227 B) | +1,056 B RAM, +9,492 B flash (same check profile at 93fa7e2) |
| Guition | 24.4% (79,916 B) | 18.6% (1,510,555 B) | about +1.2 KB RAM, +13.7 KB flash (against the 0.2.56 Easy build) |

Eight new entities on the CYD, nine on the Guition, each a component object on the heap at boot. Free heap
on real hardware was not measured.

## Firmware on the Mac (host builds, driven over the ESPHome API)

- Settings, Guition 25/25 and CYD 24/24: every setting entity offered in the config category, Rotation only on
  the Guition, the defaults on a fresh screen, numbers, times and switches publish; a number from Home
  Assistant lands through `set()` and is reported once, the same value again reports nothing, standby
  brightness clamps to the brightness, a lower brightness pulls both dim levels down; switches, time, number
  and select land and Home Assistant sees them back; an open settings page shows a change from Home
  Assistant; `screen_message` answers a ping and a layout with status and revision, answers nothing to a
  caller without a call id and logs no warning; the settings tile opens while a tile never got its state;
  a restart keeps what Home Assistant set.
- Wake and Sleep regression (the 0.2.53 host test), Guition 35/35 and CYD 35/35.

## Editor

The real server on a demo home (fake Home Assistant), in a browser: a change sends one PUT and applies,
several quick changes go out in one request, a held + repeats, a screen with firmware 0.2.48 shows the note
about firmware 0.2.49 and still saves with the layout, the top bar's clock format is the same setting, Save
sends no settings, and the cards stack on a phone-sized window.

## Live with Home Assistant 2026.9.1

The Guition firmware ran on the Mac and was added to the owner's Home Assistant through the ESPHome config
flow for the test, then removed. The add-on's own `HomeAssistant` and `Manager` classes ran against it, with
the registry narrowed to the test device so no real screen was touched.

- 34/34 checks: Home Assistant lists `esphome.<node>_screen_message` with `response: {optional: true}`, all 14
  setting entities with the ids the Claude skill names, all in the config category; the editor view reads
  the defaults; a sent layout is confirmed by an answered ping (`Synced` and the revision, 16 ms); a stale
  ping answers `Resend needed` and schedules one retry within 30 s; the keepalive ping is answered; a ping
  without an answer logs no warning; Home Assistant refuses `return_response` for an action without answers
  with "An action which does not return responses can't be called with return_response=True", which the
  fallback recognizes; brightness, the dim-level pull-down and eleven settings at once (switches, times,
  numbers, the select) reach the screen and come back as states; the same values again call nothing;
  invalid values are refused; − and + on the screen's own page reach Home Assistant, the editor view and
  one `esphome.screen_setting` event; no errors in the screen's log.
- The editor against this Home Assistant: a switch and a step in the panel reached the screen in one PUT; two
  presses of − on the screen's page showed in the open panel without a reload; with the screen stopped the
  panel said offline and disabled every control; back online it showed the kept values again.
- Found and fixed during this check: the panel showed default values for an offline screen. It now shows
  them as unknown (— and a switch between on and off).
- Restart: with the Home Assistant time source of the real profiles, every number, time and the select had
  its value at the moment Home Assistant reconnected, before the next minute. The host test profile, which
  takes its time from the Mac, showed them as unknown until that minute; that is the host build only.

## Not tested / for the owner

- Real hardware. Nothing was flashed; Studio 1 updates through Home Assistant. After updating the add-on and
  the screen: the settings page and the new entities agree, a change in ESP Screens shows on the screen and
  one made on the screen shows in ESP Screens, the Settings tile opens, and the screen's settings survive the
  firmware update itself.
- A CYD on firmware 0.2.49 (compiled and host-tested only), including its free heap after boot.
