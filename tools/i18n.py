#!/usr/bin/env python3
"""The translations in screen_manager/translations/ (app 0.2.90); docs/TRANSLATING.md says how they work.

    python3 tools/i18n.py check            every language against English: keys, placeholders, plural forms, letters
                                            the screens can't draw, screen texts much longer than the English
    python3 tools/i18n.py lint             English words left in the firmware's code instead of the translations
    python3 tools/i18n.py header [--check]  write (or compare) the key header and the tests' English table that the
                                            firmware's `screen` texts need, after changing the keys of en.json
    python3 tools/i18n.py cldr [--write]    day and month names, date orders and the decimal mark from the Unicode CLDR
                                            (Node's Intl), into every language's screen.date and screen.number
    python3 tools/i18n.py ha-words [--write] Home Assistant's own words for states (screen.ha) in every language, from a
                                            Home Assistant (HA_URL and HA_TOKEN); needs aiohttp
    python3 tools/i18n.py new CODE NAME ENGLISH PLURAL
                                            a new language file, such as `new sv Svenska Swedish one_other`
"""
import argparse
import importlib.util
import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FOLDER = ROOT / 'screen_manager' / 'translations'
KEYS_HEADER = ROOT / 'components' / 'smart_display' / 'screen_text_keys.h'
TESTS_HEADER = ROOT / 'tests' / 'screen_text_en.h'
CORE = ROOT / 'packages' / 'core.yaml'


