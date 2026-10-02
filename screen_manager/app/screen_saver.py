"""The screensaver (app 0.4.48, firmware 0.29.0): what a screen shows when Auto standby dims it, instead of its tiles.

Each screen has its own choice, kept per Home Assistant device as the screen's label is (screen_labels.py), so it
outlives a renamed inbox and leaves the layouts' storage alone: a media player, a camera and the order the screen tries
them in, with the clock as a step of its own. This app decides which step is available right now (a player that plays
and has a cover, a camera Home Assistant has, the clock always) and tells the screen in one small message whenever that
changes. The screen asks for its picture the way a camera's full view does, and the app makes one picture of the whole
glass (camera_feed.encode_saver): a camera filling it, a cover filling it or beside its own colour, all a little darker
for the words over it. Nothing on it takes a tap, so the first touch only wakes the screen.

A board without pictures (the CYD and the other boards without PSRAM) shows the clock; its editor offers nothing else."""
import json
import logging
import os
import re
import tempfile

LOG = logging.getLogger(__name__)

FEATURE = 'screensaver'
KINDS = ('media', 'camera', 'clock')
PICTURE_KINDS = frozenset(('media', 'camera'))
# `weather` (app 0.4.52): whose temperature the clock shows under the time. 'auto' takes Home Assistant's first weather
# entity that reports one, '' shows none, and a weather entity of your choice is that one.
DEFAULT = {'show': False, 'media': '', 'camera': '', 'order': list(KINDS), 'off': [], 'weather': 'auto'}
ENTITY = re.compile(r'[a-z0-9_]+\.[a-z0-9_]+')
DOMAINS = {'media': ('media_player',), 'camera': ('camera', 'image')}
# What a player does while its cover counts: playing, as Home Assistant's own media card shows its art.
PLAYING = frozenset(('playing',))
GONE = frozenset(('', 'unavailable', 'unknown'))


def validate(value):
    """A clean copy of a screensaver choice, or ValueError. Every field is optional; the default fills the rest."""
    if not isinstance(value, dict) or set(value) - set(DEFAULT):
        raise ValueError('screensaver fields')
    result = {**DEFAULT, 'order': list(DEFAULT['order']), 'off': []}
    if 'show' in value:
        if not isinstance(value['show'], bool):
            raise ValueError('screensaver show')
        result['show'] = value['show']
    for kind in ('media', 'camera'):
        entity = value.get(kind, '')
        if not isinstance(entity, str) or (entity and (len(entity) > 120 or not ENTITY.fullmatch(entity)
                                                        or entity.split('.')[0] not in DOMAINS[kind])):
            raise ValueError(f'screensaver {kind}')
        result[kind] = entity
    if 'weather' in value:
        weather = value['weather']
        if not isinstance(weather, str) or (weather not in ('auto', '') and (
                len(weather) > 120 or not ENTITY.fullmatch(weather) or weather.split('.')[0] != 'weather')):
            raise ValueError('screensaver weather')
        result['weather'] = weather
    if 'order' in value:
        order = value['order']
        if not isinstance(order, list) or sorted(order) != sorted(KINDS):
            raise ValueError('screensaver order')
        result['order'] = list(order)
    if 'off' in value:
        off = value['off']
        if not isinstance(off, list) or any(kind not in KINDS for kind in off) or len(set(off)) != len(off):
            raise ValueError('screensaver off')
        result['off'] = [kind for kind in result['order'] if kind in off]
    return result


def _degrees(state):
    """A weather entity's temperature as a number, or None. Home Assistant gives it in the unit system it is set to."""
    value = ((state or {}).get('attributes') or {}).get('temperature')
    if isinstance(value, bool) or not isinstance(value, (int, float)) or value != value or abs(value) > 999:
        return None
    return value


def weather_entity(choice, states):
    """The weather entity whose temperature the clock shows, or '' for none: the one chosen, or with 'auto' the forecast
    Home Assistant sets up for its home (Met.no's `weather.forecast_<home>`) and else the first weather entity by its id,
    of those that report a temperature right now."""
    wanted = (choice or {}).get('weather', 'auto')
    if wanted != 'auto':
        return wanted
    found = [entity for entity in states if entity.startswith('weather.') and _degrees(states[entity]) is not None]
    return min(found, key=lambda entity: (not entity.startswith('weather.forecast_'), entity), default='')


