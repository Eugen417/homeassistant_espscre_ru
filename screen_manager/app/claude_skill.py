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
                  ALERT_MIN_FIRMWARE, ALERT_SUGGESTED_ICONS, BROADCAST_DISMISS, BROADCAST_SHOW, TILE_BACKGROUNDS)

NAME = 'esp-screens'
# claude.ai accepts at most 200 characters; Claude Code picks the skill by this sentence.
DESCRIPTION = ('Show alerts or notifications on ESP Screens (CYD and Guition touchscreens run from Home Assistant): '
               'all screens or one, now or in an automation, with the icons and colors they support.')
TYPES = {'string': 'text', 'int': 'number', 'bool': 'on/off'}

def skill_dir(config=None):
    """Where the skill goes: <Home Assistant configuration>/.claude/skills/esp-screens."""
    return Path(config or os.environ.get('HA_CONFIG', '/homeassistant')) / '.claude' / 'skills' / NAME

def _limit(name, kind):
    if name in ALERT_LIMITS['cyd']:
        return f"CYD {ALERT_LIMITS['cyd'][name]} · Guition {ALERT_LIMITS['guition'][name]} bytes"
    return f'0 to {ALERT_MAX_TIMEOUT} s' if kind == 'int' else ''

def text():
    """SKILL.md: when to use it, the event for every screen, the per-screen action, fields, colors and icons."""
    fields = '\n'.join(f'| `{name}` | {TYPES[kind]} | {help_} | {_limit(name, kind)} |' for name, kind, _, help_, _ in ALERT_FIELDS)
    colors = ', '.join(f"`{name}` ({item['label']})" for name, item in TILE_BACKGROUNDS.items() if item['color'])
    suggested = ', '.join(f'`{name}`' for name in ALERT_SUGGESTED_ICONS)
    groups = '\n'.join(f'- {group}: ' + ', '.join(f'`{name}` ({label})' for name, _, label in icons) for group, icons in tile_icons.GROUPS)
    fixed = ', '.join(f'`{name}`' for name, _ in tile_icons.FIXED)
    endings = ', '.join(f'`{action}` ({label[0].lower() + label[1:]})' for action, label in ALERT_ENDINGS)
    return f'''---
name: {NAME}
description: {DESCRIPTION}
---

# Alerts on ESP Screens

Made by ESP Screen Manager (ESP Screens → Settings → Claude). Installing it again from there replaces this file, so changes made here get lost.

An alert is a card over the whole screen with an icon, a title, a subtitle and one button. It wakes the screen and stays until someone presses the button or the timeout runs out. A new alert replaces the one showing.

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
