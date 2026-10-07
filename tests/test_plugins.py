"""Plugins (docs/PLUGINS.md): the manifest check, the map language, fetch's rules, the plugins file, a plugin tile in a
layout, and one plugin API version in the firmware, the component and the add-on."""
import asyncio
import json
import re
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'screen_manager' / 'app'))

import core  # noqa: E402
import page_layout  # noqa: E402
import plugin_fetch  # noqa: E402
import plugin_manifest as pm  # noqa: E402
from firmware import Firmware  # noqa: E402
from page_delivery import plugins_of  # noqa: E402

ENGLISH = {'app': {'name': 'Bus', 'summary': 'Next bus.', 'tile': 'Next', 'stop': 'Stop', 'line': 'Line',
                   'walk': 'Walk', 'key': 'Key'}, 'screen': {'now': 'now'}}


def manifest(**changes):
    data = {
        'id': 'bus', 'version': '1.0.0', 'api': '0.1', 'icon': 'bus', 'maintainer': 'someone', 'license': 'MIT',
        'permissions': {'network': ['api.example.org']}, 'attributes': ['cloud'], 'privacy': 'https://example.org/p',
        'tiles': [{'id': 'next', 'name': 'tile', 'sizes': {'min': '1x1', 'max': '2x2'}, 'memory': 900,
                   'data': 'departures', 'options': [
                       {'id': 'stop', 'kind': 'text', 'label': 'stop'},
                       {'id': 'line', 'kind': 'choice', 'options_from': 'lines', 'label': 'line'},
                       {'id': 'walk', 'kind': 'number', 'min': 0, 'max': 20, 'default': 3, 'label': 'walk'}]}],
        'fetch': [
            {'id': 'departures', 'url': 'https://api.example.org/stops/{stop}', 'every': '60s',
             'map': {'items': '$.{stop}.passes[*]', 'fields': {'line': 'line', 'at': {'path': 'when', 'as': 'epoch'}},
                     'where': {'line': '{line}'}, 'sort': 'at', 'limit': 2}},
            {'id': 'lines', 'url': 'https://api.example.org/stops/{stop}', 'every': '1h',
             'map': {'items': '$.{stop}.passes[*]', 'value': 'line', 'label': ['line', 'to']}}],
    }
    data.update(changes)
    return data


