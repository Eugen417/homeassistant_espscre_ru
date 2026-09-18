"""Live values on the mockup, Identify, the test alert and the changelog for the Update badge (app 0.2.73).

The editor asks /api/states for the tiles it shows and draws Home Assistant's values on the mockup; Identify and
Try it call a screen's own show_alert action, with the same field rules as an alert event; the update summary
carries the CHANGELOG sections so the badge can say what a screen gets.
"""
import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'screen_manager/app'))
sys.path.insert(0, str(ROOT / 'tests'))
import changelog  # noqa: E402

HAS_AIOHTTP = importlib.util.find_spec('aiohttp') is not None

SAMPLE = '''## 0.2.74 (firmware 0.2.62)

A new editor.

- **One workspace.** Sidebar, pages, [library](docs/X.md) with `code`.
- Second line.

## 0.2.71 (firmware 0.2.60)

- **Slider stays.** Text.
'''


class Changelog(unittest.TestCase):
    def test_sections_carry_plain_bullet_lines(self):
        sections = changelog.parse(SAMPLE)
        self.assertEqual([(s['app'], s['firmware']) for s in sections], [('0.2.74', '0.2.62'), ('0.2.71', '0.2.60')])
        self.assertEqual(sections[0]['lines'], ['One workspace. Sidebar, pages, library with code.', 'Second line.'])
        self.assertEqual(changelog.parse(SAMPLE, limit=1)[0]['app'], '0.2.74')

    def test_the_real_changelog_is_found_and_shipped(self):
        sections = changelog.load()
        self.assertTrue(sections and sections[0]['lines'], 'CHANGELOG.md one folder up from the app')
        self.assertIn('COPY CHANGELOG.md /app/CHANGELOG.md', (ROOT / 'screen_manager/Dockerfile').read_text())


@unittest.skipUnless(HAS_AIOHTTP, 'Run using .venv-portal/bin/python for server tests')
class Endpoints(unittest.IsolatedAsyncioTestCase):
    async def asyncSetUp(self):
        import test_scaling
        from aiohttp.test_utils import TestClient, TestServer
        from server import Manager, create_app
        self.ha = test_scaling.fake_ha(firmware='0.2.60', node='office-1')
        self.ha.calls = []
        async def call(action, data):
            self.ha.calls.append((action, data))
        self.ha.call = call
        self.tmp = tempfile.TemporaryDirectory()
        self.manager = Manager(self.ha, Path(self.tmp.name) / 'screens.json')
        self.client = TestClient(TestServer(create_app(self.manager, True)))
        await self.client.start_server()
        inventory = await (await self.client.get('/api/inventory')).json()
        self.csrf = inventory['csrf']
        self.screen = inventory['screens'][0]
        self.headers = {'X-Screen-CSRF': self.csrf}

    async def asyncTearDown(self):
        await self.client.close()
        self.tmp.cleanup()

    async def test_states_give_the_value_word_and_attributes_per_entity(self):
        response = await self.client.get('/api/states?entity=light.a&entity=sensor.t&entity=light.nope&entity=screen.clock')
        states = (await response.json())['states']
        self.assertEqual(set(states), {'light.a', 'sensor.t'}, 'unknown entities and built-ins are left out')
        self.assertEqual(states['light.a']['state'], 'on')
        self.assertEqual(states['sensor.t']['state'], '21.5')
        self.assertEqual(states['sensor.t']['a']['unit_of_measurement'], '°C')
        self.assertIn('word', states['light.a'])

    async def test_identify_blinks_the_screen_through_its_alert_action(self):
        response = await self.client.post(f"/api/screens/{self.screen['id']}/identify", headers=self.headers)
        self.assertEqual(response.status, 200, await response.text())
        (action, data), = self.ha.calls
        self.assertEqual(action, 'esphome.office_1_show_alert')
        self.assertEqual((data['title'], data['flash'], data['timeout'], data['icon']), (f"This is {self.screen['name']}", True, 8, 'bell-ring'))
        missing = await self.client.post('/api/screens/text.nope/identify', headers=self.headers)
        self.assertEqual(missing.status, 400)

    async def test_the_test_alert_reaches_one_screen_or_all_with_the_event_rules(self):
        body = {'screen': self.screen['id'], 'data': {'title': 'Door', 'timeout': 'soon', 'flash': 'yes'}}
        response = await self.client.post('/api/alerts/test', headers=self.headers, json=body)
        result = await response.json()
        self.assertEqual((response.status, result['sent'], result['unusable']), (200, 1, ['timeout']), result)
        action, data = self.ha.calls[-1]
        self.assertEqual(action, 'esphome.office_1_show_alert')
        self.assertEqual((data['title'], data['timeout'], data['flash'], data['subtitle']), ('Door', 0, True, ''))
        everyone = await self.client.post('/api/alerts/test', headers=self.headers, json={'screen': 'all', 'data': {'title': 'Hi'}})
        self.assertEqual(everyone.status, 200, await everyone.text())
        self.assertEqual((await everyone.json())['sent'], 1)
        self.assertEqual(len(self.ha.calls), 2)
        bad = await self.client.post('/api/alerts/test', headers=self.headers, json={'screen': 'text.nope'})
        self.assertEqual(bad.status, 400)

    async def test_the_update_summary_carries_the_changelog(self):
        inventory = await (await self.client.get('/api/inventory?light=1')).json()
        sections = inventory['updates']['changelog']
        self.assertTrue(sections)
        self.assertEqual(set(sections[0]), {'app', 'firmware', 'lines'})


