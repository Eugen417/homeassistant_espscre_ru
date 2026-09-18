# Test results 0.2.73 (firmware 0.2.61, unchanged)

Date: 2026-09-18. Branch `editor-vue`, rebased on 9f5694b (0.2.72). The firmware and the packages did not change;
this release is the editor (web/, Vue 3 + Vite) and four additive add-on endpoints.

## Automated

- Python: `.venv-portal/bin/python -m unittest discover -s tests` → 398 tests OK. New: `tests/test_editor_live.py`
  (`/api/states`, `/api/screens/{inbox}/identify`, `/api/alerts/test`, the changelog in the update summary, and the
  page side of every new feature read from the Vue sources). The nine tests that used to read `static/app.js` now
  read the sources through `tests/editor_sources.py`; `tests/test_page_assets.py` checks the committed Vite build.
- Editor: `cd web && npm test` → Vitest, 37 tests OK (grid rules, top bar rules, store, TileCard, Library,
  CommandPalette in jsdom). `npm run check` (vue-tsc) clean. `npm run build` → `screen_manager/app/static`.
- `tools/generate_icons.py --check` → 150 pickable icons, 309 glyphs verified (the editor font now lives in
  `web/src/assets`).

## By hand, on the demo home (real add-on code, fake Home Assistant, port 8199)

- Choose a screen, tile drawer with every field, top bar drawer with the live sample, add to the top bar, Screen
  settings tab, add an entity from the library and Save & send (PUT arrives, chip says Saved), drag a tile onto
  another (swap), remove with Undo, Esc closes the drawer, ··· menu, Override YAML with the example, New screen,
  Firmware & USB, Alerts, Settings; dark mode; 375 px layout.
- New in this release: live values on every tile (temperature with unit, On/Off with a lit icon, setpoint, volume and
  position fills, the playing title), Identify (show_alert with flash reaches the demo HA), Try it to one screen and
  to all (two calls logged), Copy layout from the other screen, Export (download + clipboard), room filter, Hide
  placed, lit and grey avatars, ⌘K search that adds an entity, What's new under the Update badge.

## On Max's own Home Assistant

- The first build of this editor ran as a local add-on (same code, before the six features) against his real screens
  and Home Assistant: "ik vind de rest van de editor ook echt geweldig, heb hem getest".

## Not exercised

- The progress bar of a running firmware update (the demo home cannot build firmware); only the mapping from phase
  and ESPHome stage to a percentage is unit-tested.
- Drag & drop on a phone (touch), and the six new features inside the Home Assistant ingress iframe.
