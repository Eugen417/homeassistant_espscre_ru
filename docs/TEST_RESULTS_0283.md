# Test results app 0.2.83 / firmware 0.2.70 (2026-09-18)

Max asked how hard it is to choose a WLED lamp's modes on the screen ("daar heb je een mega lijst aan modes"), said the
list is dynamic and differs per lamp, wanted it as LVGL/ESPHome-native as the documentation allows, and then added the
other WLED things: presets "en die andere". From six questions on a design page he chose: the sparkles key at the top
right of the colour card (1A), LVGL's roller as the picker for every row (3B), the rows as drawn without preset chips,
effects alphabetically with Solid on top, English names from Home Assistant's own translations, and the effect's name
on the tile while one runs. Mid-build he asked for an explicit confirmation: the check key at the top right of the
picker, in the same slot; the back key sends nothing.

## What changed

- App: `light_effects.py` (the rows of the light's device, the names and icons Home Assistant gives them, the picker's
  names and their pages), `ATTRS` carries `effect`, `extras()` builds `x.rows` and `x.nums` for a light, the manager
  watches the device's selects and numbers with the light, subscribes to `esphome.screen_options` and answers with
  `op: options` (`card_options_loop`), and five firmware-only glyphs join `tile_icons.FIXED` (creation, palette-outline,
  playlist-play, play-speed, brightness-6). Nothing about WLED is hardcoded beyond a preferred order of translation keys.
- Firmware: `effects_page.h` (the page, the picker, the sliders; the model part is LVGL-free), `Extra` gained `effect`,
  `option_rows` and `number_rows`, the state parser reads them, the tile's value line names a running effect
  (`tile_controls::effect_running`), `runtime_tiles::options_request` asks for a picker's names and `op: options`
  answers reach `effects_page::received`. Both profiles: a hidden `roller_seed` so ESPHome compiles LVGL's roller, the
  `color_effects_button` on the colour card, the page's hooks at boot, and its close on dismiss, redraw and live updates.
- Docs: README pictures (`guition-effects.png`, `guition-effects-picker.png`, `cyd-effects.png`, rendered by the host
  program), a bullet in README_EXTENDED, the CHANGELOG.

## Automated

- `tools/check.sh`: Python 463 tests (`tests/test_light_effects.py` is new: rows, names, exclusions, caps, sorting,
  pages under 4096 bytes, who may ask), C++ 19/19 (`tests/test_effects_page.cpp` is new: what has a page, percent and
  value mapping, texts, both boards' sizes; `effect_running`), packages, icons, Vitest, vue-tsc, build and the bundle.
- `tools/check.sh --firmware` (ESPHome 2026.6.2): CYD 1,675,424 B = 91.3 % of the 1,835,008 B slot, +20,288 B against
  0.2.82 (tight band: LVGL's roller is compiled in for the first time, the page's code, and five glyphs in three icon
  fonts), Guition well under its 8 MB slot. The first ESP32 compile caught the usual trap the host build cannot:
  `std::max(0, int - lv_font_get_line_height())` needs an `(int)` cast (int32_t is long there).

## On the host (the real firmware for the Mac, SDL, own API ports 6231/6232)

`effects_host.py` builds each board's package as a host program with probe actions, sends a layout of three lights (a
WLED, a plain light, a Hue with its candle effect) whose messages the real add-on code builds
(`core.extras` + `state_message`), answers the screen's `esphome.screen_options` events with the real
`light_effects.options_for/pages/message`, and checks 35 steps per board, 35/35 on the Guition and 35/35 on the CYD:

- The WLED tile reads "TV Simulator", the plain light "59 %", the Hue "candle".
- The colour card of the WLED shows the sparkles key; a plain light's card has none; a Hue with effects has one.
- The page shows Effect, Color palette, Preset and Playlist with Home Assistant's values (a dash for unknown), Speed
  and Intensity at 50 %, everything inside the screen on both boards.
- A row asks for its names (`esphome.screen_options` with inbox, entity and page 0) and says "Loading..." meanwhile; the
  add-on answers one page with Solid first; the roller shows all 216 and rests on the current effect.
- Turning the drum sends nothing; the check key sends one `light.turn_on` with the effect and closes the picker; the row
  says the choice at once, a stale state does not flip it back, the reported effect settles it; opening the picker again
  uses the names it still holds and confirming the current effect sends nothing.
- A second LVGL pointer drags the drum without sending; the check sends the effect it stands on.
- The palette row asks for the select's options, lists them in Home Assistant's order, rests on Aurora, and a choice
  goes out as `select.select_option`; the back key of the picker sends nothing.
- A slider release sends `number.set_value` on the entity's own scale (80 % of 0-255 is 204); the label follows the
  finger; a reported value keeps the slider, a value changed elsewhere moves it.
- Dark mode draws the page again; closing the page returns to the colour card; closing the cards closes the page too.
- Renders of every state on both boards, light and dark, are what the README shows.

## Not tested

- On real hardware: the roller under a real finger (GT911 and XPT2046), the keys, and the answer path through a live
  Home Assistant with a real WLED. Max's Guition Wallbox at home and his VUELTA are the place for that.
- A select with more names than one message holds (the paging is covered by the Python tests only).
