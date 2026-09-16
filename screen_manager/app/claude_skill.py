"""The Claude skill ESP Screens offers under Settings → Claude: installed for Claude Code, or downloaded for claude.ai.

Claude Code finds project skills in `.claude/skills/<name>/SKILL.md` of the folder it starts in; the
Claude Code apps for Home Assistant start in the configuration folder. claude.ai takes the same folder as
a zip. The text is built from the same reference as the Alerts cheatsheet, so it never names a field,
color or icon the firmware lacks, and it is the same on every call: status() compares it byte for byte
to tell an outdated copy.
"""
import io
import os
from pathlib import Path
import zipfile

import tile_icons
from core import (ALERT_ENDINGS, ALERT_EVENT, ALERT_FALLBACK_ICON, ALERT_FIELDS, ALERT_LIMITS, ALERT_MAX_TIMEOUT,
                  ALERT_MIN_FIRMWARE, ALERT_SUGGESTED_ICONS, AUTO_STANDBY_MIN_FIRMWARE, BROADCAST_DISMISS, BROADCAST_SHOW,
                  CONTROLS, DISPLAYS, MAX_PAGES, SETTINGS_PAGE_MIN_FIRMWARE, SLOTS_PER_PAGE, TILE_BACKGROUNDS,
                  TILE_EVENTS, TILE_RESULT_EVENT, WAKE_SLEEP_MIN_FIRMWARE, SETTING_ENTITIES_MIN_FIRMWARE)

NAME = 'esp-screens'
# claude.ai accepts at most 200 characters; Claude Code picks the skill by this sentence.
DESCRIPTION = ('ESP Screens (CYD and Guition touchscreens run from Home Assistant): put a tile on a screen, move or '
               'order tiles, show an alert on one screen or all of them, and wake, sleep or keep a screen awake.')
TYPES = {'string': 'text', 'int': 'number', 'bool': 'on/off'}

def skill_dir(config=None):
    """Where the skill goes: <Home Assistant configuration>/.claude/skills/esp-screens."""
    return Path(config or os.environ.get('HA_CONFIG', '/homeassistant')) / '.claude' / 'skills' / NAME

def _limit(name, kind):
    if name in ALERT_LIMITS['cyd']:
        return f"CYD {ALERT_LIMITS['cyd'][name]} · Guition {ALERT_LIMITS['guition'][name]} bytes"
    return f'0 to {ALERT_MAX_TIMEOUT} s' if kind == 'int' else ''

