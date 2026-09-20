"""Write a board file for a new panel from one of the boards we have, keeping every size in millimetres.

usage: new_board.py NAME --from cyd|guition --width W --height H --inch D [--cols C --rows R] [--rotation 0|90|180|270]
                     [--look compact|standard]   -> <worktree>/packages/boards/lab-NAME.yaml

Every pixel size and font size of the template board is scaled by dpi_new / dpi_template (the "same millimetres"
rule of the plan); the grid values come from the proposal (propose_grid.py) or the given cols x rows. Touch tuning,
timings, byte caps and flags stay as they are. DISPLAY_W/H are the logical canvas; the display block's native
dimensions follow the rotation (LVGL turns the canvas).
"""
import argparse, math, re, sys
from pathlib import Path

WORKTREE = Path(__file__).resolve().parents[1]
TEMPLATES = {'cyd': ('cyd-2432s028.yaml', 143.0), 'guition': ('guition-4848s040.yaml', 170.0)}
# Names that are not pixels or points (never scaled).
KEEP = re.compile(r'^(TOUCH_|DISPLAY_|LVGL_|GRID_|LOOK$|ROTATION|ALERT_.*_MAX|.*_MS$|LVGL_BUFFER_SIZE|PAGE_DOTS_LARGE|DEVICE_|ROTATION_SUPPORTED|EFFECTS_ROW_FONT|PICKER_CHIP_FONT|TOUCH_TEST_TITLE|TOUCH_TEST_PAINT|BOARD_|LVGL_DEFAULT_FONT|.*_HOOK$|.*_TAIL$|BOOT_|APPLY_|MAY_OPEN_EXTRA|AWAY_EXTRA|TICK_HOOK|UI_STATE_HOOK|CLOSE_CARDS_HOOK|TOUCH_TEST_READY_LOG|ALERT_SHOW_HOOK|CLIMATE_MODE_CHOSEN_HOOK|OPEN_VALUE_OVERLAY_TAIL|APPLY_ROTATION|APPLY_BACKLIGHT|APPLY_ROTATION_ENTITY|SETTINGS_HOLD_X|SETTINGS_HOLD_W)')
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
    # The light card's vertical slider stays on the canvas.
    slider_scaled = get('OVERLAY_SLIDER_H')
    put('OVERLAY_SLIDER_H', min(slider_scaled, H - get('OVERLAY_VALUE_Y') - get('OVERLAY_SLIDER_Y') - int(40 * f)))
    # The icon inside the slider keeps its place relative to the slider's length.
    put('OVERLAY_ICON_Y', int(round(get('OVERLAY_ICON_Y') * get('OVERLAY_SLIDER_H') / max(1, slider_scaled))))
    # The Guition's climate card carries a pixel table for 480 px of height: squeeze it to a lower canvas, and keep
    # the fan/swing card for canvases with room (a component would hide itself instead).
    fy = min(f, H / 480) if a.template == 'guition' else f
    m = re.search(r'(?s)static const Layout layouts\[6\] = \{.*?\n\s*\};', text)
    if m:
        block = m.group(0)
        lines = block.split('\n')
        out_lines = []
        for line in lines:
            if line.strip().startswith('{') and line.strip().endswith('},'):
                nums = re.findall(r'-?\d+', line)
                vals = [str(int(round(int(n) * fy))) for n in nums]
                line = '              {' + ', '.join(vals) + '},'
            out_lines.append(line)
        text = text.replace(block, '\n'.join(out_lines))

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
