"""The languages of ESP Screens (app 0.2.90).

Every text of the app, the editor and the screens lives in one JSON file per language (screen_manager/translations,
docs/TRANSLATING.md); English is the source, and a text a language lacks shows in English. The editor reads its own
section in the browser, the screens get theirs built into their firmware (components/smart_display/screen_text_gen.py),
and this module gives the app its `addon` texts and the `screen` words it sends to the screens itself.

`Region` is Settings -> Language & region, the one place for every screen: the language (Home Assistant's, or a chosen
one), the clock (12 or 24 hours) and how numbers are written. "auto" does what Home Assistant does for a user who never
changed their profile: the clock and the numbers follow the language.
"""
import contextvars
import json
import logging
import os
import re
from pathlib import Path

LOG = logging.getLogger('screen_manager')
HERE = Path(__file__).resolve().parent
# In the app's image the files sit next to the code (Dockerfile: COPY translations); in a checkout one folder up.
FOLDER = next((path for path in (HERE / 'translations', HERE.parent / 'translations') if path.is_dir()), HERE / 'translations')
CODE = re.compile(r'[a-z]{2,3}(-[A-Za-z0-9]{2,8})?')
CLOCKS = ('auto', '24', '12')
NUMBERS = ('auto', 'point', 'comma', 'space')
PLACEHOLDER = re.compile(r'\{([a-z_]+)\}')

# Which form of a plural text ("1 hour ago | {n} hours ago") fits n; the firmware (screen_text_gen.py) and the editor
# (web/src/i18n.ts) have the same rules, docs/TRANSLATING.md says which languages use which.
def _slavic(n, one):
    t, h = n % 10, n % 100
    if one:
        return 0
    return 1 if 2 <= t <= 4 and not 12 <= h <= 14 else 2

PLURAL = {
    'one_other': lambda n: 0 if n == 1 else 1,
    'one_upto_1': lambda n: 0 if abs(n) <= 1 else 1,
    'slavic_pl': lambda n: _slavic(n, n == 1),
    'east_slavic': lambda n: _slavic(n, n % 10 == 1 and n % 100 != 11),
    'none': lambda n: 0,
}

# The language of an editor's request (its X-ESP-Screens-Language header), for the messages the app answers with.
REQUEST_LANGUAGE = contextvars.ContextVar('request_language', default='en')


def flatten(node, prefix=''):
    """(key, text) pairs in document order; a list's items get .0, .1, ... ."""
    if isinstance(node, dict):
        items = node.items()
    elif isinstance(node, list):
        items = ((str(index), value) for index, value in enumerate(node))
    else:
        return [(prefix, node)]
    pairs = []
    for key, value in items:
        pairs.extend(flatten(value, f'{prefix}.{key}' if prefix else key))
    return pairs


class Translations:
    """Every language file, read once."""

    def __init__(self, folder=FOLDER):
        self.data, self.flat = {}, {}
        for path in sorted(Path(folder).glob('*.json')):
            try:
                data = json.loads(path.read_text(encoding='utf-8'))
            except (OSError, ValueError) as error:
                LOG.warning('Translation %s is unreadable (%s); it is left out', path.name, type(error).__name__)
                continue
            if CODE.fullmatch(path.stem) and isinstance(data, dict):
                self.data[path.stem] = data
                self.flat[path.stem] = {key: value for key, value in flatten({k: v for k, v in data.items() if k != '_meta'})
                                        if isinstance(value, str)}
        self.data.setdefault('en', {'_meta': {'name': 'English', 'english': 'English', 'plural': 'one_other'}})
        self.flat.setdefault('en', {})

    def resolve(self, code):
        """The language ESP Screens has for `code`: the code itself (in any case), else its base (pt-BR -> pt), else English."""
        if not isinstance(code, str):
            return 'en'
        by_lower = {known.lower(): known for known in self.data}
        for candidate in (code, code.split('-')[0]):
            if candidate.lower() in by_lower:
                return by_lower[candidate.lower()]
        return 'en'

    def meta(self, code):
        return self.data.get(self.resolve(code), {}).get('_meta', {})

    def languages(self):
        """Every language for the settings: English first, then by English name."""
        found = [{'code': code, 'name': str(self.meta(code).get('name') or code),
                  'english': str(self.meta(code).get('english') or code), 'checked': self.meta(code).get('checked') is True}
                 for code in self.data]
        return sorted(found, key=lambda item: (not item['code'].startswith('en'), item['code'] != 'en', item['english']))

    def text(self, key, language='en', **params):
        """`key` in `language`, English where it has none, with {placeholders} filled; `n` picks a plural form."""
        language = self.resolve(language)
        text = self.flat.get(language, {}).get(key) or self.flat['en'].get(key)
        if not isinstance(text, str) or not text.strip():
            return key
        if '|' in text and 'n' in params:
            forms = [form.strip() for form in text.split('|')]
            rule = PLURAL.get(self.meta(language).get('plural'), PLURAL['one_other'])
            try:
                text = forms[min(rule(int(params['n'])), len(forms) - 1)]
            except (TypeError, ValueError):
                text = forms[-1]
        return PLACEHOLDER.sub(lambda match: str(params[match.group(1)]) if match.group(1) in params else match.group(0), text)

    def clock_24h(self, code):
        """Whether the language writes 24 hours by default (_meta.clock), as Home Assistant's "auto" does."""
        return str(self.meta(code).get('clock', '24')) != '12'

    def number_style(self, code):
        """How the language writes 1234.5: point (1,234.5), comma (1.234,5) or space (1 234,5)."""
        decimal = self.text('screen.number.decimal', code)
        group = self.text('screen.number.group', code)
        if decimal == '.':
            return 'point'
        return 'space' if group.strip() == '' else 'comma'