def text():
    """SKILL.md: the tile events and how to read a screen, then the alert event for every screen, the
    per-screen action, fields, colors and icons, and the standby and brightness entities."""
    fields = '\n'.join(f'| `{name}` | {TYPES[kind]} | {help_} | {_limit(name, kind)} |' for name, kind, _, help_, _ in ALERT_FIELDS)
    colors = ', '.join(f"`{name}` ({item['label']})" for name, item in TILE_BACKGROUNDS.items() if item['color'])
    suggested = ', '.join(f'`{name}`' for name in ALERT_SUGGESTED_ICONS)
    groups = '\n'.join(f'- {group}: ' + ', '.join(f'`{name}` ({label})' for name, _, label in icons) for group, icons in tile_icons.GROUPS)
    fixed = ', '.join(f'`{name}`' for name, _ in tile_icons.FIXED)
    endings = ', '.join(f'`{action}` ({label[0].lower() + label[1:]})' for action, label in ALERT_ENDINGS)
    controls = '\n'.join(f'| `{domain}` | ' + ', '.join(f'`{key}` ({label.lower()})' for key, label in choices) + ' |'
                         for domain, choices in CONTROLS.items())
    displays = '\n'.join(f'| `{domain}` | ' + ', '.join(f'`{name}`' for name in names) + ' |' for domain, names in DISPLAYS.items())
    events = '\n'.join(f'| `{event}` | {what} |' for event, what in
                       (('esp_screens_add_tile', 'Puts an entity on a screen, or changes the tile that is already there'),
                        ('esp_screens_remove_tile', 'Takes a tile off a screen'),
                        ('esp_screens_move_tile', 'Moves a tile to another page or spot'),
                        ('esp_screens_order_tiles', 'Puts tiles in the order you give')))
    return f'''---
name: {NAME}
description: {DESCRIPTION}
---

# Tiles, alerts and standby on ESP Screens

Made by ESP Screen Manager (ESP Screens → Settings → Claude). Installing it again from there replaces this file, so changes made here get lost.

Three things Home Assistant can do with the screens: choose what a screen shows ([Tiles](#tiles-on-a-screen)), show an alert (below), and wake a screen, put it to sleep or keep it awake ([Standby and brightness](#standby-and-brightness)).

An alert is a card over the whole screen with an icon, a title, a subtitle and one button. It wakes the screen and stays until someone presses the button or the timeout runs out. A new alert replaces the one showing.

## Tiles on a screen

A screen shows tiles: two columns and {SLOTS_PER_PAGE // 2} rows per page, at most {MAX_PAGES} pages and twenty tiles. A tile is single or double-width; a double-width one starts in the left column and takes two spots.

Fire one of these events and ESP Screens changes that screen and sends it right away, the same way its own editor does.

| Event | What it does |
|---|---|
{events}

```yaml
actions:
  - event: esp_screens_add_tile
    event_data:
      screen: living room
      entity: vacuum.s8
```

### What you can put in the event

| Field | Meaning |
|---|---|
| `screen` | Which screen: its device name, the name Home Assistant shows, its area, or the title on the screen. With one screen paired you can leave this out. |
| `entity` | The entity the tile shows. Use the real entity ID; never invent one. |
| `name` | A name of your own on the tile; leave it out to keep Home Assistant's. |
| `page` | Page, counted from 1. Without a spot the tile takes the first free one on that page. |
| `row`, `column` | An exact spot on that page: row 1 to {SLOTS_PER_PAGE // 2}, column `left` or `right`. |
| `size` | `single` or `wide`. |
| `controls` | What you can operate on the tile itself (see below). |
| `display` | How the tile draws itself (see below). |
| `icon`, `color` | An icon from the list further down, and one of the pastel colors. |
| `tap` | What a tap does: `auto`, `detail` (open the card), `toggle` or `none`. |
| `entities` | Only for `esp_screens_order_tiles`: the entities in the order you want them. |

A tile with a control, a forecast or a sun path is drawn double-width on its own; you don't have to ask for that.

### What a tile can do, per kind of entity

| Entity | `controls` |
|---|---|
{controls}

| Entity | `display` |
|---|---|
{displays}

Everything else shows its name and state, and opens a card of its own on a long press.

### Reading a screen first

Every screen also publishes what it shows, as `sensor.esp_screens_<device name>`: the state is the number of tiles, and the attributes hold `title`, `pages` and `tiles` with `entity`, `name`, `page`, `row`, `column`, `size`, `controls` and `display` per tile. Read that before moving things around, so you know what is already there and where.

Ordering a page means naming the tiles that are on it, in the order you want:

```yaml
actions:
  - event: esp_screens_order_tiles
    event_data:
      screen: living room
      page: 1
      entities: [light.kitchen, light.dining, vacuum.s8]
```

A tile from another page has to be moved there first (`esp_screens_move_tile` with `page`). Leave `page` out to order the whole screen: the entities you name come first, the rest keeps its order behind them.

### What comes back

ESP Screens answers every event with `{TILE_RESULT_EVENT}`, carrying `ok`, the `screen`, the `entity` and, when it refused, an `error` that says why ("No screen called ...", "That spot is taken by ...", "This screen already has twenty tiles"). The add-on log says the same. Nothing changes on a refused event.

### How to work

1. Read the screen's sensor, and Home Assistant's own entities for what the user names. Ask which screen when several could fit.
2. Say what you are going to do: which tile, which screen, which spot. A tile appears on a screen in someone's house, so wait for a yes before firing the event.
3. Fire the events one at a time and read `{TILE_RESULT_EVENT}` (or the sensor) before the next one.
4. Sorting by how much something is used comes from Home Assistant itself (history or the logbook), not from the screen.

## All screens: fire an event

ESP Screen Manager listens for two Home Assistant events and passes them on to every paired screen that is online, screens added later included. Screens need firmware {ALERT_MIN_FIRMWARE} or newer, and the app has to be running.

```yaml
actions:
  - event: {BROADCAST_SHOW}
    event_data:
      title: "Mail!"
      subtitle: "There is post in the mailbox"
      icon: mailbox
      color: orange
      button_text: "OK"
      timeout: 0
      flash: true
```

`{BROADCAST_DISMISS}` (no data) takes the alert off every screen. Every field of the event is optional: a missing or unusable value becomes empty text, `0` or off. Templates in `event_data` work as usual. For an alert right now, without an automation, fire the same event through Home Assistant's REST API (`POST /api/events/{BROADCAST_SHOW}` with the fields as JSON; inside a Home Assistant app such as Claude Code that is `http://supervisor/core/api/events/{BROADCAST_SHOW}` with `$SUPERVISOR_TOKEN` as the bearer token) or under Developer tools → Events. The ESP Screen Manager log tells how many screens got it.

## One screen: call its action

Every screen also has its own actions, `esphome.<device_name>_show_alert` and `esphome.<device_name>_dismiss_alert` (dashes in the device name become underscores). List Home Assistant's `esphome` actions to see which screens exist. These actions need all seven fields; send `""`, `0` or `false` for the ones you don't use.

```yaml
actions:
  - action: esphome.kitchen_screen_show_alert
    data:
      title: "Laundry is done"
      subtitle: ""
      icon: washing-machine
      color: green
      button_text: ""
      timeout: 600
      flash: false
```

## Fields

| Field | Type | Meaning | Limit |
|---|---|---|---|
{fields}

Text limits are in bytes; an accented letter takes two. Keep the title short, the CYD shows about forty characters on one line, and put details in the subtitle.

## Colors

`color` takes one of {colors}. Empty or unknown gives the white card.

## Icons

`icon` takes one of the names below, exactly as written (`mdi:` in front works too). The firmware carries only these glyphs and shows the warning triangle (`{ALERT_FALLBACK_ICON}`) for any other name, so pick the closest name from this list rather than another Material Design icon.

Good for alerts: {suggested}.

{groups}
- Also available: {fixed}

## When an alert ends

Each screen fires `{ALERT_EVENT}` with `action`: {endings}. The event also carries `title`, `screen` (the device name) and `device_id`. To react to the button:

```yaml
  - wait_for_trigger:
      - trigger: event
        event_type: {ALERT_EVENT}
        event_data:
          action: ok
    timeout: "00:05:00"
```

## Writing the automation

1. Find the real trigger in Home Assistant, such as the mailbox sensor. Never invent an entity ID; ask when several entities could fit.
2. All screens: the `{BROADCAST_SHOW}` event. One screen the user names: that screen's own action.
3. When the situation clears (mailbox emptied, door closed), fire `{BROADCAST_DISMISS}` from the same automation.
4. Take the icon and color from the lists above.
5. Ask before sending a test alert: it lights up the screens around the house.

Example, with the user's own entity in place of `binary_sensor.mailbox`:

```yaml
alias: Mailbox alert on the screens
triggers:
  - trigger: state
    entity_id: binary_sensor.mailbox
    to: "on"
    id: mail
  - trigger: state
    entity_id: binary_sensor.mailbox
    to: "off"
    id: emptied
actions:
  - choose:
      - conditions:
          - condition: trigger
            id: mail
        sequence:
          - event: {BROADCAST_SHOW}
            event_data:
              title: "Mail!"
              subtitle: "There is post in the mailbox"
              icon: mailbox
              color: orange
              timeout: 0
              flash: true
      - conditions:
          - condition: trigger
            id: emptied
        sequence:
          - event: {BROADCAST_DISMISS}
```

## Standby and brightness

Every screen has these entities in Home Assistant, on its ESPHome device. `<screen>` stands for the start Home Assistant gave the screen's entity IDs, the same as in `text.<screen>_tile_settings`; look them up rather than guessing.

| Entity | What it does |
|---|---|
| `button.<screen>_wake` | Wakes the screen the way a tap does and counts the standby time from that moment; a screen that is already on only restarts that count. Firmware {WAKE_SLEEP_MIN_FIRMWARE} or newer. |
| `button.<screen>_sleep` | Puts the screen in standby right away, also with Auto standby off, and closes an alert that is showing. It stays in standby until someone taps it, Wake is pressed or an alert arrives. Firmware {WAKE_SLEEP_MIN_FIRMWARE} or newer. |
| `switch.<screen>_auto_standby` | On: the screen dims after the standby time without a touch. Off: the screen wakes up and stays on, except after Sleep, which holds until Wake, a tap or an alert. Firmware {AUTO_STANDBY_MIN_FIRMWARE} or newer. |
| `number.<screen>_standby_after` | Seconds without a touch before standby, 60 to 86400. |
| `number.<screen>_normal_brightness` | Brightness while in use, 5 to 100 %. |
| `number.<screen>_standby_brightness` | Brightness in standby, 0 to 100 %, at most the normal brightness. |
| `number.<screen>_night_brightness` | Brightness in standby during the night hours, 0 to 100 %. |
| `switch.<screen>_night_mode` | Night mode: the night brightness between the two times below. Firmware {SETTING_ENTITIES_MIN_FIRMWARE} or newer, like every row below it. |
| `time.<screen>_night_starts`, `time.<screen>_night_ends` | The night hours; set them with `time.set_value`. |
| `switch.<screen>_24_hour_clock` | On: 24-hour clock. Off: 12-hour clock. |
| `switch.<screen>_back_to_page_1` | On: after `number.<screen>_back_to_page_1_after` seconds without a touch (30 to 3600) the screen closes a card and goes back to page 1. |
| `switch.<screen>_back_to_page_1_on_standby` | On: going into standby also goes back to page 1. |
| `switch.<screen>_swipe_between_pages` | On: swipe between pages. |
| `select.<screen>_rotation` | Guition only: `0°`, `90°`, `180°` or `270°`. |

Wake and Sleep are buttons: press them with the `button.press` action. They save nothing, so an automation may press them as often as it likes, on every motion too. To reach several screens at once, list their buttons under `entity_id`. Don't target an area or a device with `button.press`: that presses every other button there as well, the Wake and Sleep of the same screen included.

```yaml
alias: Screens wake on motion and sleep when the TV goes on
triggers:
  - trigger: state
    entity_id: binary_sensor.hallway_motion
    to: "on"
    id: motion
  - trigger: state
    entity_id: media_player.living_room_tv
    to: "on"
    id: tv
actions:
  - choose:
      - conditions:
          - condition: trigger
            id: motion
        sequence:
          - action: button.press
            target:
              entity_id: button.hallway_screen_wake
      - conditions:
          - condition: trigger
            id: tv
        sequence:
          - action: button.press
            target:
              entity_id:
                - button.living_room_screen_sleep
                - button.kitchen_screen_sleep
```

These switches, numbers, times and the select are the screen's own settings, the same as on its settings page and in ESP Screens: a change made from Home Assistant shows there too and stays after a restart. Turning Auto standby on again counts the standby time from that moment. Every change is saved on the screen, so switch on changes that happen a few times a day (someone comes home, a light goes on, a window opens), never on every motion: press Wake for that.

To keep screens on while something is going on at home, turn Auto standby off when the situation starts and on when it ends, all from one automation. Example with placeholders for the user's own entities:

```yaml
alias: Keep the screens awake
mode: restart
triggers:
  - trigger: state
    entity_id:
      - person.alex
      - light.living_room
      - binary_sensor.bedroom_window
actions:
  - if:
      - condition: state
        entity_id: person.alex
        state: home
      - condition: or
        conditions:
          - condition: state
            entity_id: light.living_room
            state: "on"
          - condition: state
            entity_id: binary_sensor.bedroom_window
            state: "on"
    then:
      - action: switch.turn_off
        target:
          entity_id: switch.kitchen_screen_auto_standby
    else:
      - action: switch.turn_on
        target:
          entity_id: switch.kitchen_screen_auto_standby
```

Replace the person, light, window and screen with the real entity IDs (ask which screens when there are several), and leave out conditions the user did not ask for.

## The settings page on the screen itself

Firmware {SETTINGS_PAGE_MIN_FIRMWARE} or newer carries a settings page the user can open at the panel: hold the top bar (the strip with the screen's name and the clock) for about a second and a half, until the blue line along the top edge is full. A screen can also carry a tile for it: put the entity `screen.settings` on a screen like any other tile.

It holds Brightness, Night, Screen (clock, going back to page 1, swiping, rotation) and This screen (name, address, firmware, whether Home Assistant is connected, and Restart). A change there is saved on the screen and shows up in ESP Screens within a second, exactly like a change made from Home Assistant.

You can open it for someone who is standing at the panel:

```yaml
actions:
  - action: esphome.kitchen_screen_open_settings
    data:
      page: 1
```

`page` 0 is the menu, 1 Brightness, 2 Night, 3 Screen, 4 This screen, and -1 closes the page and puts the screen back on page 1. Use it to walk someone through a setting ("I opened Brightness on the kitchen screen"), not to change something yourself: for that, use the entities above, which do the same thing without anyone at the panel.
'''