def generator():
    """components/smart_display/screen_text_gen.py, which the firmware build itself uses."""
    spec = importlib.util.spec_from_file_location('screen_text_gen', ROOT / 'components' / 'smart_display' / 'screen_text_gen.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def languages():
    """{code: data} of every language file, English first."""
    found = {path.stem: json.loads(path.read_text(encoding='utf-8')) for path in sorted(FOLDER.glob('*.json'))}
    return dict(sorted(found.items(), key=lambda item: (item[0] != 'en', item[0])))


def header(check):
    gen = generator()
    wanted = {KEYS_HEADER: gen.keys_header([key for key, _ in gen.screen_pairs(gen.load('en', FOLDER))]),
              TESTS_HEADER: gen.english_header(FOLDER)}
    stale = [path for path, text in wanted.items() if not path.exists() or path.read_text(encoding='utf-8') != text]
    if check:
        for path in stale:
            print(f'{path.relative_to(ROOT)} is out of date: run python3 tools/i18n.py header')
        return 1 if stale else 0
    for path in stale:
        path.write_text(wanted[path], encoding='utf-8')
        print(f'wrote {path.relative_to(ROOT)}')
    return 0


PLACEHOLDER = re.compile(r'\{([a-z_]+)\}')
FORMS = {'one_other': 2, 'one_upto_1': 2, 'slavic_pl': 3, 'east_slavic': 3, 'none': 1}


def font_letters():
    """The characters every text font of the screens carries: what a `screen` text may use."""
    text = CORE.read_text(encoding='utf-8')
    sets = []
    for block in re.finditer(r'(?ms)^  - file: "\$\{FONT_DIR\}/Roboto-\d+\.ttf"\n(.*?)(?=^  - |^\S|\Z)', text):
        body = block.group(1)
        if not re.search(r'(?m)^\s+id: (headline|label|sublabel|sublabel_big|watch_value)\s*$', body):
            continue
        glyphs = re.search(r'(?s)glyphs:\s*\[(.*?)\]', body)
        if glyphs:
            letters = set()
            for item in re.findall(r"'((?:[^'\\]|\\.)*)'|\"((?:[^\"\\]|\\.)*)\"", glyphs.group(1)):
                letters.update(item[0] or item[1])
            sets.append(letters)
    return set.intersection(*sets) if sets else set()


def check():
    gen = generator()
    langs = languages()
    english = langs['en']
    problems, notes = [], []
    base = dict(gen.flatten({k: v for k, v in english.items() if k != '_meta'}))
    drawable = font_letters()
    for code, data in langs.items():
        meta = data.get('_meta', {})
        for field in ('name', 'english', 'script', 'plural', 'checked'):
            if field not in meta:
                problems.append(f'{code}: _meta.{field} is missing')
        forms = FORMS.get(meta.get('plural'))
        if forms is None:
            problems.append(f'{code}: _meta.plural must be one of {", ".join(FORMS)}')
        own = dict(gen.flatten({k: v for k, v in data.items() if k != '_meta'}))
        for key in own.keys() - base.keys():
            problems.append(f'{code}: {key} is not in en.json')
        missing = [key for key in base if key not in own]
        if code != 'en' and missing:
            notes.append(f'{code}: {len(missing)} of {len(base)} texts still in English')
        for key, text in own.items():
            if key not in base or not isinstance(text, str):
                continue
            source = base[key]
            if set(PLACEHOLDER.findall(text)) != set(PLACEHOLDER.findall(source)):
                problems.append(f'{code}: {key} has placeholders {sorted(set(PLACEHOLDER.findall(text)))}, '
                                f'English {sorted(set(PLACEHOLDER.findall(source)))}')
            plural = ' | ' in source
            if plural and forms and len(text.split('|')) not in (1, forms):
                problems.append(f'{code}: {key} needs {forms} forms separated by " | "')
            if key.startswith('screen.') and drawable:
                shown = PLACEHOLDER.sub('', text).replace('|', '')
                unknown = sorted(set(shown) - drawable - {'\n'})
                if unknown:
                    problems.append(f'{code}: {key} uses letters the screens can\'t draw: {"".join(unknown)}')
                # Home Assistant's words and the calendar's are what they are; our own texts should stay about as short.
                ours = not key.startswith(('screen.ha.', 'screen.date.', 'screen.number.'))
                if ours and code != 'en' and len(text) > max(8, len(source) * 1.6) and not plural:
                    notes.append(f'{code}: {key} is much longer than the English ({len(text)} vs {len(source)})')
    for line in notes:
        print('note:', line)
    for line in problems:
        print('problem:', line)
    print(f'{len(langs)} languages, {len(base)} texts: {len(problems)} problems')
    return 1 if problems else 0


# English that stays in the firmware on purpose, with why. Everything else a screen shows comes from the translations,
# and `lint` fails on a new English text in the code, so it never slips back in.
LINT_KEEP = {
    # Statuses the app reads and acts on (screen_manager/app/server.py RESEND_STATES, the "Error" prefix): protocol.
    'Synced', 'Loading tiles', 'Layout received', 'Resend needed', 'Ready for tile configuration',
    'Use the Easy Setup profile', 'Swipe test started', 'no answer', 'Error: message too large',
    'Error: invalid message', 'Error: protocol version', 'Error: screen settings', 'Error: outdated tile',
    'Error: no memory for ', 'Error: incomplete message', 'Error: invalid encoding',
    # Only a log line or the rate limiter's reason shows these.
    'history range', 'card button ', 'media key ', 'let go', 'too short (', 'already handled in this contact',
    'same button within the debounce window', 'no runtime tiles', 'setting off', 'screen dimmed', 'card open',
    'detail card open', 'camera open', 'USB calibration ready; no tile actions', 'GT911 touch test ready; no tile actions',
    'UI_TEST START: page/overlay render stress, no HA actions', 'Color', 'Color temperature', 'Brightness',
    # Home Assistant's own values and units the code compares with, not words it shows.
    'None', 'Auto', 'Wh',
    # Placeholders of the YAML tree that the runtime tiles replace before a screen shows them, and profile defaults.
    'Lamp', 'Plug', 'Evening', 'All off', 'AC', 'Vacuum', 'Tile 7', 'Tile 8', 'Tile 9', 'Tile 10', 'Light', 'Light Color',
    'Climate', 'Example lamp', 'My CYD', 'My Guition',
}
LINT_FILES = ('components/smart_display/*.h', 'packages/core.yaml', 'packages/boards/*.yaml')


def lint():
    """English words in the firmware's code that should be a key of the translations' `screen` section."""
    literal = re.compile(r'"((?:[^"\\]|\\.)*)"')
    found = []
    for pattern in LINT_FILES:
        for path in sorted(ROOT.glob(pattern)):
            for number, line in enumerate(path.read_text(encoding='utf-8').splitlines(), 1):
                stripped = line.strip()
                # Comments, logs, and Home Assistant's entity names (renaming one gives it a new entity id).
                if stripped.startswith(('//', '#', '*')) or 'ESP_LOG' in line or 'static_assert' in line or re.match(r'\s*name: "', line):
                    continue
                code = line.split('//')[0]
                if path.suffix == '.yaml':
                    code = re.split(r'\s#', code)[0]
                for match in literal.finditer(code):
                    text = match.group(1)
                    if text in LINT_KEEP or re.search(r'[();]|::|->|\$\{', text):
                        continue
                    if re.match(r'^[A-Z][a-z]', text) or re.search(r'[A-Za-z]{2,} [a-z]{2,}', text):
                        found.append(f'{path.relative_to(ROOT)}:{number}: "{text}"')
    for line in found:
        print('English in the firmware (a key of screen_manager/translations/en.json, or tools/i18n.py LINT_KEEP):', line)
    return 1 if found else 0


# Where screen.ha's words come from: Home Assistant's own translations (frontend/get_translations, `entity_component`),
# so a screen says what Home Assistant says in the same language.
def ha_sources():
    sources = {'on': 'component.switch.entity_component._.state.on', 'off': 'component.switch.entity_component._.state.off'}
    for mode in ('off', 'heat', 'cool', 'heat_cool', 'auto', 'dry', 'fan_only'):
        sources[f'climate.{mode}'] = f'component.climate.entity_component._.state.{mode}'
    for action in ('heating', 'cooling', 'idle', 'off', 'drying', 'fan', 'preheating', 'defrosting'):
        sources[f'hvac_action.{action}'] = f'component.climate.entity_component._.state_attributes.hvac_action.state.{action}'
    for state in ('open', 'closed', 'opening', 'closing'):
        sources[f'cover.{state}'] = f'component.cover.entity_component._.state.{state}'
    for state in ('playing', 'paused', 'idle', 'standby'):
        sources[f'media.{state}'] = f'component.media_player.entity_component._.state.{state}'
    for state in ('home', 'not_home'):
        sources[f'person.{state}'] = f'component.person.entity_component._.state.{state}'
    for state in ('above_horizon', 'below_horizon'):
        sources[f'sun.{state}'] = f'component.sun.entity_component._.state.{state}'
    for state in ('docked', 'cleaning', 'paused', 'returning', 'idle'):
        sources[f'vacuum.{state}'] = f'component.vacuum.entity_component._.state.{state}'
    english = json.loads((FOLDER / 'en.json').read_text(encoding='utf-8'))['screen']['ha']
    for key in english['weather']:
        sources[f'weather.{key}'] = f'component.weather.entity_component._.state.{key.replace("_", "-") if key != "partlycloudy" else key}'
    for key in english['binary']:
        device_class, state = key.rsplit('_', 1)
        sources[f'binary.{key}'] = f'component.binary_sensor.entity_component.{device_class}.state.{state}'
    return sources


def ha_words(write):
    """screen.ha of every language from a Home Assistant (HA_URL and HA_TOKEN, or .esphome/ha_url and ha_token): its own
    words for states, English included, so a screen says what Home Assistant says (Max, app 0.2.90)."""
    import asyncio
    import os
    try:
        import aiohttp
    except ImportError:
        raise SystemExit('ha-words needs aiohttp (.venv-portal/bin/python has it)')
    def setting(name, file):
        value = os.environ.get(name)
        for folder in (ROOT, ROOT.parent / 'esphome-cyd-display'):
            if not value and (folder / '.esphome' / file).exists():
                value = (folder / '.esphome' / file).read_text().strip()
        if not value:
            raise SystemExit(f'Set {name} or .esphome/{file}')
        return value
    url, token = setting('HA_URL', 'ha_url').rstrip('/'), setting('HA_TOKEN', 'ha_token')
    sources = ha_sources()

    async def fetch(codes):
        found = {}
        async with aiohttp.ClientSession() as session:
            async with session.ws_connect(url.replace('http', 'ws', 1) + '/api/websocket') as ws:
                await ws.receive_json()
                await ws.send_json({'type': 'auth', 'access_token': token})
                if (await ws.receive_json()).get('type') != 'auth_ok':
                    raise SystemExit('Home Assistant refused the token')
                for number, code in enumerate(codes, 1):
                    await ws.send_json({'id': number, 'type': 'frontend/get_translations', 'language': code, 'category': 'entity_component'})
                    answer = await ws.receive_json()
                    found[code] = (answer.get('result') or {}).get('resources') or {}
        return found

    langs = languages()
    found = asyncio.run(fetch(list(langs)))
    status = 0
    for code, data in langs.items():
        words = found.get(code) or {}
        own = {}
        for key, source in sources.items():
            if isinstance(words.get(source), str) and words[source]:
                own[key] = words[source]
            else:
                print(f'{code}: Home Assistant has no {source}')
        reference = dict(generator().flatten(data.get('screen', {}).get('ha', {})))
        if code == 'en':
            for key, word in own.items():
                if reference.get(key) != word:
                    print(f'en: screen.ha.{key} "{reference.get(key)}" becomes Home Assistant\'s "{word}"')
        tree = {}
        for key, word in own.items():
            node = tree
            parts = key.split('.')
            for part in parts[:-1]:
                node = node.setdefault(part, {})
            node[parts[-1]] = word
        before = json.dumps(data, ensure_ascii=False)
        merge(data.setdefault('screen', {}).setdefault('ha', {}), tree)
        if json.dumps(data, ensure_ascii=False) != before:
            print(f'{code}: {"wrote" if write else "would change"} {len(own)} words from Home Assistant')
            if write:
                write_language(code, data)
    return status


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest='command', required=True)
    sub.add_parser('check')
    sub.add_parser('lint')
    head = sub.add_parser('header')
    head.add_argument('--check', action='store_true')
    cldr = sub.add_parser('cldr')
    cldr.add_argument('--write', action='store_true')
    words = sub.add_parser('ha-words')
    words.add_argument('--write', action='store_true')
    fresh = sub.add_parser('new')
    fresh.add_argument('code')
    fresh.add_argument('name')
    fresh.add_argument('english')
    fresh.add_argument('plural')
    args = parser.parse_args()
    if args.command == 'new':
        return new_language(args.code, args.name, args.english, args.plural)
    if args.command == 'check':
        return check()
    if args.command == 'lint':
        return lint()
    if args.command == 'header':
        return header(args.check)
    if args.command == 'cldr':
        return cldr_dates(args.write)
    if args.command == 'ha-words':
        return ha_words(args.write)
    return 2


