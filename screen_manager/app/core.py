"""Pure validation, firmware generation and bounded display protocol."""
import base64
import json
import math
import re
import secrets

DOMAINS = frozenset('light switch input_boolean scene script climate vacuum fan cover sensor binary_sensor input_select select number input_number weather media_player button input_button'.split())
REPO = 'https://github.com/MaxGramser/homeassistant_espscreen'
REFS = {'cyd': 'main', 'guition': 'main'}
ATTRS = frozenset('brightness percentage current_position current_temperature temperature current_humidity min_temp max_temp target_temp_step supported_color_modes hvac_modes hs_color color_temp_kelvin min_color_temp_kelvin max_color_temp_kelvin fan_speed_list unit_of_measurement battery_level fan_speed volume_level media_title options min max step temperature_unit supported_features'.split())

# Additive schema 1 extension. An absent object retains old firmware/YAML defaults.
SETTING_RULES = {
    'standby_enabled': (True, None, None),
    'standby_seconds': (600, 60, 86400),
    'brightness': (100, 5, 100),
    'standby_brightness': (20, 0, 100),
    'night_enabled': (True, None, None),
    'night_start': (1320, 0, 1439),
    'night_end': (420, 0, 1439),
    'night_brightness': (10, 0, 100),
    'show_clock': (True, None, None),
    'clock_24h': (True, None, None),
    'home_on_standby': (False, None, None),
    'swipe_pages': (False, None, None),
}

def validate_settings(data):
    if not isinstance(data, dict) or set(data) - SETTING_RULES.keys():
        raise ValueError('Onbekende scherminstellingen; vernieuw de beheerpagina.')
    clean = {}
    for key, (default, minimum, maximum) in SETTING_RULES.items():
        value = data.get(key, default)
        if minimum is None:
            valid = type(value) is bool
        else:
            valid = type(value) is int and minimum <= value <= maximum
        if not valid:
            raise ValueError(f'Ongeldige waarde voor {key}.')
        clean[key] = value
    if max(clean['standby_brightness'], clean['night_brightness']) > clean['brightness']:
        raise ValueError('Helderheid in standby en nacht mag niet hoger zijn dan normaal.')
    return clean

def entity_id(value):
    return isinstance(value, str) and len(value) <= 120 and re.fullmatch(r'[a-z0-9_]+\.[a-z0-9_]+', value) and value.split('.')[0] in DOMAINS

def short(value, limit):
    return str(value).encode('utf-8')[:limit].decode('utf-8', errors='ignore')

def validate_layout(data):
    if not isinstance(data, dict):
        raise ValueError('Ongeldige indeling.')
    title, tiles = data.get('title'), data.get('tiles')
    if not isinstance(title, str) or not title.strip() or len(title.encode()) > 96:
        raise ValueError('Geef het scherm een titel van maximaal 96 bytes.')
    if not isinstance(tiles, list) or len(tiles) > 20:
        raise ValueError('Kies maximaal 20 tegels.')
    clean, seen = [], set()
    for tile in tiles:
        if not isinstance(tile, dict) or not entity_id(tile.get('entity')):
            raise ValueError('Deze entiteit wordt niet ondersteund.')
        if tile['entity'] in seen:
            raise ValueError('Een entiteit kan maar één keer op een scherm staan.')
        name = tile.get('name', '')
        if not isinstance(name, str) or len(name.encode()) > 80:
            raise ValueError('Een tegelnaam mag maximaal 80 bytes bevatten.')
        seen.add(tile['entity'])
        item = {'entity': tile['entity'], 'name': name.strip()}
        if 'options' in tile:
            options = tile['options']
            if not isinstance(options, dict) or set(options) - {'tap', 'display', 'inline', 'history_hours'}:
                raise ValueError('Onbekende tegelinstellingen.')
            choices = {'tap': ('auto', 'detail', 'toggle', 'none'), 'display': ('standard', 'watch'), 'inline': ('none', 'slider')}
            domain = tile['entity'].split('.')[0]
            for key, allowed in choices.items():
                if key in options and options[key] not in allowed:
                    raise ValueError('Ongeldige tegelinstelling: ' + key)
            if options.get('tap') == 'toggle' and domain not in {'light','switch','input_boolean','fan','media_player'}:
                raise ValueError('Deze entiteit ondersteunt geen aan/uit-actie.')
            if options.get('inline') == 'slider' and domain not in {'light','fan','cover','number','input_number','media_player'}:
                raise ValueError('Deze entiteit ondersteunt geen mini-schuif.')
            if 'history_hours' in options and (type(options['history_hours']) is not int or options['history_hours'] not in (1,6,24)):
                raise ValueError('Geschiedenis: kies 1, 6 of 24 uur.')
            if options.get('display') == 'watch' and options.get('inline') == 'slider':
                raise ValueError('Kies grote waarde of mini-schuif.')
            item['options'] = dict(options)
        clean.append(item)
    result = {'title': title.strip(), 'tiles': clean}
    if 'settings' in data:
        result['settings'] = validate_settings(data['settings'])
    return result

