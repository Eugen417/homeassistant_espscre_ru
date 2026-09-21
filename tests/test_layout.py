"""Regression checks for the page geometry (the board's grid) and the event guards."""
from pathlib import Path
import re
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import profiles  # noqa: E402
SOURCE = profiles.resolved('home-like-2432s028.yaml')
VALUES = dict(re.findall(r'^  (\w+): "([^"]*)"', SOURCE, re.M))

class LayoutTests(unittest.TestCase):
    def test_the_cells_fill_the_glass_and_leave_the_page_bar_clear(self):
        """A board states columns, rows, its margin and its gaps; LVGL divides the tile area over the cells."""
        for name in profiles.PROFILES:
            values = dict(re.findall(r'^  (\w+): "([^"]*)"', profiles.resolved(name), re.M))
            v = lambda k: int(values[k])
            cols, rows = v('GRID_COLS'), v('GRID_ROWS')
            self.assertGreaterEqual(cols, 1, name)
            self.assertGreaterEqual(rows, 1, name)
            self.assertEqual(2 * v('GRID_MARGIN') + cols * v('TILE_W') + (cols - 1) * v('GRID_GAP_X'),
                             v('DISPLAY_W'), f'{name}: the cells and their gaps fill the width')
            self.assertEqual(rows * v('TILE_H') + (rows - 1) * v('GRID_GAP_Y'),
                             v('SCROLL_H'), f'{name}: the cells and their gaps fill the tile area')
            self.assertEqual(v('SCROLL_Y') + v('SCROLL_H') + v('PAGE_BAR_H'),
                             v('DISPLAY_H'), f'{name}: top bar, tiles and page bar fill the glass')
            # Without the page bar the tile area reaches the bottom edge, keeping the margin the sides have.
            self.assertGreater(v('DISPLAY_H') - v('SCROLL_Y') - v('GRID_MARGIN'), v('SCROLL_H'), name)

    def test_the_cards_are_cells_of_an_lvgl_grid(self):
        """No card carries a coordinate: the container is a grid and place_page only names a cell and its span."""
        self.assertIn('type: GRID', SOURCE)
        self.assertNotRegex(SOURCE, r'id: tile\d+\n\s+x: ')
        self.assertEqual(SOURCE.count('grid_cell_row_pos: 0'), int(VALUES['GRID_COLS']) * int(VALUES['GRID_ROWS']))
        runtime = (ROOT / 'components/smart_display/runtime_tiles.h').read_text()
        self.assertIn('lv_obj_set_grid_dsc_array(container, grid_columns_dsc.data(), grid_rows_dsc.data());', runtime)
        self.assertIn('lv_obj_set_grid_cell(w.tile,LV_GRID_ALIGN_STRETCH,column,span_x,LV_GRID_ALIGN_STRETCH,row,span_y);', runtime)

    def test_runtime_binds_every_tile_and_guards_a_tap(self):
        """The tiles are bound by the runtime; it filters a tap before anything happens."""
        cells = int(VALUES['GRID_COLS']) * int(VALUES['GRID_ROWS'])
        for n in range(1, cells + 1):
            self.assertIn(f'runtime_tiles::bind({n - 1}, id(tile{n})', SOURCE)
        self.assertNotIn(f'runtime_tiles::bind({cells}, ', SOURCE)
        runtime = (ROOT / 'components/smart_display/runtime_tiles.h').read_text()
        self.assertIn('if (!allowed(esphome::millis(), 100 + w.index, model.tiles[w.index].entity)) return;', runtime)

    def test_navigation_is_above_grid_but_below_modal_overlays(self):
        grid = SOURCE.index('            id: tile_scroll')
        nav = SOURCE.index('            id: page_prev')
        modal = SOURCE.index('            id: color_detail_overlay')
        self.assertLess(grid, nav)
        self.assertLess(nav, modal)

    def test_page_keys_are_the_halves_of_the_band_under_the_tiles(self):
        """Firmware 0.2.69+: a chevron in each half of the band, the dots between them take no touches."""
        for name in profiles.PROFILES:
            source = profiles.resolved(name)
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
        runtime = (ROOT / 'components/smart_display/runtime_tiles.h').read_text()
        self.assertIn('settings_screen::page_dots(nav_number,page,pages,', runtime)
        self.assertIn('set_hidden(control,!bar)', runtime)

    def test_nothing_is_placed_before_a_layout_arrives(self):
        """Until ESP Screens sends a layout the cells stay empty; place_page hides every slot the page has no card for."""
        block = SOURCE.split('  - id: show_tile_page\n', 1)[1].split('  - id: apply_screen_settings', 1)[0]
        self.assertIn('lv_obj_add_flag(w.tile, LV_OBJ_FLAG_HIDDEN)', block)
        self.assertIn('id(page_prev), id(page_next), id(page_number)', block)
        runtime = (ROOT / 'components/smart_display/runtime_tiles.h').read_text()
        self.assertIn('else{lv_obj_add_flag(w.tile,LV_OBJ_FLAG_HIDDEN);hide_extra(w);hide_panel(w);}', runtime)

    def test_self_test_cannot_call_a_home_assistant_action(self):
        block = SOURCE.split('  - id: ui_self_test\n', 1)[1].split('  - id: show_tile_page', 1)[0]
        self.assertNotIn('homeassistant.action:', block)
        self.assertNotIn('do_tile_action', block)
        self.assertNotIn('climate_target_commit', block)

if __name__ == '__main__':
    unittest.main()
