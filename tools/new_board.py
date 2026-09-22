"""Write a board file for a new panel from one of the boards we have, keeping every size in millimetres.

usage: new_board.py NAME --from cyd|guition --width W --height H --inch D [--cols C --rows R] [--rotation 0|90|180|270]
                     [--portrait-cols C --portrait-rows R] [--dpi D]   -> packages/boards/NAME.yaml

Every pixel size and font size of the template board is scaled by dpi_new / dpi_template (the "same millimetres"
rule, docs/RESPONSIVE.md); the grid comes from the proposal (propose_grid.py) or the given cols x rows, and the
look from the template (the CYD's is compact, the Guition's standard). Touch tuning, timings, byte caps and flags
stay as they are. --width and --height are the canvas of the screen lying down and --rotation the LVGL angle that
lays the panel out that way, so PANEL_W/PANEL_H are that canvas turned back: the pixels the glass really has, which
the display block drives whichever way the screen ends up hanging. The board also states the grid of a page standing
up, proposed for the turned canvas unless it is given. A board only tried out is best called lab-<name>: Git ignores
those, and `tools/generate_cells.py --lab` writes the cards of its grid. The hardware sections are the template's
and are replaced by hand (docs/ADDING_A_BOARD.md).
"""
import argparse, math, re, sys
from pathlib import Path

WORKTREE = Path(__file__).resolve().parents[1]
TEMPLATES = {'cyd': ('cyd-2432s028.yaml', 143.0), 'guition': ('guition-4848s040.yaml', 170.0)}
# Names that are not pixels or points (never scaled).
KEEP = re.compile(r'^(TOUCH_|DISPLAY_|LVGL_|GRID_|PANEL_|ROTATION_|LOOK$|ALERT_.*_MAX|.*_MS$|LVGL_BUFFER_SIZE|PAGE_DOTS_LARGE|DEVICE_|EFFECTS_ROW_FONT|TOUCH_TEST_TITLE|TOUCH_TEST_PAINT|BOARD_|LVGL_DEFAULT_FONT|.*_HOOK$|.*_TAIL$|BOOT_|APPLY_|MAY_OPEN_EXTRA|AWAY_EXTRA|SETTINGS_HOLD_X)')
SUB = re.compile(r'^(  )([A-Z][A-Z0-9_]*): "(-?\d+)"(.*)$')