def state_message(index, tile, states):
    state = states.get(tile['entity'], {})
    attrs = state.get('attributes', {})
    bounded = {}
    for key in ATTRS:
        value = attrs.get(key)
        if isinstance(value, bool) or value is None:
            continue
        if isinstance(value, (int, float)):
            if math.isfinite(value) and abs(value) <= 1000000:
                bounded[key] = value
        elif isinstance(value, str):
            bounded[key] = short(value, 48)
        elif isinstance(value, list):
            # Attribute lists have bounded lengths, strings and numeric ranges.
            limit = 2 if key == 'hs_color' else 4 if key == 'fan_speed_list' else 8
            bounded[key] = [short(v, 48) if isinstance(v, str) else v for v in value[:limit]
                            if isinstance(v, str) or isinstance(v, (float, int)) and math.isfinite(v) and abs(v) <= 1000000]
    return {'v': 1, 'op': 'state', 'i': index, 'entity': tile['entity'],
            'name': short(tile['name'] or attrs.get('friendly_name') or tile['entity'], 80),
            'state': short(state.get('state', 'unavailable'), 160), 'a': bounded,
            **({'o': tile['options']} if 'options' in tile else {})}

def packets(message, token=None):
    raw = json.dumps(message, ensure_ascii=False, separators=(',', ':'), allow_nan=False).encode()
    if len(raw) > 4096:
        raise ValueError('Het schermbericht is te groot.')
    encoded = base64.b64encode(raw).decode('ascii')
    token = token or secrets.token_hex(8)
    chunks = [encoded[i:i+200] for i in range(0, len(encoded), 200)]
    return [f'{token}|{i}|{int(i == len(chunks)-1)}|{part}' for i, part in enumerate(chunks)]

def discover(registry, states, devices, areas):
    device_map = {d['id']: d for d in devices}
    area_map = {a['area_id']: a['name'] for a in areas}
    versions = {item.get('device_id'): states.get(item['entity_id'], {}).get('state', 'onbekend') for item in registry
                if item.get('platform') == 'esphome' and item.get('original_name') == 'Schermfirmware'}
    screens, entities = [], []
    for item in registry:
        eid = item['entity_id']
        device = device_map.get(item.get('device_id'), {})
        state = states.get(eid, {})
        area = area_map.get(item.get('area_id') or device.get('area_id'), '')
        if item.get('platform') == 'esphome' and eid.startswith('text.') and item.get('original_name') == 'Tegelinstellingen' and not item.get('disabled_by'):
            screens.append({'id': eid, 'name': device.get('name_by_user') or device.get('name') or eid,
                            'firmware': versions.get(item.get('device_id'), 'onbekend'),
                            'area': area, 'online': state.get('state') not in (None, 'unknown', 'unavailable'),
                            'status': state.get('state', 'Niet verbonden')})
        if not entity_id(eid) or item.get('disabled_by'):
            continue
        entities.append({'id': eid, 'name': state.get('attributes', {}).get('friendly_name') or item.get('name') or item.get('original_name') or eid,
                         'device': device.get('name_by_user') or device.get('name') or '', 'area': area,
                         'state': state.get('state', 'unavailable')})
    # YAML entities may not have an entity-registry entry.
    registered = {e['id'] for e in entities}
    for eid, state in states.items():
        if entity_id(eid) and eid not in registered and not any(r['entity_id'] == eid for r in registry):
            entities.append({'id': eid, 'name': state.get('attributes', {}).get('friendly_name', eid), 'device': '', 'area': '', 'state': state['state']})
    return screens, sorted(entities, key=lambda e: e['name'].casefold())

def installation_yaml(data):
    board, name, friendly = data.get('board'), data.get('name'), data.get('friendly_name')
    if board not in REFS or not isinstance(name, str) or not re.fullmatch(r'[a-z][a-z0-9-]{0,29}', name):
        raise ValueError('Kies een bord en een unieke naam (kleine letters, cijfers, streepjes; maximaal 30 tekens).')
    if not isinstance(friendly, str) or not friendly.strip() or len(friendly) > 60:
        raise ValueError('Geef het scherm een herkenbare naam (maximaal 60 tekens).')
    quote = lambda s: json.dumps(s, ensure_ascii=False)
    key, ota, ap = base64.b64encode(secrets.token_bytes(32)).decode(), secrets.token_urlsafe(24), secrets.token_urlsafe(12)
    return f'''# Bewaar dit bestand: het bevat de unieke sleutels voor dit scherm.
# Wifi komt uit de secrets.yaml van ESPHome Device Builder.
substitutions:
  DEVICE_NAME: {quote(name)}
  DEVICE_FRIENDLY_NAME: {quote(friendly.strip())}

esphome:
  name: {quote(name)}
  friendly_name: {quote(friendly.strip())}

packages:
  display:
    url: {REPO}
    ref: {REFS[board]}
    files: [packages/{board}.yaml]
    refresh: 0s

api:
  encryption:
    key: {quote(key)}
ota:
  - platform: esphome
    password: {quote(ota)}
wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
  ap:
    ssid: {quote(name + ' Setup')}
    password: {quote(ap)}
captive_portal:
'''
