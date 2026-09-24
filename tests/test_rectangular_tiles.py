"""Rectangular occupancy and compatibility, independent of card rendering."""
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'screen_manager/app'))
from core import Grid, validate_layout, resolve_controls, tile_options, pack_page
from layout_migrations import migrate_legacy
from page_layout import compile_tiles, legacy_projection, LayoutError, validate_document


def tile(entity, slot, size='single'):
    return dict(entity=entity, name='', slot=slot, options={'size': size})


class Rectangles(unittest.TestCase):
    def test_cells_are_rectangles_not_contiguous_ranges(self):
        grid = Grid(3, 3)
        self.assertEqual(grid.footprint(1, 'tall'), (1, 4))
        self.assertEqual(grid.footprint(1, 'square'), (1, 2, 4, 5))
        self.assertTrue(grid.fits(3, 'square'))
        self.assertFalse(grid.fits(2, 'square'))
        self.assertFalse(grid.fits(6, 'tall'))
        self.assertFalse(Grid(1, 4).fits(0, 'square'))
        self.assertFalse(Grid(4, 1).fits(0, 'tall'))

    def test_neighbor_cells_remain_usable_but_covered_rows_do_not(self):
        grid = Grid(2, 3)
        layout = dict(title='Test', tiles=[tile('light.tall', 0, 'tall'), tile('sensor.next', 1)])
        validate_layout(layout, grid=grid)
        layout['tiles'][1]['slot'] = 2
        with self.assertRaises(ValueError): validate_layout(layout, grid=grid)
        layout['tiles'] = [tile('light.tall', 4, 'tall')]
        with self.assertRaises(ValueError): validate_layout(layout, grid=grid)

    def test_packing_respects_holes_and_page_boundaries(self):
        grid = Grid(2, 3)
        tiles = [tile('light.tall', 0, 'tall')] + [tile(f'sensor.n{i}', 0) for i in range(5)]
        self.assertEqual(grid.pack(tiles), [0, 1, 3, 4, 5, 6])
        self.assertEqual(grid.pack([tile('light.square', 0, 'square'), tile('sensor.n', 0)]), [0, 4])

    def test_page_document_roundtrip_and_legacy_refusal(self):
        for size, dimensions in [('tall', (1, 2)), ('square', (2, 2))]:
            with self.subTest(size=size):
                record = migrate_legacy(dict(title='Test', tiles=[tile('light.test', 0, size)]), Grid())
                source = record['layout']['pages'][0]['tiles'][0]
                self.assertEqual((source['placement']['columns'], source['placement']['rows']), dimensions)
                self.assertEqual(compile_tiles(record['layout'], Grid())[0]['options']['size'], size)
                with self.assertRaises(LayoutError): legacy_projection(record)
                source['appearance'].pop('presentation')
                validate_document(record['layout'], Grid())
                with self.assertRaises(LayoutError): legacy_projection(record)

    def test_two_by_two_keeps_wide_controls_but_tall_keeps_single_design(self):
        self.assertEqual(resolve_controls(tile('light.test', 0, 'square')), resolve_controls(tile('light.test', 0, 'wide')))
        self.assertIsNone(resolve_controls(tile('light.test', 0, 'tall')))

    def test_events_keep_square_controls_and_in_order_packing(self):
        self.assertEqual(tile_options({'size': 'square'}, {'controls': 'playback'})['size'], 'square')
        items = [tile('sensor.a', 0), tile('sensor.b', 0), tile('light.c', 0, 'wide'), tile('sensor.d', 0)]
        pack_page(items, 1, Grid(3, 3))
        self.assertEqual([item['slot'] for item in items], [9, 10, 12, 14])
