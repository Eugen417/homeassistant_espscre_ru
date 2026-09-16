"""Opening ESP Screens while the add-on is busy (app 0.2.58).

The page asks for the full inventory and opens the live stream at the same time. An add-on that is building
firmware answers the stream first, whose light payload has screens but no entities, so the first screen was
opened with an empty catalogue: the entity list said "No entities found" and the tiles showed entity ids until
a filter chip was pressed. The first full inventory now draws those parts again.
"""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = (ROOT / 'screen_manager/app/static/app.js').read_text()


def function(name):
    start = SCRIPT.index(f'function {name}(')
    return SCRIPT[start:SCRIPT.index('\n}\n', start)]


class Startup(unittest.TestCase):
    def test_the_live_stream_can_open_a_screen_before_the_catalogue(self):
        # The race exists: the light payload selects the first screen, and it carries no entities.
        self.assertIn('if (!selected && inventory.screens.length) select(inventory.screens[0].id);', function('applyLive'))
        self.assertRegex(function('select'), r'renderTopbar\(\);\s+renderTiles\(\);\s+renderResults\(\);')

    def test_the_first_full_inventory_draws_what_needs_entities_again(self):
        body = function('refresh')
        first = body.index('const firstCatalogue = full && !inventory.entities?.length;')
        self.assertLess(first, body.index('inventory = full ? data'), 'decided before the catalogue is replaced')
        redraw = re.search(r'if \(firstCatalogue && selected && layout && !drag\.active && !chipDrag\.active\) '
                           r'\{ renderTopbar\(\); renderTiles\(\); renderResults\(\); \}', body)
        self.assertTrue(redraw, 'tiles, top bar and entity list are drawn again, never in the middle of a drag')
        self.assertLess(redraw.start(), body.index('if (!selected && inventory.screens.length) select('))


if __name__ == '__main__':
    unittest.main()
