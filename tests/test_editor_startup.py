"""Opening ESP Screens.

App 0.2.58: the page asks for the full inventory and opens the live stream at the same time. An add-on that is
building firmware answers the stream first, whose light payload has screens but no entities, so a screen opened
then had an empty catalogue: the entity list said "No entities found" and the tiles showed entity ids until a
filter chip was pressed. The first full inventory now draws those parts again.

App 0.2.65: the page no longer opens the first screen by itself; the owner picks one. Until then a card asks for
that, or offers the install when there are no screens yet. A screen picked before the full inventory arrives
still gets its entity names from the redraw above.
"""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = (ROOT / 'screen_manager/app/static/app.js').read_text()
PAGE = (ROOT / 'screen_manager/app/static/index.html').read_text()


def function(name):
    start = SCRIPT.index(f'function {name}(')
    return SCRIPT[start:SCRIPT.index('\n}\n', start)]


class Startup(unittest.TestCase):
    def test_no_screen_opens_by_itself(self):
        self.assertNotIn('inventory.screens[0]', SCRIPT)
        # The one way into a screen is its button in the list.
        self.assertEqual(re.findall(r'(?<![\w.])(?<!function )select\(([^)]*)\)', SCRIPT), ['screen.id'])
        for name in ('refresh', 'applyLive'):
            self.assertNotRegex(function(name), r'(?<![\w.])select\(', name)

    def test_the_right_side_asks_for_a_screen_until_one_is_chosen(self):
        choose = re.search(r'<section id="choose" class="empty" hidden>(.*?)</section>', PAGE, re.S)
        self.assertTrue(choose, 'hidden until the first inventory')
        self.assertIn('<h2>Choose a screen</h2>', choose[1])
        self.assertNotIn('<button', choose[1], 'the list beside it is the choice')
        self.assertIn('<section id="empty" class="empty" hidden>', PAGE, 'no install card flashing while the page loads')
        self.assertIn('<section id="editor" hidden>', PAGE)
        body = function('renderScreens')
        self.assertIn('if (!selected) {\n    $("#choose").hidden = !inventory.screens.length;\n'
                      '    $("#empty").hidden = !!inventory.screens.length;\n  }', body)
        self.assertLess(body.index('$("#choose").hidden'), body.index('if ($("#screens .screen-host"))'),
                        'also while an address form keeps the list')
        opened = function('select')
        for part in ('$("#editor").hidden = false;', '$("#empty").hidden = true;', '$("#choose").hidden = true;'):
            self.assertIn(part, opened)

    def test_the_first_full_inventory_draws_what_needs_entities_again(self):
        body = function('refresh')
        first = body.index('const firstCatalogue = full && !inventory.entities?.length;')
        self.assertLess(first, body.index('inventory = full ? data'), 'decided before the catalogue is replaced')
        redraw = re.search(r'if \(firstCatalogue && selected && layout && !drag\.active && !chipDrag\.active\) '
                           r'\{ renderTopbar\(\); renderTiles\(\); renderResults\(\); \}', body)
        self.assertTrue(redraw, 'tiles, top bar and entity list are drawn again, never in the middle of a drag')
        self.assertRegex(function('select'), r'renderTopbar\(\);\s+renderTiles\(\);\s+renderResults\(\);')


if __name__ == '__main__':
    unittest.main()