def temperature(choice, states):
    """The outside temperature as the clock shows it, whole degrees and the sign alone ("21°"), or '' without one. The
    number is Home Assistant's own, so Celsius or Fahrenheit as it is set there."""
    entity = weather_entity(choice, states)
    value = _degrees(states.get(entity)) if entity else None
    if value is None or (states.get(entity) or {}).get('state', '') in GONE:
        return ''
    return f'{round(value) or 0}°'


def entities(choice, states=None):
    """The entities a choice follows; with the states, also the weather entity its clock reads."""
    found = {choice[kind] for kind in ('media', 'camera') if choice.get(kind)}
    weather = weather_entity(choice, states) if states is not None and choice.get('show') else ''
    return found | ({weather} if weather else set())


def available(kind, choice, states, pictures):
    """Whether a step can show now: a player that plays with a cover, a camera Home Assistant has, the clock always.
    The two pictures need a board that draws them."""
    if kind == 'clock':
        return True
    if not pictures or not choice.get(kind):
        return False
    state = states.get(choice[kind]) or {}
    if kind == 'camera':
        return state.get('state', '') not in GONE
    attrs = state.get('attributes') or {}
    return state.get('state') in PLAYING and any(isinstance(attrs.get(name), str) and attrs.get(name)
                                                  for name in ('entity_picture_local', 'entity_picture'))


def pick(choice, states, pictures):
    """The first step of the order that is on and available, or '' for none (standby shows the dimmed tiles as before)."""
    if not choice or not choice.get('show'):
        return ''
    for kind in choice.get('order', KINDS):
        if kind not in choice.get('off', ()) and available(kind, choice, states, pictures):
            return kind
    return ''


def message(choice, states, pictures, short, media_extras, ground=None):
    """The screen message for what the screensaver shows now. `short(text, n)` cuts a text the way every message does,
    `media_extras(attrs)` is the media card's (core.media_extras), `ground(entity, attrs)` the cover's colours."""
    kind = pick(choice, states, pictures)
    result = {'op': 'saver', 'k': kind}
    if kind == 'clock':
        # The outside temperature under the time (app 0.4.52, firmware 0.31.0+); older firmware reads past it.
        degrees = temperature(choice, states)
        if degrees:
            result['w'] = degrees
    if kind not in PICTURE_KINDS:
        return result
    entity = choice[kind]
    state = states.get(entity) or {}
    attrs = state.get('attributes') or {}
    result['e'] = entity
    name = attrs.get('friendly_name')
    result['n'] = short(name, 60) if isinstance(name, str) and name.strip() else entity.split('.', 1)[1]
    if kind == 'media':
        title = attrs.get('media_title')
        if isinstance(title, str) and title.strip():
            result['t'] = short(title.strip(), 80)
        # The words and the marks the picture is made from; where the track is changes all the time and shows nowhere.
        extras = media_extras(attrs) or {}
        result['x'] = {key: extras[key] for key in ('artist', 'album', 'pic') if key in extras}
        colours = ground(entity, attrs) if ground else None
        if colours:
            result['x']['g'] = colours
    return result


class ScreenSavers:
    """Every screen's choice, by its Home Assistant device, in one small file beside the layouts."""

    def __init__(self, path):
        self.path = path
        self.choices = {}
        try:
            data = json.loads(path.read_text())
            saved = data.get('screens') if isinstance(data, dict) and data.get('version') == 1 else None
        except (OSError, ValueError):
            saved = None
        for device, choice in (saved or {}).items():
            try:
                self.choices[str(device)] = validate(choice)
            except ValueError:
                LOG.warning('A screensaver choice was not readable and starts over')

    def get(self, device):
        choice = self.choices.get(device) or DEFAULT
        return {**DEFAULT, **choice, 'order': list(choice['order']), 'off': list(choice['off'])}

    def set(self, device, value):
        choice = validate(value)
        if choice == DEFAULT:
            self.choices.pop(device, None)
        else:
            self.choices[device] = choice
        self._save()
        return self.get(device)

    def forget(self, device):
        if device and self.choices.pop(device, None) is not None:
            self._save()

    def _save(self):
        temporary = None
        try:
            self.path.parent.mkdir(parents=True, exist_ok=True)
            with tempfile.NamedTemporaryFile(mode='w', dir=self.path.parent, delete=False) as handle:
                temporary = handle.name
                json.dump({'version': 1, 'screens': self.choices}, handle, ensure_ascii=False)
                handle.flush()
                os.fsync(handle.fileno())
            os.replace(temporary, self.path)
        except OSError as error:
            LOG.warning('Could not keep the screensavers (%s)', type(error).__name__)
        finally:
            if temporary and os.path.exists(temporary):
                os.unlink(temporary)