class Manifest(unittest.TestCase):
    def test_a_complete_manifest_passes(self):
        out = pm.check(manifest(), ENGLISH)
        self.assertEqual(out['tiles'][0]['options'][2]['default'], 3)
        self.assertEqual(out['fetch'][0]['every'], 60)
        self.assertEqual(out['fetch'][0]['placeholders'], ['stop'])

    def test_the_real_plugins_pass(self):
        plugins = ROOT.parent / 'tessera-plugins'
        for folder in [*(plugins / 'plugins').glob('*'), plugins / 'template']:
            if (folder / 'tessera-plugin.yaml').is_file():
                import yaml
                with self.subTest(folder.name):
                    pm.check(yaml.safe_load((folder / 'tessera-plugin.yaml').read_text()),
                             json.loads((folder / 'translations' / 'en.json').read_text()))

    def test_a_new_kind_of_right_does_not_ask_everyone_again(self):
        import hashlib
        out = pm.check(manifest(), ENGLISH)
        # The fingerprint as plugin API 0.1 made it, before ha_commands existed: an installed plugin keeps its consent.
        before = {'permissions': {'read_entities': [], 'home_assistant_actions': [], 'network': ['api.example.org']},
                  'attributes': ['cloud']}
        self.assertEqual(pm.permission_hash(out), hashlib.sha256(json.dumps(before, sort_keys=True).encode()).hexdigest()[:16])
        asking = pm.check(manifest(permissions={'network': ['api.example.org'], 'ha_commands': ['history/history_during_period']}), ENGLISH)
        self.assertNotEqual(pm.permission_hash(asking), pm.permission_hash(out))

    def test_a_preview_names_fields_of_its_data(self):
        def with_preview(preview):
            data = manifest()
            data['tiles'][0]['preview'] = preview
            return data
        self.assertEqual(pm.check(with_preview({'badge': '{line}', 'countdown': 'at'}), ENGLISH)['tiles'][0]['preview'],
                         {'badge': '{line}', 'countdown': 'at'})
        for wrong in ({'title': '{nowhere}'}, {'countdown': 'line'}, {'value': '{line}', 'countdown': 'at'}, {'colour': 'x'}):
            with self.subTest(wrong), self.assertRaises(pm.ManifestError):
                pm.check(with_preview(wrong), ENGLISH)

    def test_mistakes_say_where(self):
        cases = [
            (manifest(colour='red'), 'unknown field'),
            (manifest(api='1'), 'api'),
            (manifest(id='Bus'), 'id'),
            (manifest(license='Proprietary'), 'license'),
            (manifest(privacy=None, attributes=['cloud']), 'privacy'),
            (manifest(attributes=[]), 'cloud'),
            (manifest(permissions={'network': ['192.168.1.2']}), 'public host'),
            (manifest(permissions={'network': ['router.local']}), 'public host'),
        ]
        bad_fetch = manifest()
        bad_fetch['fetch'][0]['url'] = 'https://other.example.org/x'
        cases.append((bad_fetch, 'permissions.network'))
        often = manifest()
        often['fetch'][0]['every'] = '5s'
        cases.append((often, '30s'))
        secret_in_path = manifest(inputs=[{'id': 'key', 'kind': 'secret', 'label': 'key'}])
        secret_in_path['fetch'][0]['url'] = 'https://api.example.org/{key}/stops/{stop}'
        cases.append((secret_in_path, 'never in the path'))
        secret_over_http = manifest(inputs=[{'id': 'key', 'kind': 'secret', 'label': 'key'}])
        secret_over_http['fetch'][0]['url'] = 'http://api.example.org/stops/{stop}?key={key}'
        cases.append((secret_over_http, 'https'))
        unknown = manifest()
        unknown['fetch'][0]['url'] = 'https://api.example.org/stops/{nope}'
        cases.append((unknown, '{nope}'))
        for data, words in cases:
            with self.subTest(words):
                with self.assertRaises(pm.ManifestError) as caught:
                    pm.check(data, ENGLISH)
                self.assertIn(words, str(caught.exception))

    def test_every_text_is_in_english(self):
        english = {'app': dict(ENGLISH['app']), 'screen': {}}
        del english['app']['walk']
        with self.assertRaises(pm.ManifestError) as caught:
            pm.check(manifest(), english)
        self.assertIn('walk', str(caught.exception))

    def test_api_versions(self):
        self.assertTrue(pm.api_fits('0.1', (0, 1)))
        self.assertTrue(pm.api_fits('0.1', (0, 2)))    # something new raises the minor and breaks nothing
        self.assertFalse(pm.api_fits('0.3', (0, 2)))   # a plugin that needs a newer core says so
        self.assertTrue(pm.api_fits('1.1', (1, 3)))
        self.assertFalse(pm.api_fits('1.4', (1, 3)))
        self.assertFalse(pm.api_fits('2.0', (1, 3)))

    def test_one_plugin_api_everywhere(self):
        header = (ROOT / 'components/smart_display/plugin_api.h').read_text()
        major, minor = re.search(r'PLUGIN_API_MAJOR = (\d+), PLUGIN_API_MINOR = (\d+)', header).groups()
        component = (ROOT / 'components/smart_display/__init__.py').read_text()
        self.assertIn(f'PLUGIN_API = ({major}, {minor})', component)
        self.assertEqual(pm.PLUGIN_API, (int(major), int(minor)))

    def test_placeholder_price_is_the_screens(self):
        host = (ROOT / 'components/smart_display/plugin_host.h').read_text()
        self.assertIn(f'PLACEHOLDER_BYTES = {core.PLUGIN_PLACEHOLDER_BYTES};', host)


