"""The shape a screen reports (firmware 0.2.9x): "<width>x<height> <columns>x<rows>".

The add-on reads it from the screen's own diagnostic sensor, so the editor draws the glass and the cells of
the screen in front of it instead of guessing from the board it was built for. Firmware from before says
nothing, and then the two boards that shipped first decide.
"""
import unittest
from pathlib import Path

import sys
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'screen_manager' / 'app'))
import core


class Shape(unittest.TestCase):
    def test_a_screen_reports_its_canvas_and_its_grid(self):
        self.assertEqual(core.parse_shape('800x480 3x2'),
                         {'width': 800, 'height': 480, 'columns': 3, 'rows': 2})
        self.assertEqual(core.parse_shape(' 320x240 2x3 '),
                         {'width': 320, 'height': 240, 'columns': 2, 'rows': 3})

    def test_anything_else_is_no_shape(self):
        for text in ('', None, 'unavailable', '800x480', '800 x 480 3x2', '0x0 2x3', '800x480 0x3',
                     '800x480 13x2', '800x480 9x9'):
            self.assertIsNone(core.parse_shape(text), text)

    def test_a_screen_without_the_sensor_falls_back_to_its_board(self):
        self.assertEqual(core.shape_of({'board': 'guition'}), core.SHAPES['guition'])
        self.assertEqual(core.shape_of({'board': 'unknown'}), core.SHAPES['cyd'])
        self.assertEqual(core.shape_of({'board': 'guition', 'shape': None}), core.SHAPES['guition'])
        reported = {'width': 800, 'height': 480, 'columns': 3, 'rows': 2}
        self.assertEqual(core.shape_of({'board': 'guition', 'shape': reported}), reported)

    def test_every_shape_holds_a_page_of_cells(self):
        for board, shape in core.SHAPES.items():
            self.assertLessEqual(shape['columns'] * shape['rows'], core.MAX_SLOTS, board)
            self.assertGreaterEqual(shape['width'], 200, board)

    def test_the_firmware_publishes_the_sensor_the_manager_reads(self):
        core_yaml = (ROOT / 'packages' / 'core.yaml').read_text()
        self.assertIn('name: "Screen layout"', core_yaml)
        self.assertIn('runtime_tiles::GRID_COLUMNS', core_yaml)
        self.assertIn('Screen layout', str(core.NAME_SCREEN_LAYOUT))
        self.assertIn('Screen layout', core.SCREEN_ENTITY_NAMES)


if __name__ == '__main__':
    unittest.main()