def status(directory):
    """Whether the skill is there and equal to what this app version writes."""
    try:
        installed = (directory / 'SKILL.md').read_text(encoding='utf8')
    except FileNotFoundError:
        return {'installed': False, 'current': False, 'path': str(directory)}
    except (OSError, UnicodeDecodeError):
        installed = None
    return {'installed': True, 'current': installed == text(), 'path': str(directory)}

def archive():
    """The skill folder as a zip, the way claude.ai takes an uploaded skill: esp-screens/SKILL.md."""
    buffer = io.BytesIO()
    with zipfile.ZipFile(buffer, 'w', zipfile.ZIP_DEFLATED) as bundle:
        entry = zipfile.ZipInfo(f'{NAME}/SKILL.md', date_time=(2026, 1, 1, 0, 0, 0))
        entry.compress_type = zipfile.ZIP_DEFLATED
        entry.external_attr = 0o644 << 16
        bundle.writestr(entry, text())
    return buffer.getvalue()

def install(directory):
    """Write SKILL.md, replacing an older copy. `restart`: the skills folder is new, and Claude Code only
    watches skills folders that existed when it started."""
    restart = not directory.parent.is_dir()
    try:
        directory.mkdir(parents=True, exist_ok=True)
        temp = directory / 'SKILL.md.tmp'
        temp.write_text(text(), encoding='utf8')
        temp.replace(directory / 'SKILL.md')
    except OSError as error:
        raise ValueError(f"Couldn't write the skill to {directory} ({error.strerror or type(error).__name__}).") from error
    return {**status(directory), 'restart': restart}
