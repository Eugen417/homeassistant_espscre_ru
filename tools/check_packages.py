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
    block = re.search(r'^packages:\n(.*?)(?=^[a-z_0-9]+:|\Z)', text, re.M | re.S)
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
    # Every board file, not only the three the manager ships: a new panel starts life as packages/boards/lab-*.yaml
    # (tools/new_board.py) and is flashed and rendered long before it is registered, so the two things that were
    # silently wrong on a real screen this month are checked for all of them.
    seen_ids = {}
    for path in sorted((ROOT / 'packages' / 'boards').glob('*.yaml')):
        values = profiles.substitutions_of(path)
        # As many cards as the page has cells. A board that raised its rows but kept the cells file of the old
        # grid drew nothing in the cells it gained: the tiles were sent, the screen knew their slots, and the
        # cards to put them in did not exist (Waveshare 3 x 2 -> 3 x 3, 2026-09-20).
        if 'GRID_COLS' in values and 'GRID_ROWS' in values:
            need = int(values['GRID_COLS']) * int(values['GRID_ROWS'])
            for cells in profiles.cells_of(path):
                if cells.parent.name == 'cells' and cells.stem.isdigit() and int(cells.stem) != need:
                    fail(f'{path.relative_to(ROOT)} includes cells/{cells.name} but its grid is '
                         f'{values["GRID_COLS"]} x {values["GRID_ROWS"]} = {need} cells: the cards for the cells '
                         f'it gained are missing. Run tools/generate_cells.py and include cells/{need}.yaml')
        else:
            fail(f'{path.relative_to(ROOT)} has no GRID_COLS / GRID_ROWS: every board says what its page holds')
        # And a word of its own for what it is. The screen reports it and ESP Screens goes by it (boards.json,
        # camera sizes); a board that kept the word of the board it was copied from would answer for that one.
        board_id = values.get('BOARD_ID', '').strip('"')
        if not board_id:
            fail(f'{path.relative_to(ROOT)} does not define BOARD_ID: the screen reports it so the manager '
                 f'knows which board it is')
        elif board_id in seen_ids:
            fail(f'{path.relative_to(ROOT)} and {seen_ids[board_id]} both call themselves "{board_id}": a board '
                 f'copied from another keeps its word unless tools/new_board.py sets it')
        else:
            seen_ids[board_id] = path.relative_to(ROOT)
    for board, path in profiles.BOARDS.items():
        found = profiles.substitutions_of(path).get('BOARD_ID', '').strip('"')
        if found and found != board:
            fail(f'{path.relative_to(ROOT)} says BOARD_ID "{found}" but the manager knows it as "{board}" '
                 f'(tools/profiles.py): boards.json and the screen would not agree')
    # Turning: every board offers the half turn through its own Rotation select, a square one the quarter turns as well
    # (a list of options cannot come from a substitution, so the block is per board); the shared tree applies the angle.
    blocks = {}
    for board, path in profiles.BOARDS.items():
        values = profiles.substitutions_of(path)
        square = values['DISPLAY_W'] == values['DISPLAY_H']
        select = re.search(r'^select:\n(.*?)(?=^[a-z_0-9]+:|\Z)', path.read_text(), re.M | re.S)
        options = re.search(r'^    options: (\[.*?\])$', select[1], re.M) if select else None
        wanted = '["0°", "90°", "180°", "270°"]' if square else '["0°", "180°"]'
        if not options or options[1] != wanted:
            fail(f'{path.relative_to(ROOT)}: the Rotation select offers {options[1] if options else "nothing"}, its glass '
                 f'({values["DISPLAY_W"]} x {values["DISPLAY_H"]}) asks for {wanted}')
        blocks[board] = '\n'.join(line for line in select[1].replace(options[1], '').split('\n') if not line.lstrip().startswith('#'))
    if len(set(blocks.values())) != 1:
        fail('the Rotation select differs between the board files beyond its options; keep the three alike')
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
