"""Check that the packages a screen builds from fit together (app 0.2.84+; tools/check.sh runs this).

A screen is packages/core.yaml plus one board file under packages/boards/, included by an entry file: packages/<board>.yaml
for a screen that builds over GitHub, <profile>.yaml in the repository root for a build from a checkout. This checks what
ESPHome would only tell one build at a time:

- every entry names files that exist, and the published entry and the checkout entry of a board include the same two;
- nothing under packages/ carries a secret, a local component path or a fixed Home Assistant subscription: a screen
  gets its values from ESP Screen Manager while it runs, and its keys from its own YAML;
- every ${NAME} the shared core uses is defined, and every board defines the same names: a new board file that
  forgets a size or a hook fails here, not in the first build of a user.
"""
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import profiles  # noqa: E402

ROOT = profiles.ROOT
PLACEHOLDER = re.compile(r'\$\{([A-Z_][A-Z0-9_]*)\}')
# Defined by the entry files (the fonts' place) or by ESP Screen Manager's screen YAML.
FROM_ENTRY = {'FONT_DIR'}


def included(path):
    text = path.read_text()
    block = re.search(r'^packages:\n(.*?)(?=^[a-z_]+:|\Z)', text, re.M | re.S)
    return [str((path.parent / name).resolve().relative_to(ROOT)) for name in re.findall(r'!include (\S+)', block[1])] if block else []


def fail(message):
    raise SystemExit(f'check_packages: {message}')


def main():
    for board in profiles.BOARDS:
        entries = [name for name, b in profiles.ENTRIES.items() if b == board]
        wanted = {'packages/core.yaml', str(profiles.BOARDS[board].relative_to(ROOT))}
        for name in entries:
            files = included(ROOT / name)
            if set(files) != wanted:
                fail(f'{name} includes {files}, expected {sorted(wanted)}')
            for file in files:
                if not (ROOT / file).exists():
                    fail(f'{name} includes a file that does not exist: {file}')
    for path in sorted((ROOT / 'packages').rglob('*.yaml')):
        text = '\n'.join(line for line in path.read_text().split('\n') if not line.lstrip().startswith('#'))
        for needle, why in (('!secret', 'a secret'), ('type: local', 'a local component path')):
            if needle in text:
                fail(f'{path.relative_to(ROOT)} carries {why} ({needle})')
        # A screen gets its values from ESP Screen Manager while it runs: no fixed subscription (time: is the exception).
        for section in ('sensor', 'binary_sensor', 'text_sensor'):
            match = re.search(r'^' + section + r':\n.*?(?=^[a-zA-Z_]+:|\Z)', text, re.M | re.S)
            if match and re.search(r'^  - platform: homeassistant$', match[0], re.M):
                fail(f'{path.relative_to(ROOT)}: a {section} subscribes to Home Assistant; runtime tiles bring their own values')
    # The names the core uses, comment lines left out (its header says "${NAME}" in words).
    core = '\n'.join(line for line in profiles.CORE.read_text().split('\n') if not line.lstrip().startswith('#'))
    core_names = set(profiles.substitutions_of(profiles.CORE))
    used = set(PLACEHOLDER.findall(core))
    board_names = {}
    for board, path in profiles.BOARDS.items():
        values = profiles.substitutions_of(path)
        # The cards of the board's grid come from its cells package, which defines the line that binds them.
        for cells in profiles.cells_of(path):
            values = {**profiles.substitutions_of(cells), **values}
        board_names[board] = set(values)
        # A hook's code may name sizes of its own.
        used_here = used | {name for value in values.values() for name in PLACEHOLDER.findall(value)}
        missing = used_here - core_names - set(values) - FROM_ENTRY
        if missing:
            fail(f'{path.relative_to(ROOT)} does not define {sorted(missing)}, which the shared core uses')
    boards = list(board_names)
    for a in boards:
        for b in boards:
            only = board_names[a] - board_names[b]
            only -= {name for name in only if name.startswith(('TOUCH_AFFINE_', 'TOUCH_CAL_', 'EDGE_SWIPE_', 'ALERT_CARD_H_IMAGE', 'ALERT_IMAGE_', 'ALERT_SUBTITLE_H_IMAGE', 'CAMERA_'))}
            if only:
                fail(f'{profiles.BOARDS[a].name} defines {sorted(only)}, which {profiles.BOARDS[b].name} lacks: a board-only name belongs in that board\'s own sections, a shared one in both')
    unused = core_names - used
    for board, path in profiles.BOARDS.items():
        board_text = path.read_text()
        unused -= set(PLACEHOLDER.findall(board_text))
    print(f'packages: {len(used)} names in the shared core, defined by every board' + (f'; unused in the core: {sorted(unused)}' if unused else ''))


if __name__ == '__main__':
    main()