class Map(unittest.TestCase):
    ANSWER = {'30003025': {'passes': {
        'b': {'line': '7', 'to': 'Slotermeer', 'when': '2026-10-07T17:30:00', 'state': 'DRIVING'},
        'a': {'line': '15', 'to': 'Sloterdijk', 'when': '2026-10-07T17:20:00+02:00', 'state': 'PASSED'},
        'c': {'line': '15', 'to': 'Sloterdijk', 'when': '2026-10-07T17:25:00', 'state': 'PLANNED'}}}}

    def spec(self, **extra):
        return pm.check_map({'items': '$.{stop}.passes[*]', 'fields': {
            'line': 'line', 'at': {'path': 'when', 'as': 'epoch', 'tz': 'Europe/Amsterdam'}, 'state': 'state'}, **extra},
            'map', False)

    def test_items_of_an_object_sorted_filtered_and_cut(self):
        out = plugin_fetch.apply_map(self.spec(skip={'state': ['PASSED']}, sort='at', limit=1), self.ANSWER,
                                     {'stop': '30003025'})
        self.assertEqual([i['line'] for i in out['items']], ['15'])
        self.assertEqual(out['items'][0]['at'], 1791386700)    # 17:25 in Amsterdam (CEST), 15:25 UTC

    def test_where_with_an_empty_option_keeps_everything(self):
        spec = self.spec(where={'line': '{line}'}, sort='at')
        everything = plugin_fetch.apply_map(spec, self.ANSWER, {'stop': '30003025', 'line': ''})
        only = plugin_fetch.apply_map(spec, self.ANSWER, {'stop': '30003025', 'line': '7'})
        self.assertEqual(len(everything['items']), 3)
        self.assertEqual([i['line'] for i in only['items']], ['7'])

    def test_choices_once_each(self):
        spec = pm.check_map({'items': '$.{stop}.passes[*]', 'value': 'line', 'label': ['line', 'to']}, 'map', True)
        self.assertEqual(plugin_fetch.apply_choices(spec, self.ANSWER, {'stop': '30003025'}),
                         [{'value': '7', 'label': '7 · Slotermeer'}, {'value': '15', 'label': '15 · Sloterdijk'}])

    def test_paths(self):
        self.assertEqual(pm.parse_path('$.a[*].b[:2][0]'), [('key', 'a'), ('all',), ('key', 'b'), ('first', 2), ('at', 0)])
        for bad in ('a.b', '$.a[?(@.x)]', '$..a'):
            with self.subTest(bad), self.assertRaises(ValueError):
                pm.parse_path(bad)

    def test_text_is_bounded(self):
        spec = pm.check_map({'fields': {'name': 'name'}}, 'map', False)
        self.assertEqual(len(plugin_fetch.apply_map(spec, {'name': 'é' * 100})['name'].encode()), 48)


class Fetch(unittest.TestCase):
    def test_fill_encodes_and_needs_every_value(self):
        fetch = pm.check(manifest(), ENGLISH)['fetch'][0]
        url, _ = plugin_fetch.fill(fetch, {'stop': 'a b/c'}, {})
        self.assertEqual(url, 'https://api.example.org/stops/a%20b%2Fc')
        with self.assertRaises(plugin_fetch.FetchRefused):
            plugin_fetch.fill(fetch, {'stop': ''}, {})

    def test_home_addresses_are_refused(self):
        for address in ('192.168.1.10', '10.0.0.1', '127.0.0.1', '::1', 'fe80::1', '169.254.1.1', '::ffff:192.168.1.1'):
            with self.subTest(address):
                self.assertTrue(plugin_fetch.blocked_address(address))
        self.assertFalse(plugin_fetch.blocked_address('195.201.197.190'))

    def test_a_failure_keeps_the_last_answer_and_waits(self):
        clock = [1000.0]
        fetcher = plugin_fetch.Fetcher(clock=lambda: clock[0])
        answers = [{'x': 1}, OSError('HTTP 503')]

        async def download(url, headers, network):
            answer = answers.pop(0)
            if isinstance(answer, Exception):
                raise answer
            return answer
        fetcher.download = download
        checked = pm.check(manifest(), ENGLISH)
        fetch = checked['fetch'][0]

        async def run():
            _, first = await fetcher.get('bus', checked, fetch, {'stop': '1'}, {})
            self.assertEqual(first['data'], {'x': 1})
            clock[0] += 30
            _, same = await fetcher.get('bus', checked, fetch, {'stop': '1'}, {})   # not due yet: no ask
            self.assertEqual(len(answers), 1)
            clock[0] += 60
            _, failed = await fetcher.get('bus', checked, fetch, {'stop': '1'}, {})
            self.assertTrue(failed['stale'])
            self.assertEqual(failed['data'], {'x': 1})
            self.assertEqual(failed['retry'], clock[0] + 60)
        asyncio.run(run())

    def test_secrets_are_redacted(self):
        self.assertEqual(plugin_fetch.redact('GET /x?key=abc123 failed', {'key': 'abc123'}), 'GET /x?key=*** failed')


