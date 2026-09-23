"""Write a board file for a new panel from one of the boards we have (docs/ADDING_A_BOARD.md).

usage: new_board.py NAME --from BOARD --width W --height H --inch D [--cols C --rows R] [--rotation 0|90|180|270]
                     [--portrait-cols C --portrait-rows R] [--dpi D] [--look standard|compact]   -> packages/boards/NAME.yaml

A board file is small since app 0.2.127: its hardware, the facts of its glass and its grid, and the packages it
includes (docs/PROFILES.md). Every size comes from the look, which scales it by DISPLAY_DPI so a tile, a letter and a
key keep their size in millimetres; nothing is copied or scaled here. What this writes:

- the template board's `packages:` (its build, hardware, look and features), with the cards of the new grid;
- the new board's word (BOARD_ID), its panel (the canvas lying down, turned back by --rotation), its density, and its
  grid both ways up: the proposal of tools/propose_grid.py, or --cols/--rows;
- the template's hardware sections, which are the template's and are replaced by hand.

Nothing is fitted to the glass here: what depends on the canvas (the cells, the page bar, the alert card) the firmware
works out on the glass itself.

--width and --height are the canvas of the screen lying down and --rotation the LVGL angle that lays the panel out that
way. A board only tried out is best called lab-<name>: Git ignores those, and `tools/generate_cells.py --lab` writes the
cards of its grid.
"""
import argparse
import math
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import profiles  # noqa: E402
import propose_grid  # noqa: E402

BANNER = '# ============================================================\n# No need to edit below this line.'


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('name')
    ap.add_argument('--from', dest='template', required=True, choices=sorted(profiles.BOARDS))
    ap.add_argument('--width', type=int, required=True)
    ap.add_argument('--height', type=int, required=True)
    ap.add_argument('--inch', type=float, required=True)
    ap.add_argument('--cols', type=int)
    ap.add_argument('--rows', type=int)
    ap.add_argument('--portrait-cols', dest='portrait_cols', type=int)
    ap.add_argument('--portrait-rows', dest='portrait_rows', type=int)
    ap.add_argument('--rotation', type=int, default=0, choices=(0, 90, 180, 270))
    ap.add_argument('--dpi', type=float)
    ap.add_argument('--look', choices=('standard', 'compact'))
    a = ap.parse_args()
    if not re.fullmatch(r'[a-z][a-z0-9-]*', a.name):
        raise SystemExit('The name is the file name and the board\'s word: lower case, digits and dashes.')

    template = profiles.BOARDS[a.template]
    source = template.read_text()
    W, H = a.width, a.height
    dpi = a.dpi or math.hypot(W, H) / a.inch
    look = a.look or profiles.evaluate(profiles.raw_substitutions(template))['LOOK']
    if a.cols and a.rows:
        cols, rows = a.cols, a.rows
    else:
        lying = propose_grid.propose(W, H, a.inch, look=look)
        cols, rows = lying['cols'], lying['rows']
    # The grid of a page when this screen stands up: the same proposal for the turned canvas, which on square glass
    # is the same answer again. A card keeps its size in millimetres, so glass that holds three columns lying down
    # may hold one standing up.
    if a.portrait_cols and a.portrait_rows:
        tall_cols, tall_rows = a.portrait_cols, a.portrait_rows
    else:
        tall = propose_grid.propose(H, W, a.inch, look=look)
        tall_cols, tall_rows = tall['cols'], tall['rows']
    if W == H:
        tall_cols, tall_rows = cols, rows
    # The panel's own pixels: the canvas lying down, turned back by the angle that lays it out that way. The display
    # block drives the panel at this size whichever way the screen hangs; LVGL turns the picture.
    native_w, native_h = (H, W) if a.rotation in (90, 270) else (W, H)
    # The board brings the cards of the page that holds most: a screen is built one way or the other from one file.
    cells = max(cols * rows, tall_cols * tall_rows)

    packages = re.search(r'^packages:\n(.*?)(?=^[a-z_0-9]+:|\Z)', source, re.M | re.S)[0].rstrip('\n') + '\n'
    packages = re.sub(r'(cells: !include \.\./cells/)\d+(\.yaml)', rf'\g<1>{cells}\g<2>', packages)
    packages = re.sub(r'(look: !include \.\./looks/)\w+(\.yaml)', rf'\g<1>{look}\g<2>', packages)
    lines = [
        f'# ESP Screens - {a.name}: a {a.inch}-inch {W} x {H} panel. Written by tools/new_board.py from the {a.template}',
        '# board: its packages and its hardware sections are the template\'s until they are replaced by this board\'s',
        '# (docs/ADDING_A_BOARD.md). docs/PROFILES.md says how the files of a screen fit together.',
        packages,
        'substitutions:',
        '  # The key of this board file; the firmware reports it so ESP Screens knows what board a screen is.',
        f'  BOARD_ID: "{a.name}"',
        f'  DEVICE_NAME: "{a.name}-new"',
        f'  DEVICE_FRIENDLY_NAME: "My {a.name}"',
        '',
        f'  # The glass: the panel\'s own pixels and its density, {math.hypot(W, H):.0f} diagonal pixels over {a.inch} inches.',
        f'  PANEL_W: "{native_w}"',
        f'  PANEL_H: "{native_h}"',
        f'  DISPLAY_DPI: "{dpi:.3f}"',
    ]
    if a.rotation:
        lines += [f'  # The LVGL angle that lays this panel out lying down ({W} x {H}). Standing up is a quarter further.',
                  f'  ROTATION_LANDSCAPE: "{a.rotation}"']
    lines += ['  # The tile grid: columns and rows of cells lying down, and standing up.',
              f'  GRID_COLS: "{cols}"', f'  GRID_ROWS: "{rows}"']
    if (tall_cols, tall_rows) != (cols, rows):
        lines += [f'  GRID_COLS_PORTRAIT: "{tall_cols}"', f'  GRID_ROWS_PORTRAIT: "{tall_rows}"']
    buffer = profiles.substitutions_of(template).get('LVGL_BUFFER_SIZE', '12%')
    lines += [f'  LVGL_BUFFER_SIZE: "{buffer}"                       # LVGL\'s draw buffer as a share of the screen', '']

    dest = profiles.ROOT / 'packages' / 'boards' / f'{a.name}.yaml'
    hardware = source[source.index(BANNER):] if BANNER in source else ''
    # The display block drives the panel at its own size.
    hardware, n = re.subn(r'(?s)(dimensions:\s*\n\s*width:\s*)\d+(\s*\n\s*height:\s*)\d+', rf'\g<1>{native_w}\g<2>{native_h}',
                          hardware, count=1)
    if not n:
        hardware = re.sub(r'(dimensions: \{width: )\d+(, height: )\d+', rf'\g<1>{native_w}\g<2>{native_h}', hardware, count=1)
    dest.write_text('\n'.join(lines) + '\n' + hardware)

    print(f'{dest.relative_to(profiles.ROOT)}: panel {native_w} x {native_h}, {dpi:.1f} dpi, the {look} look, grid {cols} x {rows} '
          f'lying down and {tall_cols} x {tall_rows} standing up ({cells} cards)')
    print('Next: replace the hardware sections, run tools/generate_cells.py (--lab for a lab board), '
          'tools/check_packages.py, and render it (docs/ADDING_A_BOARD.md).')


if __name__ == '__main__':
    main()
