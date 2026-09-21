"""Write screen_manager/app/boards.json: what every board looks like, straight from its own YAML.

A board file already says what its screen is (DISPLAY_W, DISPLAY_H, DISPLAY_DPI, LOOK) and how its page is
divided (GRID_COLS, GRID_ROWS). The manager needs the same numbers to draw a screen in the editor before it has
ever been flashed, so they are copied here instead of typed a second time; `--check` fails when the file is out
of date, which tools/check.sh runs. A screen that is online reports its own shape as well (firmware 0.2.80),
and that one wins: it knows about rotation.

usage: generate_board_shapes.py [--check]
"""
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import profiles

OUT = profiles.ROOT / 'screen_manager' / 'app' / 'boards.json'


def shapes():
    """{entry file: shape} for every entry a screen's YAML can include, plus the board names themselves."""
    found = {}
    for board, path in profiles.BOARDS.items():
        values = profiles.substitutions_of(path)
        shape = {'board': board,
                 'width': int(values['DISPLAY_W']), 'height': int(values['DISPLAY_H']),
                 'columns': int(values['GRID_COLS']), 'rows': int(values['GRID_ROWS']),
                 'dpi': int(values['DISPLAY_DPI']), 'look': values['LOOK'].strip('"')}
        # A board that draws camera pictures says how large it wants them (its online_image components and the
        # frame in its alert card). Without those four the board has no camera at all, like the CYD: the manager
        # then refuses a camera tile instead of sending a picture that never arrives.
        box = {view: [int(values[f'CAMERA_{view.upper()}_W']), int(values[f'CAMERA_{view.upper()}_H'])]
               for view in ('full', 'thumb')
               if f'CAMERA_{view.upper()}_W' in values and f'CAMERA_{view.upper()}_H' in values}
        if len(box) == 2:
            shape['camera'] = box
        found[board] = shape
    for entry, board in profiles.ENTRIES.items():
        found[entry] = found[board]
    return dict(sorted(found.items()))


def main():
    text = json.dumps(shapes(), indent=1, sort_keys=True) + '\n'
    if '--check' in sys.argv:
        current = OUT.read_text() if OUT.exists() else ''
        if current != text:
            print(f'{OUT.relative_to(profiles.ROOT)} is out of date; run tools/generate_board_shapes.py')
            return 1
        print(f'{OUT.relative_to(profiles.ROOT)} matches the board files')
        return 0
    OUT.write_text(text)
    print(f'wrote {OUT.relative_to(profiles.ROOT)}: {", ".join(sorted(profiles.BOARDS))}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