class Layout(unittest.TestCase):
    DOC = {'title': 'T', 'homePageId': 'aaaaaaaaaaaaaaaa', 'pages': [{
        'id': 'aaaaaaaaaaaaaaaa', 'navigation': {'excludeFromPagination': False},
        'topbar': {'leading': [], 'title': {'source': 'screen'}, 'trailing': []},
        'tiles': [{'id': 't1', 'content': {'kind': 'plugin', 'plugin': 'bus', 'tile': 'next',
                                            'options': {'stop': '30003025', 'walk': 3}},
                   'placement': {'row': 0, 'column': 0, 'columns': 2, 'rows': 1}, 'appearance': {'label': ''},
                   'interaction': {}}]}]}

    def test_a_plugin_tile_round_trips(self):
        page_layout.validate_document(json.loads(json.dumps(self.DOC)), core.DEFAULT_GRID)
        flat = page_layout.compile_tiles(self.DOC, core.DEFAULT_GRID)[0]
        self.assertEqual(flat['entity'], 'plugin:bus.next')
        self.assertEqual(flat['options']['plugin'], {'stop': '30003025', 'walk': 3})
        back = page_layout.tile_from_fields(flat, core.DEFAULT_GRID, ['aaaaaaaaaaaaaaaa'], id_factory=lambda: 't1')
        self.assertEqual(back['content'], self.DOC['pages'][0]['tiles'][0]['content'])

    def test_a_tile_of_an_entity_round_trips(self):
        doc = json.loads(json.dumps(self.DOC))
        doc['pages'][0]['tiles'][0]['content']['entityId'] = 'sensor.waste_next'
        page_layout.validate_document(json.loads(json.dumps(doc)), core.DEFAULT_GRID)
        flat = page_layout.compile_tiles(doc, core.DEFAULT_GRID)[0]
        self.assertEqual(flat['options']['plugin_entity'], 'sensor.waste_next')
        back = page_layout.tile_from_fields(flat, core.DEFAULT_GRID, ['aaaaaaaaaaaaaaaa'], id_factory=lambda: 't1')
        self.assertEqual(back['content'], doc['pages'][0]['tiles'][0]['content'])
        for wrong in ('screen.clock', 'not an entity'):
            with self.subTest(wrong), self.assertRaises(ValueError):
                core.validate_layout({'title': '', 'tiles': [{'entity': 'plugin:bus.next', 'name': '', 'slot': 0,
                                                              'options': {'plugin_entity': wrong}}]})

    def test_only_the_shape_is_checked(self):
        ok = {'title': '', 'tiles': [{'entity': 'plugin:bus.next', 'name': '', 'slot': 0,
                                      'options': {'plugin': {'a': 'x', 'b': 2, 'c': True}, 'tap': 'none'}}]}
        core.validate_layout(ok)
        for options in ({'plugin': {'a': {'nested': 1}}}, {'plugin': {'a': 'x' * 65}}, {'display': 'watch'},
                        {'plugin': {str(i): 1 for i in range(13)}}):
            with self.subTest(options), self.assertRaises(ValueError):
                core.validate_layout({'title': '', 'tiles': [{'entity': 'plugin:bus.next', 'name': '', 'slot': 0,
                                                              'options': options}]})
        self.assertFalse(core.entity_id('plugin:bus.next'))   # commands and events keep Home Assistant's ids

    def test_price(self):
        memory = {'psram': False, 'tile': 300, 'extra': 200}
        core.PLUGIN_MEMORY['plugin:bus.next'] = 900
        try:
            self.assertEqual(core.tile_cost({'entity': 'plugin:bus.next'}, memory), 1400)
            self.assertEqual(core.tile_cost({'entity': 'plugin:gone.tile'}, {**memory, 'psram': True}),
                             core.PLUGIN_PLACEHOLDER_BYTES)
        finally:
            core.PLUGIN_MEMORY.pop('plugin:bus.next')


