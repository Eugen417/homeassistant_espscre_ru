"""Regression checks for the 320x240 page geometry and event guards."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = (ROOT / 'home-like-2432s028.yaml').read_text()
VALUES = dict(re.findall(r'^  (\w+): "([^"]*)"', SOURCE, re.M))

class LayoutTests(unittest.TestCase):
    def test_visible_rows_fit_viewport_and_leave_navigation_clear(self):
        v = lambda k: int(VALUES[k])
        rectangles = []
        for row in range(1, 4):
            for col in range(1, 3):
                x, y = v(f'GRID_COL{col}_X'), v('SCROLL_Y') + v(f'GRID_ROW{row}_Y')
                w, h = v('TILE_W'), v('TILE_H')
                self.assertGreaterEqual(x, 0)
                self.assertLessEqual(x + w, v('DISPLAY_W'))
                self.assertLessEqual(y + h, v('SCROLL_Y') + v('SCROLL_H'))
                self.assertLessEqual(y + h, v('DISPLAY_H') - 34)
                for xx, yy, ww, hh in rectangles:
                    self.assertFalse(x < xx + ww and x + w > xx and y < yy + hh and y + h > yy)
                rectangles.append((x, y, w, h))

    def test_runtime_binds_every_tile_and_guards_a_tap(self):
        """The tiles are bound by the runtime; it filters a tap before anything happens."""
        for n in range(1, 11):
            self.assertIn(f'runtime_tiles::bind({n - 1}, id(tile{n})', SOURCE)
        runtime = (ROOT / 'components/smart_display/runtime_tiles.h').read_text()
        self.assertIn('if (!allowed(esphome::millis(), 100 + w.index, model.tiles[w.index].entity)) return;', runtime)

    def test_navigation_is_above_grid_but_below_modal_overlays(self):
        grid = SOURCE.index('            id: tile_scroll')
        nav = SOURCE.index('            id: page_prev')
        modal = SOURCE.index('            id: brightness_overlay')
        self.assertLess(grid, nav)
        self.assertLess(nav, modal)

    def test_page_keys_are_the_halves_of_the_band_under_the_tiles(self):
        """Firmware 0.2.69+: a chevron in each half of the band, the dots between them take no touches."""
        for name in ('home-like-2432s028.yaml', 'guition-4848s040.yaml'):
            source = (ROOT / name).read_text()
            values = dict(re.findall(r'^  (\w+): "([^"]*)"', source, re.M))
            band = int(values['DISPLAY_H']) - int(values['SCROLL_Y']) - int(values['SCROLL_H'])
            half = int(values['DISPLAY_W']) // 2
            for key, glyph in (('page_prev', 'F0141'), ('page_next', 'F0142')):
                block = source.split(f'            id: {key}\n', 1)[1].split('\n        - ', 1)[0]
                self.assertIn(f'width: {half}\n', block, f'{name} {key}')
                self.assertIn(f'height: {band}\n', block, f'{name} {key}')
                self.assertIn('styles: paint_page_pressed', block, f'{name} {key}: the half lights up under a finger')
                self.assertIn(f'\\U000{glyph}', block, f'{name} {key}')
                self.assertIn('text_font: materialdesign_icons_mini', block, f'{name} {key}')
                self.assertNotIn('Previous', block)
                self.assertNotIn('Next  >', block)
            number = source.split('            id: page_number\n', 1)[1].split('\n        - ', 1)[0]
            self.assertIn('clickable: false', number, name)
            self.assertIn(f'height: {band}\n', number, name)
            self.assertIn('settings_screen::page_dots(id(page_number)', source, name)
        runtime = (ROOT / 'components/smart_display/runtime_tiles.h').read_text()
        self.assertIn('settings_screen::page_dots(nav_number,page,pages,', runtime)
        self.assertIn('set_hidden(control,!bar)', runtime)

    def test_pagination_is_conditional_and_excess_tiles_are_hidden(self):
        block = SOURCE.split('  - id: show_tile_page\n', 1)[1].split('  - id: wake_display', 1)[0]
        self.assertIn('const bool paginated = count > 6;', block)
        self.assertIn('i < count &&', block)
        self.assertIn('if (!paginated) id(tile_page) = 0;', block)
        self.assertIn('id(page_prev), id(page_next), id(page_number)', block)
        self.assertIn('else lv_obj_add_flag(control, LV_OBJ_FLAG_HIDDEN);', block)

    def test_self_test_cannot_call_a_home_assistant_action(self):
        block = SOURCE.split('  - id: ui_self_test\n', 1)[1].split('  - id: show_tile_page', 1)[0]
        self.assertNotIn('homeassistant.action:', block)
        self.assertNotIn('do_tile_action', block)
        self.assertNotIn('climate_target_commit', block)

if __name__ == '__main__':
    unittest.main()
