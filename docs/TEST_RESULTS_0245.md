# Test results app 0.2.45 (firmware stays 0.2.38, 2026-09-15)

One alert for every screen (`esp_screens_show_alert` / `esp_screens_dismiss_alert`), the Claude skill
(install for Claude Code or download for claude.ai), the Settings page and grey screen cards. See
CHANGELOG 0.2.45 and RELEASING "Compatibility 0.2.45". App only; no firmware or package changes.

## Automated

- Python (`.venv-portal`): 164 tests OK, also in a clean worktree at the release parent with only
  this release's changes applied.
  - New: `tests/test_alert_broadcast.py` covers field typing, target selection, fan-out, failures,
    queue order, and that the reader only queues.
  - New: `tests/test_claude_skill.py` covers skill text, YAML examples, zip, install/update and the endpoints.
  - New: `tests/test_settings_view.py` covers the header, Settings contents, view switching and the grey cards.
- `node --check app.js` clean.
- No C++ or ESPHome builds: firmware code, board profiles and packages are unchanged.

## Live Home Assistant (owner's HA 2026.9.1, dry run)

- The new `HomeAssistant` and `Manager` code ran from a development machine against the owner's
  Home Assistant with a long-lived token. Screen actions were recorded, not sent.
- `POST /api/events/esp_screens_show_alert` with `timeout: "5"`, `flash: "yes"`, `button_text: true`:
  - the subscription received it;
  - the log said `esp_screens_show_alert: 2 of 2 screens`, with a warning that `button_text` was left empty;
  - calls were recorded for `esphome.guition_wallbox_show_alert` and `esphome.cyd_2432s028_show_alert`
    with `timeout: 5`, `flash: true`, `button_text: ""`.
- `esp_screens_dismiss_alert` gave both `_dismiss_alert` calls with empty data.
- Home Assistant lists both per-screen actions with the same seven fields.

## Editor (local server with a fake Home Assistant, desktop 1280 px and phone 375 px)

- The header shows one Settings button. `#settings` survives a reload; the header button, "← My screens"
  and browser back/forward switch the views.
- Settings → New screen opens and closes the installer. Firmware & USB opens the firmware window
  (`GET /api/firmware` 200).
- The nightly update checkbox on/off gives `PUT /api/updates` 200 with both toasts. Update all
  (forced to two pending screens) posts `/api/updates/run` with the CSRF header.
- Alerts opens the cheatsheet. "Copy YAML" under All screens copies the event example.
- Claude:
  - Install shows the restart toast when the skills folder is new, then "● Installed".
  - After a changed skill text it shows "Installed · a newer version is ready" with "Update the skill".
  - The zip download returns `application/zip` with `esp-screens/SKILL.md`.
- My screens: the unselected screen sits on the grey card (`#e9ecf1`), the selected one is white.
- No console errors; every API request 200. At 375 px the Settings cards stack, without
  horizontal scrolling.
- README images `editor.png` and `editor-settings.png` come from the demo home (`.esphome/readme-render`).

## Not tested / for the owner

- The released app inside Home Assistant (ingress, Supervisor token, `/homeassistant` mount):
  - the Install button writing `/homeassistant/.claude/skills/esp-screens/SKILL.md`;
  - the zip download inside the HA iframe.
- A real alert on the screens through the event (the dry run did not call the screens).
- Claude Code in Home Assistant picking up the skill and writing the mailbox automation.
- The claude.ai upload of the zip.