class EntityTile(unittest.IsolatedAsyncioTestCase):
    async def test_state_name_and_named_attributes_only(self):
        import plugins as plugin_service
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)

        class FakeManager:
            ha = type('HA', (), {'states': {'sensor.waste_next': {'state': 'paper', 'attributes': {
                'friendly_name': 'Next collection', 'next_date': '2026-10-09', 'days': 2, 'secret': 'no'}}},
                'time_zone': 'Europe/Amsterdam', 'changed': asyncio.Event(), 'dirty': set()})()
            page_senders = {}
        service = plugin_service.Plugins(FakeManager(), Path(tmp.name) / 'data', Path(tmp.name) / 'esphome')
        kind = {'id': 'next', 'entity': ['sensor'], 'attributes': ['next_date', 'days']}
        entity, part = service.entity_part(kind, {'options': {'plugin_entity': 'sensor.waste_next'}})
        self.assertEqual(entity, 'sensor.waste_next')
        self.assertEqual(part['state'], 'paper')
        self.assertEqual(part['name'], 'Next collection')
        self.assertEqual(set(part['attributes']), {'next_date', 'days'})
        self.assertIsInstance(part['attributes']['next_date'], int)   # a date as seconds, for the screen's own words
        _, wrong = service.entity_part(kind, {'options': {'plugin_entity': 'light.kitchen'}})
        self.assertEqual(wrong, {'wait': 'wrong_entity'})