class Editor(unittest.TestCase):
    """The page side of the same features, read from the Vue sources."""
    def setUp(self):
        import editor_sources
        self.store = editor_sources.source('store.ts')
        self.page = editor_sources.PAGE

    def test_the_mockup_polls_live_values_and_draws_them(self):
        self.assertIn('getJson(`states?${query}`)', self.store)
        self.assertIn('if (!document.hidden && state.layout && state.tab === "layout" && route.value === "") loadStates();', self.store)
        for marker in ('liveOf(props.tile.entity)', ':class="{ lit: isOn }"', ':style="sliderStyle"', "class=\"tog\" :class=\"{ off: !on }\""):
            self.assertIn(marker, self.page, marker)

    def test_identify_and_the_test_alert_have_their_buttons(self):
        self.assertIn('send(`screens/${encodeURIComponent(screen.id)}/identify`, "POST")', self.store)
        self.assertIn('send("alerts/test", "POST", { screen: target, data })', self.store)
        self.assertIn('id="identify"', self.page)
        self.assertIn('id="alerts-try"', self.page)
        self.assertIn('id="try-send"', self.page)

    def test_layouts_can_be_copied_exported_and_imported(self):
        for name in ('export function copyLayoutFrom', 'export function exportLayout', 'export function importLayout'):
            self.assertIn(name, self.store)
        for marker in ('id="copy-layout"', 'id="export-layout"', 'id="import-layout"', 'accept="application/json,.json"'):
            self.assertIn(marker, self.page, marker)
        self.assertIn('.slice(0, tileLimit.value)', self.store, 'an imported layout never exceeds the firmware limit')

    def test_the_library_filters_by_room_and_placement_and_the_palette_exists(self):
        for marker in ('id="room"', 'id="hide-placed"', 'id="open-palette"', 'id="palette-input"', "e.key.toLowerCase() === \"k\""):
            self.assertIn(marker, self.page, marker)

    def test_updates_show_their_notes_and_progress(self):
        self.assertIn('export function whatsNew', self.store)
        self.assertIn('export function updateProgress', self.store)
        for marker in ('class="whatsnew"', 'role="progressbar"', "go('#firmware')"):
            self.assertIn(marker, self.page, marker)


if __name__ == '__main__':
    unittest.main()
