#!/usr/bin/env python3
"""The translations in screen_manager/translations/ (app 0.2.90); docs/TRANSLATING.md says how they work.

    python3 tools/i18n.py check            every language against English: keys, placeholders, plural forms, letters
                                            the screens can't draw, screen texts much longer than the English
    python3 tools/i18n.py header [--check]  write (or compare) the key header and the tests' English table that the
                                            firmware's `screen` texts need, after changing the keys of en.json
    python3 tools/i18n.py cldr [--write]    day and month names, date orders and the decimal mark from the Unicode CLDR
                                            (Node's Intl), into every language's screen.date and screen.number
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
                unknown = sorted(set(text) - drawable - {'\n'})
                if unknown:
                    problems.append(f'{code}: {key} uses letters the screens can\'t draw: {"".join(unknown)}')
                if code != 'en' and len(text) > max(8, len(source) * 1.6) and not plural:
                    notes.append(f'{code}: {key} is much longer than the English ({len(text)} vs {len(source)})')
    for line in notes:
        print('note:', line)
    for line in problems:
        print('problem:', line)
    print(f'{len(langs)} languages, {len(base)} texts: {len(problems)} problems')
    return 1 if problems else 0


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest='command', required=True)
    sub.add_parser('check')
    head = sub.add_parser('header')
    head.add_argument('--check', action='store_true')
    cldr = sub.add_parser('cldr')
    cldr.add_argument('--write', action='store_true')
    args = parser.parse_args()
    if args.command == 'check':
        return check()
    if args.command == 'header':
        return header(args.check)
    if args.command == 'cldr':
        return cldr_dates(args.write)
    return 2


def cldr_dates(write):
    raise SystemExit('not yet')


if __name__ == '__main__':
    sys.exit(main())