class Questions(unittest.IsolatedAsyncioTestCase):
    """A plugin asks Home Assistant something through the app: only what its manifest names, the answer bounded."""

    async def test_named_commands_only_and_bounded(self):
        import shutil
        import plugins as plugin_service
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        config = Path(tmp.name) / 'esphome'
        config.mkdir()
        shutil.copytree(ROOT / 'tests' / 'fixtures' / 'plugins' / 'calendar_peek', Path(tmp.name) / 'tessera-plugins' / 'calendar_peek')
        asked, sent = [], []

        class FakeHA:
            states, changed, dirty = {}, asyncio.Event(), set()

            async def request(self, kind, **data):
                asked.append((kind, data))
                return {'response': {'calendar.waste': {'events': [{'summary': 'Paper ' * 20, 'start': '2026-10-09'}] * 100}}}

        class FakeManager:
            ha = FakeHA()
            page_senders, aliases = {}, {}

            def screen(self, inbox):
                return {'id': inbox, 'node': inbox, 'name': 'Kitchen', 'board': 'guition', 'online': True}

            def transport(self, inbox, screen=None):
                return 'action'

            async def send_auxiliary(self, inbox, message, action, request):
                sent.append(message)
        service = plugin_service.Plugins(FakeManager(), Path(tmp.name) / 'data', config)
        service.scan_folders()
        service.store.put('kitchen', {'id': 'calendar_peek', 'source': 'folder', 'state': 'active'})
        body = {'re': 7, 'ask': 'call_service:calendar.get_events', 'data': {'entity_id': 'calendar.waste', 'duration': {'days': 28}}}
        await service.answer({'inbox': 'kitchen', 'plugin': 'calendar_peek', 'body': json.dumps(body)})
        self.assertEqual(asked[0][0], 'call_service')
        self.assertEqual(asked[0][1]['target'], {'entity_id': 'calendar.waste'})
        self.assertTrue(asked[0][1]['return_response'])
        reply = sent[0]['m']
        self.assertEqual((sent[0]['op'], sent[0]['p'], reply['re'], reply['ok']), ('plugin', 'calendar_peek', 7, True))
        self.assertLessEqual(len(json.dumps(reply)), 3400)
        await service.answer({'inbox': 'kitchen', 'plugin': 'calendar_peek', 'body': json.dumps({'re': 8, 'ask': 'get_states'})})
        self.assertEqual(sent[1]['m'], {'re': 8, 'ok': False, 'error': 'not_allowed'})
        self.assertEqual(len(asked), 1)
        # A plugin the screen does not run gets nothing at all.
        await service.answer({'inbox': 'kitchen', 'plugin': 'other', 'body': json.dumps(body)})
        self.assertEqual(len(sent), 2)

    async def test_settings_of_the_screens_own_entities_only(self):
        import shutil
        import plugins as plugin_service
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        config = Path(tmp.name) / 'esphome'
        config.mkdir()
        shutil.copytree(ROOT / 'tests' / 'fixtures' / 'plugins' / 'calendar_peek', Path(tmp.name) / 'tessera-plugins' / 'calendar_peek')
        calls = []

        class FakeHA:
            changed, dirty = asyncio.Event(), set()
            registry = [{'entity_id': 'switch.kitchen_peek_in_bar', 'device_id': 'dev1'},
                        {'entity_id': 'number.kitchen_peek_days', 'device_id': 'dev1'},
                        {'entity_id': 'switch.hall_peek_in_bar', 'device_id': 'dev2'}]
            states = {'switch.kitchen_peek_in_bar': {'state': 'on'},
                      'number.kitchen_peek_days': {'state': '2.0', 'attributes': {'min': 0, 'max': 3, 'step': 1}}}

            async def call_service(self, domain, service, data):
                calls.append((domain, service, data))

        class FakeManager:
            ha = FakeHA()
            page_senders, aliases = {}, {}

            def screen(self, inbox):
                return {'id': inbox, 'device_id': 'dev1', 'online': True}
        service = plugin_service.Plugins(FakeManager(), Path(tmp.name) / 'data', config)
        service.scan_folders()
        service.store.put('kitchen', {'id': 'calendar_peek', 'source': 'folder', 'state': 'active'})
        groups = service.settings_for('kitchen')
        rows = groups[0]['rows']
        self.assertEqual([(r['entity'], r['kind'], r['value']) for r in rows],
                         [('switch.kitchen_peek_in_bar', 'switch', True), ('number.kitchen_peek_days', 'number', 2.0)])
        await service.set_setting('kitchen', 'number.kitchen_peek_days', 3)
        self.assertEqual(calls, [('number', 'set_value', {'entity_id': 'number.kitchen_peek_days', 'value': 3})])
        for entity, value in (('switch.hall_peek_in_bar', True), ('light.kitchen', True), ('switch.kitchen_peek_in_bar', 'yes')):
            with self.subTest(entity), self.assertRaises(ValueError):
                await service.set_setting('kitchen', entity, value)

    def test_the_manifest_refuses_commands_that_reach_home_assistant_itself(self):
        for wrong in ('call_service', 'config/entity_registry/update', 'auth/sign_path', 'fire_event', 'supervisor/api'):
            with self.subTest(wrong), self.assertRaises(pm.ManifestError):
                pm.check(manifest(permissions={'network': ['api.example.org'], 'ha_commands': [wrong]}), ENGLISH)
        pm.check(manifest(permissions={'network': ['api.example.org'], 'ha_commands': ['history/history_during_period']}), ENGLISH)


