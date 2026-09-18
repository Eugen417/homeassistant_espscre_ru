"""Opening ESP Screens.

App 0.2.58: the page asks for the full inventory and opens the live stream at the same time. An add-on that is
building firmware answers the stream first, whose light payload has screens but no entities. The page keeps the
catalogue it has on a light poll and every name on the page follows the inventory, so the first full inventory
fills in what was missing.

App 0.2.65: the page no longer opens the first screen by itself; the owner picks one. Until then a card asks for
that, or offers the install when there are no screens yet.

App 0.2.74: the page is the Vue app in web/; these tests read its source (tests/editor_sources.py).
"""
import re
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import editor_sources  # noqa: E402

SCRIPT = editor_sources.SCRIPT
STORE = editor_sources.source('store.ts')


class Startup(unittest.TestCase):
    def test_no_screen_opens_by_itself(self):
        self.assertNotIn('inventory.screens[0]', SCRIPT)
        # The one way into a screen is its button in the list.
        self.assertEqual(re.findall(r'(?<![\w.])(?<!function )select\(([^)]*)\)', SCRIPT), ['screen.id'])
        self.assertIn('@click="select(screen.id)"', editor_sources.component('Sidebar'))
        for name in ('refresh', 'applyLive'):
            body = STORE[STORE.index(f'function {name}('):]
            self.assertNotRegex(body[:body.index('\n}\n')], r'(?<![\w.])select\(', name)

    def test_the_right_side_asks_for_a_screen_until_one_is_chosen(self):
        empty = editor_sources.component('EmptyState')
        choose = re.search(r'<section v-if="state.inventory.screens.length" id="choose" class="empty">(.*?)</section>', empty, re.S)
        self.assertTrue(choose, 'the card asks for a screen while there are screens')
        self.assertIn('<h2>Choose a screen</h2>', choose[1])
        self.assertNotIn('<button', choose[1], 'the list beside it is the choice')
        self.assertIn('id="empty" class="empty"', empty, 'the install card when there are no screens yet')
        self.assertIn('id="start"', empty)
        app = editor_sources.source('App.vue')
        self.assertIn('currentScreen.value && state.layout ? ScreenView : EmptyState', app)

    def test_a_light_poll_keeps_the_catalogue_and_names_follow_the_inventory(self):
        self.assertIn('state.inventory = full ? data : { ...state.inventory, ...data };', STORE)
        self.assertIn('inventory?light=1', STORE)
        # Names come from the inventory at render time, so they appear as soon as the full inventory does.
        self.assertIn('state.inventory.entities.find((e) => e.id === id)?.name', STORE)
        self.assertIn('entityName(props.tile.entity)', editor_sources.component('TileCard'))


if __name__ == '__main__':
    unittest.main()