# The CLDR locale of a file where it differs: Portuguese is Home Assistant's pt, the one of Portugal.
CLDR_LOCALE = {'pt': 'pt-PT'}


def write_language(code, data):
    (FOLDER / f'{code}.json').write_text(json.dumps(data, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')


def merge(target, source):
    """`source` into `target` in place, keeping the order of what `target` already has."""
    for key, value in source.items():
        if isinstance(value, dict) and isinstance(target.get(key), dict):
            merge(target[key], value)
        else:
            target[key] = value


def cldr_dates(write, codes=None):
    """Day and month names, date orders, AM/PM, the number marks and the clock of every language but English (whose
    texts are the source) from Node's Intl, into screen.date, screen.time, screen.number and _meta.clock."""
    codes = codes or [code for code in languages() if not code.startswith('en')]
    if not codes:
        return 0
    locales = [CLDR_LOCALE.get(code, code) for code in codes]
    found = json.loads(subprocess.run(['node', str(ROOT / 'tools' / 'i18n_cldr.mjs'), *locales], check=True,
                                      capture_output=True, text=True).stdout)
    for code, locale in zip(codes, locales):
        path = FOLDER / f'{code}.json'
        data = json.loads(path.read_text(encoding='utf-8'))
        cldr = found[locale]
        before = json.dumps(data, ensure_ascii=False)
        data.setdefault('_meta', {})['clock'] = cldr['clock']
        merge(data.setdefault('screen', {}), {'number': cldr['number'], 'time': cldr['time'], 'date': cldr['date']})
        if json.dumps(data, ensure_ascii=False) != before:
            print(f'{code}: {"wrote" if write else "would change"} the CLDR texts')
            if write:
                write_language(code, data)
    return 0


def new_language(code, name, english, plural):
    """A new language file: its _meta and the CLDR texts, every other text still English until someone translates it."""
    if not re.fullmatch(r'[a-z]{2,3}(-[A-Za-z0-9]{2,8})?', code) or (FOLDER / f'{code}.json').exists():
        raise SystemExit(f'{code}: not a new language code')
    if plural not in FORMS:
        raise SystemExit(f'plural must be one of {", ".join(FORMS)}')
    write_language(code, {'_meta': {'name': name, 'english': english, 'script': 'latin', 'plural': plural, 'clock': '24',
                                    'checked': False}, 'screen': {}, 'addon': {}, 'editor': {}})
    return cldr_dates(True, [code])


if __name__ == '__main__':
    sys.exit(main())
