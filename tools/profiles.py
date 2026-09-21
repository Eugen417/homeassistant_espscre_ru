"""The board profiles as the tests and tools read them (app 0.2.84+).

A screen is three files: an entry (`home-like-2432s028.yaml` or `packages/cyd.yaml` for a CYD, `guition-4848s040.yaml`
or `packages/guition.yaml` for a Guition), `packages/core.yaml` that every board shares, and the board's own file under
`packages/boards/`. `text(name)` is the three of them one after the other, so a check that looks for a line finds it
wherever it lives; `resolved(name)` also fills in the `${NAME}` substitutions from the three files (the sizes, the fonts
and the hooks with the board's code), for a check that needs the numbers or the whole lambda. Neither is what ESPHome
builds: that is the merge ESPHome's packages component makes (docs/PROFILES.md); the firmware check in tools/check.sh
compiles the real thing.
"""
import re
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parents[1]
CORE = ROOT / 'packages/core.yaml'
BOARDS = {'cyd': ROOT / 'packages/boards/cyd-2432s028.yaml', 'guition': ROOT / 'packages/boards/guition-4848s040.yaml',
          'waveshare43': ROOT / 'packages/boards/waveshare-esp32s3-43.yaml',
          'jc8012p4a1': ROOT / 'packages/boards/guition-jc8012p4a1.yaml'}
ENTRIES = {'home-like-2432s028.yaml': 'cyd', 'guition-4848s040.yaml': 'guition',
           'waveshare-esp32s3-43.yaml': 'waveshare43', 'guition-jc8012p4a1.yaml': 'jc8012p4a1',
           'packages/cyd.yaml': 'cyd', 'packages/guition.yaml': 'guition',
           'packages/waveshare43.yaml': 'waveshare43', 'packages/jc8012p4a1.yaml': 'jc8012p4a1'}
# The names the entry files are known by, in the order the older tests listed them.
PROFILES = ('home-like-2432s028.yaml', 'guition-4848s040.yaml', 'waveshare-esp32s3-43.yaml', 'guition-jc8012p4a1.yaml')
PACKAGES = ('packages/cyd.yaml', 'packages/guition.yaml', 'packages/waveshare43.yaml', 'packages/jc8012p4a1.yaml')
NAMES = PROFILES + PACKAGES


def board_of(name):
    return ENTRIES[str(name).replace(str(ROOT) + '/', '')]


def cells_of(board_file):
    """The cells package a board file brings: the cards of its grid (packages/cells/<number>.yaml)."""
    found = []
    block = re.search(r'^packages:\n(.*?)(?=^[a-z_0-9]+:|\Z)', board_file.read_text(), re.M | re.S)
    if block:
        for include in re.findall(r'!include (\S+)', block[1]):
            found.append((board_file.parent / include).resolve())
    return found


def files(name):
    """The entry, the core, the cards of the board's grid and the board file, in that order."""
    board = BOARDS[board_of(name)]
    return [ROOT / name, CORE, *cells_of(board), board]


def text(name):
    return '\n'.join(path.read_text() for path in files(name))


def substitutions_of(path):
    """The `substitutions:` block of one file, as ESPHome reads it (block scalars included)."""
    block = re.search(r'^substitutions:\n(.*?)(?=^[a-z_0-9]+:|\Z)', path.read_text(), re.M | re.S)
    if not block:
        return {}
    values = yaml.safe_load('substitutions:\n' + block[1])['substitutions'] or {}
    return {key: '' if value is None else str(value) for key, value in values.items()}


def substitutions(name):
    """Every substitution the three files define, the entry's winning over the board's over the core's."""
    values = {}
    for path in reversed(files(name)):
        values.update(substitutions_of(path))
    return values


def resolve(source, values):
    """`${NAME}` filled in, until nothing is left to fill (a hook's code names sizes of its own)."""
    for _ in range(4):
        before = source
        source = re.sub(r'\$\{([A-Z_][A-Z0-9_]*)\}', lambda m: values.get(m[1], m[0]), source)
        if source == before:
            break
    return source


def resolved(name):
    return resolve(text(name), substitutions(name))


def merged(name):
    """Closer to what ESPHome builds: the three files without their substitutions blocks, every ${NAME} filled in.
    For a check that counts things, so a hook's definition in the board file is not counted next to its use."""
    without = [re.sub(r'^substitutions:\n(.*?)(?=^[a-z_0-9]+:|\Z)', '', path.read_text(), count=1, flags=re.M | re.S) for path in files(name)]
    return resolve('\n'.join(without), substitutions(name))