TRANSLATIONS = Translations()


def t(key, **params):
    """An `addon` text in the language of the editor that asked (REQUEST_LANGUAGE)."""
    return TRANSLATIONS.text(key, REQUEST_LANGUAGE.get(), **params)


# The screens' language and number format (Settings -> Language & region), for the words the app sends to the screens
# itself: tile names, the top bar, the history card. The manager keeps it current (Manager.language_changed).
SCREENS = {'language': 'en', 'numbers': 'point'}


def set_screens(language, numbers):
    SCREENS.update(language=language, numbers=numbers)


def screen_t(key, **params):
    """A text in the screens' language, for what the app sends to them."""
    return TRANSLATIONS.text(key, SCREENS['language'], **params)


def screen_number(text):
    """A number as Home Assistant sends it, written as the screens write numbers ("1.234,5" in Dutch)."""
    return format_number(text, SCREENS['numbers'])


class Region:
    """Settings -> Language & region, kept in /data/language.json: one language, clock and number format for every
    screen. A screen's firmware carries its language; the clock and the numbers travel in the layout message."""

    def __init__(self, path, translations=TRANSLATIONS, ha_language=None):
        self.path, self.tr = Path(path), translations
        self.setting, self.clock, self.numbers = 'auto', 'auto', 'auto'
        # Home Assistant's own language (get_config), once connected.
        self.ha_language = ha_language or (lambda: None)
        self.stored = self.path.exists()
        if self.stored:
            try:
                raw = json.loads(self.path.read_text(encoding='utf-8'))
            except (OSError, ValueError):
                raw = {}
            if raw.get('version') == 1:
                self.setting = raw.get('language') if raw.get('language') == 'auto' or raw.get('language') in self.tr.data else 'auto'
                self.clock = raw.get('clock') if raw.get('clock') in CLOCKS else 'auto'
                self.numbers = raw.get('numbers') if raw.get('numbers') in NUMBERS else 'auto'

    def save(self):
        self.path.parent.mkdir(parents=True, exist_ok=True)
        temporary = self.path.with_suffix('.tmp')
        with open(temporary, 'w', encoding='utf-8') as handle:
            os.chmod(temporary, 0o600)
            json.dump({'version': 1, 'language': self.setting, 'clock': self.clock, 'numbers': self.numbers}, handle)
            handle.flush()
            os.fsync(handle.fileno())
        temporary.replace(self.path)
        self.stored = True

    def change(self, data):
        """Apply {language?, clock?, numbers?} from the editor; ValueError (translated) for anything unknown."""
        if not isinstance(data, dict):
            raise ValueError(t('addon.errors.language_choice'))
        setting = data.get('setting', self.setting)
        clock = data.get('clock', self.clock)
        numbers = data.get('numbers', self.numbers)
        if setting != 'auto' and setting not in self.tr.data:
            raise ValueError(t('addon.errors.language_choice'))
        if clock not in CLOCKS or numbers not in NUMBERS:
            raise ValueError(t('addon.errors.region_choice'))
        self.setting, self.clock, self.numbers = setting, clock, numbers
        self.save()

    def language(self):
        """The language of every screen: the chosen one, else Home Assistant's, else English."""
        return self.setting if self.setting != 'auto' else self.tr.resolve(self.ha_language() or 'en')

    def clock_24h(self):
        return self.tr.clock_24h(self.language()) if self.clock == 'auto' else self.clock == '24'

    def number_style(self):
        return self.tr.number_style(self.language()) if self.numbers == 'auto' else self.numbers

    def view(self):
        """What the editor's Language & region card shows."""
        return {'setting': self.setting, 'effective': self.language(),
                'ha': self.ha_language(), 'languages': self.tr.languages(),
                'clock': self.clock, 'clock_effective': '24' if self.clock_24h() else '12',
                'numbers': self.numbers, 'numbers_effective': self.number_style()}

    def screen_text(self, key, **params):
        """A `screen` or `addon` text in the screens' language, for what the app itself sends to them."""
        return self.tr.text(key, self.language(), **params)


def format_number(text, style):
    """A number as Home Assistant sends it ("1234.5") written in `style` ("1.234,5" for comma); anything else unchanged."""
    if not isinstance(text, str) or not re.fullmatch(r'-?\d+(\.\d+)?', text):
        return text
    sign, body = ('-', text[1:]) if text.startswith('-') else ('', text)
    whole, _, rest = body.partition('.')
    group, decimal = {'point': (',', '.'), 'comma': ('.', ','), 'space': (' ', ',')}.get(style, (',', '.'))
    if len(whole) > 3:
        head = len(whole) % 3 or 3
        whole = whole[:head] + ''.join(group + whole[index:index + 3] for index in range(head, len(whole), 3))
    return sign + whole + (decimal + rest if rest else '')