class Hello(unittest.TestCase):
    def test_plugins_of_a_hello(self):
        self.assertEqual(plugins_of({}), (None, []))
        api, plugins = plugins_of({'plugin_api': '0.1', 'plugins': [
            {'id': 'bus', 'version': '1.0.0', 'tiles': ['next', 'Bad!'], 'taps': ['schedule']}, {'id': 'Nope'}, 'junk']})
        self.assertEqual(api, '0.1')
        self.assertEqual(plugins, [{'id': 'bus', 'version': '1.0.0', 'tiles': ['next'], 'taps': ['schedule']}])


class Sidecar(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        root = Path(self.tmp.name) / 'esphome'
        root.mkdir()
        (root / 'kitchen.yaml').write_text('esphome:\n  name: kitchen\npackages:\n  display:\n    url: x\n'
                                           '  local_overrides: !include kitchen.local.yaml\napi:\n')
        self.firmware = Firmware(root, Path(self.tmp.name) / 'data')

    def tearDown(self):
        self.tmp.cleanup()

    def test_written_then_attached_once_and_hidden(self):
        content = '# Written by Tessera.\npackages:\n  plugin_bus: !include ../tessera-plugins/bus/plugin.yaml\n'
        self.firmware.save_plugins('kitchen.yaml', content)
        self.firmware.save_plugins('kitchen.yaml', content)
        text = (self.firmware.root / 'kitchen.yaml').read_text()
        self.assertEqual(text.count('tessera_plugins: !include kitchen.plugins.yaml'), 1)
        self.assertLess(text.index('tessera_plugins'), text.index('api:'))
        self.assertEqual([p['file'] for p in self.firmware.profiles()], ['kitchen.yaml'])
        self.assertTrue(self.firmware.plugins_file('kitchen.yaml')['attached'])

    def test_only_packages_and_components(self):
        with self.assertRaises(ValueError):
            self.firmware.save_plugins('kitchen.yaml', 'wifi:\n  ssid: x\n')
        self.assertFalse((self.firmware.root / 'kitchen.plugins.yaml').exists())


class Queue(unittest.IsolatedAsyncioTestCase):
    """Ticking several screens builds them one after the other, never two at once and never a refusal."""

    async def test_screens_build_one_after_the_other(self):
        import shutil
        import plugins as plugin_service
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        config = Path(tmp.name) / 'esphome'
        config.mkdir()
        shutil.copytree(ROOT / 'tests' / 'fixtures' / 'plugins' / 'clock_words', Path(tmp.name) / 'tessera-plugins' / 'clock_words')
        started, running = [], []

        class FakeFirmware:
            task = None
            job = None

            def save_plugins(self, profile, text):
                (config / profile.replace('.yaml', '.plugins.yaml')).write_text(text)

            def start(self, data):
                assert not (self.task and not self.task.done()), 'two builds at once'
                started.append(data['file'])

                async def build():
                    running.append(data['file'])
                    await asyncio.sleep(0.01)
                    self.job = {'state': 'success'}
                self.task = asyncio.get_running_loop().create_task(build())

        class FakeManager:
            firmware = FakeFirmware()
            ha = type('HA', (), {'changed': asyncio.Event(), 'dirty': set()})()
            page_senders = {}

            def screen(self, inbox):
                return {'id': inbox, 'node': inbox, 'board': 'guition'}

            def notify(self):
                pass
        FakeManager.updates = type('U', (), {'resolve': lambda self, screen: (screen['id'] + '.yaml', '10.0.0.1')})()
        service = plugin_service.Plugins(FakeManager(), Path(tmp.name) / 'data', config)
        for inbox in ('one', 'two', 'three'):
            result = await service.apply(inbox, {'add': [{'id': 'clock_words', 'source': 'folder'}]})
            self.assertTrue(result['built'])
        await service.worker
        self.assertEqual(started, ['one.yaml', 'two.yaml', 'three.yaml'])
        self.assertEqual({i: service.store.get(i, 'clock_words')['state'] for i in ('one', 'two', 'three')},
                         {'one': 'active', 'two': 'active', 'three': 'active'})
        self.assertIn('../tessera-plugins/clock_words/plugin.yaml', (config / 'one.plugins.yaml').read_text())


if __name__ == '__main__':
    unittest.main()
