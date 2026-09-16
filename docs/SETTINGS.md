# Settings: on the screen and in ESP Screens

Every screen setting lives in two places at once: the **Screen settings** block of the add-on editor, and
the **settings page on the screen itself** (firmware 0.2.44+). This page says what that page is made of,
and exactly what adding one more setting takes.

## What the user sees

Holding the top bar of the overview for about one and a half seconds opens the page; a blue line grows
along the top edge while you hold, and letting go before it finishes cancels. A screen can also carry a
`screen.settings` tile, and Home Assistant can open it with `esphome.<screen>_open_settings`.

The page is a menu of groups, each of which opens a page of its own:

| Group | Rows |
|---|---|
| Brightness | Brightness, Auto standby, Standby after, Standby brightness |
| Night | Night mode, Starts, Ends, Night brightness |
| Screen | Clock, Back to page 1, After, Also on standby, Swipe between pages, Rotation (boards that turn) |
| This screen | Screen, Address, Firmware, Home Assistant, Restart |

Every change is saved on the screen, applied at once, and sent to the add-on as an
`esphome.screen_setting` event, so the editor shows the same value within a second. The add-on remains
the owner: its next layout message repeats every setting, and a screen that was offline catches up then.

## The rules the page follows

- **Two levels, never three.** A group page is the deepest place a setting can live.
- **No free scrolling.** A group that does not fit gets the same `< Previous / Next >` pager as the tile
  pages. A 320x240 board shows five rows, a 480x480 board six.
- **A row is a control, not a form.** Toggles flip on tap, numbers and times have `-` and `+`, a choice
  cycles through its options in a chip on the right.
- **A row that depends on a switch above it is greyed out, not hidden**, so the page never jumps around.
- **Only settings that belong to this piece of glass.** Tiles, the top bar and pages belong to the editor,
  which has a mouse and a keyboard; the screen gets what you want to change while standing in front of it.

## Adding a setting

The example below adds "Beep on touch" (`beep`), a switch.

### 1. Where the value lives

Two homes, and the choice is fixed by history:

- `screen_settings::Settings` in `components/smart_display/screen_settings.h` is the **frozen** block of
  eleven values that ESP Screens sends as `settings`. Its format is version 1 and changing it needs a
  preference migration *and* firmware-version gating in the add-on, because older firmware refuses a
  `settings` object that does not have exactly its own keys. Don't.
- Anything new is a plain value in `settings_screen.h` next to `swipe_pages`, `rotation` and `auto_home`,
  with its own preference and its own key in the layout message. That is backward and forward
  compatible in both directions: old firmware ignores the key, a new screen with an old add-on keeps its
  saved value.

```cpp
// settings_screen.h, next to the others
inline int32_t swipe_pages = 0, rotation = 0, auto_home = 1, auto_home_seconds = 120, beep = 0;
```

Storing it (`components/smart_display/runtime_tiles.h`): extend `HomeTimeout`-style records or add one of
your own, load it in `load_settings()`, write it in `persist_settings()`, and accept the key in
`receive()` next to `auto_home` — validate first, then save.

### 2. The row

One line in the table of the group it belongs to, in `settings_screen.h`:

```cpp
toggle("Beep on touch", [] { return beep; },
       [](int32_t value) { beep = value ? 1 : 0; changed("beep", beep); }),
```

`changed(key, value)` saves, applies and reports in one go; `key` must be the add-on's key exactly.
The row kinds are `toggle`, `number` (fixed step, optional unit), `duration` (seconds, steps grow with
the value), `moment` (minutes since midnight, quarters by tap and hours while held), `choice`, `info`
and `action`. A row can carry `shown` (leave it out on boards that lack the hardware) and `enabled`
(grey while the switch it depends on is off).

Watch the count: a group of more than five rows gets a pager on a 320x240 board. Six is the maximum
that still fits a 480x480 board in one go.

### 3. The add-on

- `screen_manager/app/core.py`: one line in `SETTING_RULES` (`'beep': (False, None, None)`), which gives
  you validation, storage and the write-back from the screen for free.
- `screen_manager/app/server.py`: add the key to `extra` in `layout_message` and send it as its own key,
  next to `auto_home`. Never let it into the `settings` block.
- `screen_manager/app/static/app.js`: one line in `settingDefinitions` for the editor's own control.
- Needs new firmware? `min_firmware()` in `core.py` keeps the screen from being told about something it
  cannot do.

### 4. Tests and proof

- `tests/test_settings_screen.cpp` (`clang++ -std=c++17 -Wall -Wextra -Werror -I.`) walks every page and
  every row: a new row is checked by the table test as soon as it exists.
- `tests/test_settings_view.py` and `tests/test_layout.py` cover the add-on side.
- Render it before believing it: `.esphome/readme-render/host_build.py <board> --compile` builds the real
  firmware for the Mac and `render_settings.py <board>` drives it over the API and saves PNGs of every
  page, including the hold gesture, the time picker and the settings tile.
