"""Write screen_manager/app/boards.json: what every board looks like, straight from its own YAML.

A board says what its panel is (PANEL_W, PANEL_H, the pixels the glass really has), which LVGL angle lays
that panel out lying down (ROTATION_LANDSCAPE), how a page is divided each way (GRID_COLS x GRID_ROWS lying down,
GRID_COLS_PORTRAIT x GRID_ROWS_PORTRAIT standing up), its density and its look. The manager needs the same numbers
to draw a screen in the editor before it has ever been flashed, so they are worked out here instead of typed a
second time; `--check` fails when the file is out of date, which tools/check.sh runs. A screen that is online
reports its own shape as well (firmware 0.2.80), and that one wins: it knows how it was built and turned.

Every board gets both orientations under `orientations`, and keeps the landscape numbers at the top level, so
everything that only ever knew one shape per board reads the same file as before.

usage: generate_board_shapes.py [--check]
"""
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import profiles

OUT = profiles.ROOT / 'screen_manager' / 'app' / 'boards.json'


def orientations(values):
    """{'landscape': ..., 'portrait': ...}: the canvas, the grid and the LVGL angle of each way a board can hang.

    The canvas is the panel turned by that angle, because LVGL turns the picture and the touch together: a quarter
    turn swaps the sides, a half turn keeps them. Standing up is a quarter further than lying down, which is the
    one line that differs between the two builds (LVGL_ROTATION in the board file).

    Square glass has no second way to hang, so its portrait entry is its landscape entry, angle included: choosing
    to stand a square screen up then builds exactly the same firmware instead of turning the picture for nothing.
    """
    panel = (int(values['PANEL_W']), int(values['PANEL_H']))
    angle = int(values['ROTATION_LANDSCAPE']) % 360
    wide = (panel[1], panel[0]) if angle in (90, 270) else panel
    lying = {'width': wide[0], 'height': wide[1],
             'columns': int(values['GRID_COLS']), 'rows': int(values['GRID_ROWS']), 'rotation': angle}
    if wide[0] == wide[1]:
        return {'landscape': lying, 'portrait': dict(lying)}
    return {'landscape': lying,
            'portrait': {'width': wide[1], 'height': wide[0],
                         'columns': int(values['GRID_COLS_PORTRAIT']), 'rows': int(values['GRID_ROWS_PORTRAIT']),
                         'rotation': (angle + 90) % 360}}


def fit_percent(across, room, inset):
    """The firmware's own arithmetic, kept in step by hand (screen_alert::fit_percent in alert_overlay.h): what the
    alert card is brought back to when the table a board states lands on narrower glass. Whole percent, rounded down,
    and never more than the card's stated size."""
    space = room - 2 * inset
    if across <= 0 or space <= 0 or across <= space:
        return 100
    return space * 100 // across


def scaled(value, percent):
    return value if percent >= 100 else value * percent // 100


def camera_of(values, side, stated):
    """The pixel box of each picture a board draws, on the glass of one orientation.

    Full screen is that orientation's canvas: a screen standing up wants a picture standing up, and sending it the
    other one would letterbox it into a third of the glass. The thumb sits in the alert card, and the card is
    brought back to fit narrower glass (screen_alert::frame), so the thumb is capped at what is left inside the
    card after its two insets have come back with it, keeping the proportions of the one the board states.

    Lying down that factor is 1 on every board, so the board's own tuned numbers come through untouched; that is
    what tests/test_camera.py pins, and it is a good check on this arithmetic.
    """
    thumb_w, thumb_h = stated['thumb']
    percent = fit_percent(int(values['ALERT_CARD_W']), side['width'], int(values['ALERT_BUTTON_INSET']))
    room = scaled(int(values['ALERT_CARD_W']), percent) - 2 * scaled(int(values['ALERT_IMAGE_INSET']), percent)
    width = max(1, min(thumb_w, room))
    return {'full': [side['width'], side['height']],
            'thumb': [width, max(1, round(width * thumb_h / thumb_w))] if thumb_w else list(stated['thumb'])}


def shapes():
    """{entry file: shape} for every entry a screen's YAML can include, plus the board names themselves."""
    found = {}
    for board in profiles.BOARDS:
        # What a screen of this board sees: its board file over its look and features over the core's defaults.
        values = profiles.board_values(board)
        both = orientations(values)
        lying = both['landscape']
        shape = {'board': board,
                 # The landscape numbers stay the board's own: a screen that says nothing about itself is taken to
                 # hang the way its board file is written, and that is lying down.
                 'width': lying['width'], 'height': lying['height'],
                 'columns': lying['columns'], 'rows': lying['rows'],
                 'orientations': both,
                 'dpi': round(float(values['DISPLAY_DPI'])), 'look': values['LOOK'].strip('"'),
                 # Whether the backlight takes levels (app 0.2.105): on a board whose backlight is one line, a
                 # brightness percentage is a number that lies, so the settings panel shows a switch instead.
                 'dimmable': values.get('BACKLIGHT_DIMMABLE', 'true').strip('"') != 'false',
                 # Whether the screen can go dark at all (app 0.2.106): the Waveshare's backlight boost browns the
                 # board out when it switches on again, so that board has no standby and no night, and the settings
                 # panel leaves those out as the screen itself does.
                 'can_standby': values.get('CAN_STANDBY', 'true').strip('"') != 'false'}
        # A board that draws camera pictures says how large it wants them (its online_image components and the
        # frame in its alert card). Without those four the board has no camera at all, like the CYD: the manager
        # then refuses a camera tile instead of sending a picture that never arrives.
        box = {view: [int(values[f'CAMERA_{view.upper()}_W']), int(values[f'CAMERA_{view.upper()}_H'])]
               for view in ('full', 'thumb')
               if f'CAMERA_{view.upper()}_W' in values and f'CAMERA_{view.upper()}_H' in values}
        if len(box) == 2:
            # The landscape box stays at the top level with the other landscape numbers; each orientation gets the
            # box for its own glass, because a picture is shaped for the canvas it lands on.
            shape['camera'] = box
            for side in both.values():
                side['camera'] = camera_of(values, side, box)
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