def scaled(name, value, f):
    if KEEP.match(name):
        return value
    v = int(value)
    out = int(round(v * f))
    if name.startswith('FONT_') and out < 8:
        out = 8
    if v and out == 0:
        out = 1 if v > 0 else -1
    return str(out)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('name'); ap.add_argument('--from', dest='template', required=True, choices=TEMPLATES)
    ap.add_argument('--width', type=int, required=True); ap.add_argument('--height', type=int, required=True)
    ap.add_argument('--inch', type=float, required=True); ap.add_argument('--cols', type=int); ap.add_argument('--rows', type=int)
    ap.add_argument('--portrait-cols', dest='portrait_cols', type=int); ap.add_argument('--portrait-rows', dest='portrait_rows', type=int)
    ap.add_argument('--rotation', type=int, default=0); ap.add_argument('--dpi', type=float)
    a = ap.parse_args()
    file, dpi_ref = TEMPLATES[a.template]
    W, H = a.width, a.height
    dpi = a.dpi or math.hypot(W, H) / a.inch
    f = dpi / dpi_ref
    text = (WORKTREE / 'packages' / 'boards' / file).read_text()
    lines = text.split('\n')
    out = []
    in_subst = False
    for line in lines:
        if re.match(r'^substitutions:', line):
            in_subst = True
        elif re.match(r'^[a-z]', line):
            in_subst = False
        m = SUB.match(line) if in_subst else None
        if m:
            indent, name, value, rest = m.groups()
            line = f'{indent}{name}: "{scaled(name, value, f)}"{rest}'
        out.append(line)
    text = '\n'.join(out)

    def get(name):
        return int(re.search(rf'(?m)^  {name}: "(-?\d+)"', text).group(1))

    def put(name, value):
        nonlocal text
        text, n = re.subn(rf'(?m)^(  {name}: )"[^"]*"', rf'\g<1>"{value}"', text)
        assert n == 1, name

    sys.path.insert(0, str(Path(__file__).resolve().parent))
    import propose_grid
    look = {'cyd': 'compact', 'guition': 'standard'}[a.template]
    if a.cols and a.rows:
        cols, rows = a.cols, a.rows
    else:
        p = propose_grid.propose(W, H, a.inch, look=look)
        cols, rows = p['cols'], p['rows']
    # The grid of a page when this screen stands up: the same proposal for the turned canvas, which on square glass
    # is the same answer again. It is a grid of its own because a card keeps its size in millimetres, so glass that
    # holds three columns lying down may hold one standing up.
    if a.portrait_cols and a.portrait_rows:
        tall_cols, tall_rows = a.portrait_cols, a.portrait_rows
    else:
        tall = propose_grid.propose(H, W, a.inch, look=look)
        tall_cols, tall_rows = tall['cols'], tall['rows']
    margin, gap_x, gap_y = get('GRID_MARGIN'), get('GRID_GAP_X'), get('GRID_GAP_Y')
    top, bar = get('SCROLL_Y'), get('PAGE_BAR_H')
    # The panel's own pixels: the canvas of the screen lying down, turned back by the angle that lays it out that
    # way. The display block drives the panel at this size whichever way the screen hangs; LVGL turns the picture.
    native_w, native_h = (H, W) if a.rotation % 360 in (90, 270) else (W, H)
    put('PANEL_W', native_w); put('PANEL_H', native_h); put('ROTATION_LANDSCAPE', a.rotation % 360)
    put('GRID_COLS', cols); put('GRID_ROWS', rows)
    put('GRID_COLS_PORTRAIT', tall_cols); put('GRID_ROWS_PORTRAIT', tall_rows)
    put('DISPLAY_DPI', int(round(dpi))); put('LOOK', look)
    # Things that follow the canvas, not the dpi. The cells, the page bar and the settings strip are not among them
    # any more: the firmware works those out from the canvas LVGL gives it, so they are right either way up.
    put('ALERT_CARD_W', min(get('ALERT_CARD_W'), W - 2 * margin)); put('ALERT_CARD_H', min(get('ALERT_CARD_H'), H - 2 * margin))
    # The alert's inner geometry follows its card (a component would compute this itself).
    card_w, card_h = get('ALERT_CARD_W'), get('ALERT_CARD_H')
    put('ALERT_TEXT_W', max(40, card_w - get('ALERT_TEXT_X') - get('ALERT_ICON_X')))
    put('ALERT_SUBTITLE_H', max(20, card_h - get('ALERT_SUBTITLE_Y') - get('ALERT_BUTTON_H') - get('ALERT_BUTTON_INSET') - 6))
    if re.search(r'(?m)^  CAMERA_FULL_W:', text):
        put('CAMERA_FULL_W', W); put('CAMERA_FULL_H', H)
        put('CAMERA_THUMB_W', min(get('CAMERA_THUMB_W'), W - 2 * margin)); put('CAMERA_THUMB_H', min(get('CAMERA_THUMB_H'), H // 2))
        put('ALERT_CARD_H_IMAGE', min(get('ALERT_CARD_H_IMAGE'), H - 2 * margin))
    put('DEVICE_NAME', a.name); put('DEVICE_FRIENDLY_NAME', a.name)
    # The board says which board it is: the screen reports this word and ESP Screens knows what it can do
    # (its shape, whether it draws camera pictures). Without it a new board kept the template's word and
    # called itself a Guition.
    put('BOARD_ID', a.name)
    # The board brings the cards of its own grid: one per cell of the page that holds most, which is the page lying
    # down on every board so far and has to cover both, because a screen is built one way or the other from one file.
    cells = max(cols * rows, tall_cols * tall_rows)
    text = re.sub(r'(cells: !include \.\./cells/)\d+(\.yaml)', rf'\g<1>{cells}\g<2>', text)
    text = re.sub(r'(one per cell, from the file for )\d+( cells)', rf'\g<1>{cells}\g<2>', text)
    # The display block drives the panel at its own size.
    text, n = re.subn(r'(?s)(dimensions:\s*\n\s*width:\s*)\d+(\s*\n\s*height:\s*)\d+', rf'\g<1>{native_w}\g<2>{native_h}', text, count=1)
    if not n:
        text, n = re.subn(r'(dimensions: \{width: )\d+(, height: )\d+', rf'\g<1>{native_w}\g<2>{native_h}', text, count=1)
    assert n == 1, 'display dimensions'
    text = (f'# Board {a.name}: panel {native_w}x{native_h} at {dpi:.0f} dpi ({a.inch}"), scale {f:.3f} from the '
            f'{a.template}, {W}x{H} grid {cols}x{rows} lying down, {H}x{W} grid {tall_cols}x{tall_rows} standing up.\n') + text
    dest = WORKTREE / 'packages' / 'boards' / f'{a.name}.yaml'
    dest.write_text(text)
    print(f'{dest.name}: panel {native_w}x{native_h} {dpi:.0f} dpi scale {f:.3f} grid {cols}x{rows} lying down, '
          f'{tall_cols}x{tall_rows} standing up, margin {margin} gaps {gap_x}/{gap_y} top {top} bar {bar}')


if __name__ == '__main__':
    main()
