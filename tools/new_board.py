"""Write a board file for a new panel from one of the boards we have, keeping every size in millimetres.

usage: new_board.py NAME --from cyd|guition --width W --height H --inch D [--cols C --rows R] [--rotation 0|90|180|270]
                     [--dpi D]   -> packages/boards/NAME.yaml

Every pixel size and font size of the template board is scaled by dpi_new / dpi_template (the "same millimetres"
rule, docs/RESPONSIVE.md); the grid comes from the proposal (propose_grid.py) or the given cols x rows, and the
look from the template (the CYD's is compact, the Guition's standard). Touch tuning, timings, byte caps and flags
stay as they are. DISPLAY_W/H are the logical canvas; the display block's native dimensions follow the rotation
(LVGL turns the canvas). A board only tried out is best called lab-<name>: Git ignores those, and
`tools/generate_cells.py --lab` writes the cards of its grid. The hardware sections are the template's and are
replaced by hand (docs/ADDING_A_BOARD.md).
"""
import argparse, math, re, sys
from pathlib import Path

WORKTREE = Path(__file__).resolve().parents[1]
TEMPLATES = {'cyd': ('cyd-2432s028.yaml', 143.0), 'guition': ('guition-4848s040.yaml', 170.0)}
# Names that are not pixels or points (never scaled).
KEEP = re.compile(r'^(TOUCH_|DISPLAY_|LVGL_|GRID_|LOOK$|ALERT_.*_MAX|.*_MS$|LVGL_BUFFER_SIZE|PAGE_DOTS_LARGE|DEVICE_|EFFECTS_ROW_FONT|TOUCH_TEST_TITLE|TOUCH_TEST_PAINT|BOARD_|LVGL_DEFAULT_FONT|.*_HOOK$|.*_TAIL$|BOOT_|APPLY_|MAY_OPEN_EXTRA|AWAY_EXTRA|SETTINGS_HOLD_X|SETTINGS_HOLD_W)')
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

    if a.cols and a.rows:
        cols, rows = a.cols, a.rows
    else:
        sys.path.insert(0, str(Path(__file__).resolve().parent))
        import propose_grid
        p = propose_grid.propose(W, H, a.inch, look={'cyd': 'compact', 'guition': 'standard'}[a.template])
        cols, rows = p['cols'], p['rows']
    margin, gap_x, gap_y = get('GRID_MARGIN'), get('GRID_GAP_X'), get('GRID_GAP_Y')
    top, bar = get('SCROLL_Y'), get('PAGE_BAR_H')
    tile_w = (W - 2 * margin - (cols - 1) * gap_x) // cols
    scroll_h = H - top - bar
    tile_h = (scroll_h - (rows - 1) * gap_y) // rows
    put('DISPLAY_W', W); put('DISPLAY_H', H); put('LVGL_ROTATION', a.rotation)
    put('GRID_COLS', cols); put('GRID_ROWS', rows)
    put('SCROLL_H', scroll_h); put('TILE_W', tile_w); put('TILE_H', tile_h)
    put('DISPLAY_DPI', int(round(dpi))); put('LOOK', {'cyd': 'compact', 'guition': 'standard'}[a.template])
    put('PAGE_KEY_W', W // 2)
    # Things that follow the canvas, not the dpi.
    if re.search(r'(?m)^  SETTINGS_HOLD_W: "\d+"', text):
        band = get('EDGE_SWIPE_BAND_PX') if re.search(r'(?m)^  EDGE_SWIPE_BAND_PX:', text) else 0
        put('SETTINGS_HOLD_W', W - 2 * band)
    inset, size = get('TOUCH_TEST_INSET'), get('TOUCH_TEST_SIZE')
    put('TOUCH_TEST_FAR_X', W - inset - size - 1); put('TOUCH_TEST_FAR_Y', H - inset - size - 1)
    put('TOUCH_TEST_MID_X', W // 2 - size // 2); put('TOUCH_TEST_MID_Y', H // 2 - size // 2)
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
    # The board brings the cards of its own grid.
    text = re.sub(r'(cells: !include \.\./cells/)\d+(\.yaml)', rf'\g<1>{cols * rows}\g<2>', text)
    text = re.sub(r'(one per cell, from the file for )\d+( cells)', rf'\g<1>{cols * rows}\g<2>', text)
    # Native panel size for the display block: the canvas turned back by the rotation.
    native_w, native_h = (H, W) if a.rotation in (90, 270) else (W, H)
    text, n = re.subn(r'(?s)(dimensions:\s*\n\s*width:\s*)\d+(\s*\n\s*height:\s*)\d+', rf'\g<1>{native_w}\g<2>{native_h}', text, count=1)
    if not n:
        text, n = re.subn(r'(dimensions: \{width: )\d+(, height: )\d+', rf'\g<1>{native_w}\g<2>{native_h}', text, count=1)
    assert n == 1, 'display dimensions'
    text = f'# Board {a.name}: {W}x{H} at {dpi:.0f} dpi ({a.inch}"), scale {f:.3f} from the {a.template}, grid {cols}x{rows}, tile {tile_w}x{tile_h} px.\n' + text
    dest = WORKTREE / 'packages' / 'boards' / f'{a.name}.yaml'
    dest.write_text(text)
    print(f'{dest.name}: {W}x{H} {dpi:.0f} dpi scale {f:.3f} grid {cols}x{rows} tile {tile_w}x{tile_h} margin {margin} gaps {gap_x}/{gap_y} top {top} bar {bar}')


if __name__ == '__main__':
    main()
