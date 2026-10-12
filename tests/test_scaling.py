from manager_fixtures import edit_layout
"""Daily efficiency with many screens: incremental sync, one action per message, the keepalive ping,
bundled history in the background and the cached screen list (app 0.2.39 / firmware 0.2.33)."""
from manager_fixtures import with_screen_grid
import asyncio
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import time
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'screen_manager/app'))
from core import TRANSPORT_MIN_FIRMWARE, encode, message_action, packets, revision

HAS_AIOHTTP = importlib.util.find_spec('aiohttp') is not None
if HAS_AIOHTTP:
    import server
    from server import FULL_REPEAT_SECONDS, HISTORY_SECONDS, KEEPALIVE_SECONDS, HomeAssistant, Manager, samples


class ProtocolTests(unittest.TestCase):
    def test_encode_revision_and_action(self):
        message = {'v': 1, 'op': 'layout', 'title': 'Office 1', 'entities': ['light.a']}
        self.assertEqual(json.loads(encode(message)), message)
        self.assertNotIn(', ', encode(message), 'compact separators')
        with self.assertRaises(ValueError):
            encode({'x': 'y' * 4096})
        # The chunks carry exactly the encoded message.
        import base64
        self.assertEqual(base64.b64decode(''.join(p.split('|')[3] for p in packets(message))).decode(), encode(message))
        # Same content, any key order: same revision; any change: another one.
        self.assertEqual(revision(message), revision({'entities': ['light.a'], 'title': 'Office 1', 'op': 'layout', 'v': 1}))
        self.assertNotEqual(revision(message), revision({**message, 'title': 'Office 2'}))
        self.assertEqual(len(revision(message)), 12)
        self.assertEqual(message_action('office-1'), 'esphome.office_1_screen_message')
        self.assertIsNone(message_action(None))
        self.assertEqual(TRANSPORT_MIN_FIRMWARE, (0, 2, 33))

    def test_samples_take_the_last_value_per_bucket(self):
        events = [(100, 1.0), (50, 2.0), (1000, 3.0)]
        out = samples(events, 0, 2400)
        self.assertEqual(out[0], 1.0, 'both early events fall in the first bucket; the later one wins')
        self.assertEqual(out[8], 1.0)
        self.assertEqual(out[9], 3.0)
        self.assertEqual(out[23], 3.0, 'carried forward')
        self.assertEqual(samples([], 0, 2400), [None] * 24)


def fake_ha(firmware='0.2.33', node='office-1'):
    class HA:
        online = True

        def __init__(self):
            self.registry = [{'entity_id': 'text.screen', 'platform': 'esphome', 'original_name': 'Tile settings', 'device_id': 'd1'},
                             {'entity_id': 'sensor.fw', 'platform': 'esphome', 'original_name': 'Screen firmware', 'device_id': 'd1'},
                             {'entity_id': 'text.node', 'platform': 'esphome', 'original_name': 'Device name', 'device_id': 'd1'},
                             {'entity_id': 'light.a', 'platform': 'hue'}, {'entity_id': 'light.b', 'platform': 'hue'},
                             {'entity_id': 'sensor.t', 'platform': 'mqtt'}, {'entity_id': 'sensor.co2', 'platform': 'mqtt'}]
            self.devices, self.areas = [{'id': 'd1', 'name': 'Office 1'}], []
            self.states = {'text.screen': {'state': 'Synced'}, 'sensor.fw': {'state': firmware}, 'text.node': {'state': node},
                           'light.a': {'state': 'on', 'attributes': {'friendly_name': 'Lamp A'}},
                           'light.b': {'state': 'off', 'attributes': {'friendly_name': 'Lamp B'}},
                           'sensor.t': {'state': '21.5', 'attributes': {'unit_of_measurement': '°C', 'friendly_name': 'Temperature'}},
                           'sensor.co2': {'state': '600', 'attributes': {'unit_of_measurement': 'ppm', 'friendly_name': 'CO2'}}}
            self.changed = asyncio.Event()
            self.dirty = set()
            self.relevant = None
            self.messages = []
            self.stat_calls, self.history_calls = [], []
            self.stats = {}

        async def send(self, inbox, message, action=None):
            self.messages.append((inbox, message, action))

        async def statistics(self, entities, hours):
            self.stat_calls.append((sorted(entities), hours))
            return {e: self.stats[e] for e in entities if e in self.stats}

        async def history(self, entity, hours):
            self.history_calls.append((entity, hours))
            return [1.0] * 24
    return HA()


@unittest.skipUnless(HAS_AIOHTTP, 'Run using .venv-portal/bin/python for server tests')
class IncrementalSync(unittest.IsolatedAsyncioTestCase):
    def manager(self, tmp, **kw):
        m = Manager(with_screen_grid(fake_ha(**kw)), Path(tmp) / 'screens.json')
        m.save('text.screen', {'title': 'Office 1', 'tiles': [{'entity': 'light.a', 'name': ''}, {'entity': 'light.b', 'name': ''}],
                               'header': {'items': [{'type': 'clock'}, {'type': 'entity', 'entity': 'sensor.t', 'content': 'state', 'icon': 'auto', 'show': 'always'}]}})
        return m

    async def test_only_dirty_tiles_are_rebuilt_and_sent(self):
        with tempfile.TemporaryDirectory() as tmp:
            m = self.manager(tmp)
            layout = m.layouts['text.screen']
            self.assertTrue(await m.sync_one('text.screen', layout, dirty=set()))
            ops = [msg['op'] for _, msg, _ in m.ha.messages]
            self.assertEqual(ops, ['layout', 'header', 'state', 'state'])
            self.assertIn('rev', m.ha.messages[0][1])
            self.assertTrue(all(action == 'esphome.office_1_screen_message' for _, _, action in m.ha.messages), 'firmware 0.2.33: one action per message')
            m.ha.messages.clear()
            # Both lamps change in HA, but only light.a is reported dirty: light.b's message is reused as sent.
            m.ha.states['light.a']['state'] = 'off'
            m.ha.states['light.b']['state'] = 'on'
            with patch.object(server, 'state_message', wraps=server.state_message) as built:
                self.assertTrue(await m.sync_one('text.screen', layout, dirty={'light.a'}))
                self.assertEqual(built.call_count, 1, 'one tile rebuilt')
            self.assertEqual([(msg['op'], msg.get('entity')) for _, msg, _ in m.ha.messages], [('state', 'light.a')])
            m.ha.messages.clear()
            # Nothing dirty: nothing rebuilt, nothing sent, and the top bar untouched.
            with patch.object(server.header_bar, 'message', wraps=server.header_bar.message) as bar:
                self.assertFalse(await m.sync_one('text.screen', layout, dirty=set()))
                self.assertEqual(bar.call_count, 0)
            self.assertEqual(m.ha.messages, [])
            # The top bar follows its own entity.
            m.ha.states['sensor.t']['state'] = '22.0'
            self.assertTrue(await m.sync_one('text.screen', layout, dirty={'sensor.t'}))
            self.assertEqual([msg['op'] for _, msg, _ in m.ha.messages], ['header'])
            m.ha.messages.clear()
            # None rebuilds everything and sends the differences (light.b was still stale).
            self.assertTrue(await m.sync_one('text.screen', layout, dirty=None))
            self.assertEqual([(msg['op'], msg.get('entity')) for _, msg, _ in m.ha.messages], [('state', 'light.b')])
            m.ha.messages.clear()
            # force repeats everything, as the hourly safety net and older firmware need.
            self.assertTrue(await m.sync_one('text.screen', layout, force=True, dirty=set()))
            self.assertEqual([msg['op'] for _, msg, _ in m.ha.messages], ['layout', 'header', 'state', 'state'])

    async def test_ping_and_old_firmware_transport(self):
        with tempfile.TemporaryDirectory() as tmp:
            m = self.manager(tmp)
            layout = m.layouts['text.screen']
            await m.sync_one('text.screen', layout, dirty=set())
            rev = m.ha.messages[0][1]['rev']
            m.ha.messages.clear()
            await m.ping('text.screen', m.screen('text.screen'))
            self.assertEqual(m.ha.messages, [('text.screen', {'v': 1, 'op': 'ping', 'rev': rev, 'keepalive': KEEPALIVE_SECONDS}, 'esphome.office_1_screen_message')])
            self.assertEqual(rev, revision({k: v for k, v in m.sent['text.screen']['layout'].items() if k != 'rev'}))
            # Firmware before 0.2.33: base64 chunks in the text inbox, no ping.
            old = self.manager(tmp, firmware='0.2.32')
            self.assertIsNone(old.transport('text.screen'))
            self.assertFalse(old.supports_ping('text.screen'))
            await old.sync_one('text.screen', old.layouts['text.screen'], dirty=set())
            self.assertTrue(all(action is None for _, _, action in old.ha.messages))
            old.ha.messages.clear()
            await old.ping('text.screen', old.screen('text.screen'))  # harmless: nothing recorded as sent by ping
            self.assertEqual([msg['op'] for _, msg, _ in old.ha.messages], ['ping'], 'ping itself is version-agnostic; run() decides')

    async def run_loop_for(self, m, seconds):
        task = asyncio.create_task(m.run())
        await asyncio.sleep(seconds)
        task.cancel()
        try:
            await task
        except asyncio.CancelledError:
            pass

    async def test_run_loop_pings_resends_and_repeats(self):
        with tempfile.TemporaryDirectory() as tmp:
            m = self.manager(tmp)
            m.ha.changed.set()
            await self.run_loop_for(m, 0.6)
            ops = [msg['op'] for _, msg, _ in m.ha.messages]
            self.assertEqual(ops, ['layout', 'header', 'state', 'state'], 'first pass sends everything')
            self.assertEqual(m.ha.relevant, {'light.a', 'light.b', 'sensor.t', 'text.screen', 'sensor.fw', 'text.node', 'sensor.test_grid_text_screen'})
            m.ha.messages.clear()
            # A wake without dirty entities sends nothing, not even a ping before its time.
            m.ha.changed.set()
            await self.run_loop_for(m, 0.6)
            self.assertEqual(m.ha.messages, [])
            # Two minutes later: only the ping.
            m.pinged['text.screen'] = time.monotonic() - KEEPALIVE_SECONDS
            m.ha.changed.set()
            await self.run_loop_for(m, 0.6)
            self.assertEqual([msg['op'] for _, msg, _ in m.ha.messages], ['ping'])
            m.ha.messages.clear()
            # The screen answered a ping with a mismatch (or restarted): everything again, once.
            m.ha.states['text.screen'] = {'state': 'Resend needed'}
            m.last['text.screen'] = time.monotonic() - 130
            m.ha.changed.set()
            await self.run_loop_for(m, 0.6)
            self.assertEqual([msg['op'] for _, msg, _ in m.ha.messages], ['layout', 'header', 'state', 'state'])
            m.ha.messages.clear()
            # The same answer right after a full send does not loop.
            m.ha.changed.set()
            await self.run_loop_for(m, 0.6)
            self.assertEqual(m.ha.messages, [])
            # The hourly safety net repeats everything.
            m.ha.states['text.screen'] = {'state': 'Synced'}
            m.last['text.screen'] = time.monotonic() - FULL_REPEAT_SECONDS
            m.ha.changed.set()
            await self.run_loop_for(m, 0.6)
            self.assertEqual([msg['op'] for _, msg, _ in m.ha.messages], ['layout', 'header', 'state', 'state'])
            m.ha.messages.clear()
            # A registry refresh rebuilds everything without differences: the ping is not postponed.
            m.ha.messages.clear()
            m.pinged['text.screen'] = time.monotonic() - KEEPALIVE_SECONDS
            m.ha.registry = list(m.ha.registry)
            m.ha.changed.set()
            await self.run_loop_for(m, 0.6)
            self.assertEqual([msg['op'] for _, msg, _ in m.ha.messages], ['ping'])
            m.ha.messages.clear()
            # A dirty state change sends that tile only.
            m.ha.states['light.b']['state'] = 'on'
            m.ha.dirty.add('light.b')
            m.ha.changed.set()
            await self.run_loop_for(m, 0.6)
            self.assertEqual([(msg['op'], msg.get('entity')) for _, msg, _ in m.ha.messages], [('state', 'light.b')])

    async def test_old_firmware_keeps_the_full_keepalive(self):
        with tempfile.TemporaryDirectory() as tmp:
            m = self.manager(tmp, firmware='0.2.32')
            m.ha.changed.set()
            await self.run_loop_for(m, 0.6)
            m.ha.messages.clear()
            m.last['text.screen'] = time.monotonic() - KEEPALIVE_SECONDS
            m.ha.changed.set()
            await self.run_loop_for(m, 0.6)
            self.assertEqual([msg['op'] for _, msg, _ in m.ha.messages], ['layout', 'header', 'state', 'state'])
            self.assertTrue(all(action is None for _, _, action in m.ha.messages))


@unittest.skipUnless(HAS_AIOHTTP, 'Run using .venv-portal/bin/python for server tests')
class ScreenCache(unittest.IsolatedAsyncioTestCase):
    async def test_screens_are_rediscovered_only_when_their_inputs_change(self):
        with tempfile.TemporaryDirectory() as tmp:
            m = Manager(with_screen_grid(fake_ha()), Path(tmp) / 'screens.json')
            with patch.object(server, 'discover_screens', wraps=server.discover_screens) as found:
                first = m.screens()
                self.assertEqual([s['id'] for s in first], ['text.screen'])
                first[0]['layout'] = 'mutated by a caller'
                self.assertNotIn('layout', m.screens()[0], 'callers get copies')
                m.ha.states['light.a']['state'] = 'off'  # not a screen diagnostic
                m.screens()
                self.assertEqual(found.call_count, 1)
                m.ha.states['sensor.fw'] = {'state': '0.2.34'}
                self.assertEqual(m.screens()[0]['firmware'], '0.2.34')
                self.assertEqual(found.call_count, 2)
                m.ha.registry = list(m.ha.registry)  # a new registry object from HA
                m.screens()
                self.assertEqual(found.call_count, 3)
                self.assertEqual(found.call_args[0][0], m.screen_registry(), 'only the screens\' own entries are walked')
            # The watched set follows layouts and registry, not every call.
            with patch.object(server, 'header_items', wraps=server.header_items) as items:
                m.watched_entities(); m.watched_entities()
                self.assertEqual(items.call_count, 0, 'no layouts yet: nothing to walk')
                m.save('text.screen', {'title': 'K', 'tiles': [{'entity': 'light.a', 'name': ''}]})
                self.assertIn('light.a', m.watched_entities())
                self.assertIn('text.screen', m.watched_entities())
                calls = items.call_count
                m.watched_entities()
                self.assertEqual(items.call_count, calls)


@unittest.skipUnless(HAS_AIOHTTP, 'Run using .venv-portal/bin/python for server tests')
class HistoryInTheBackground(unittest.IsolatedAsyncioTestCase):
    async def test_statistics_bundle_rest_fallback_and_dirty_marking(self):
        with tempfile.TemporaryDirectory() as tmp:
            m = Manager(with_screen_grid(fake_ha()), Path(tmp) / 'screens.json')
            m.ha.stats = {'sensor.t': [21.0] * 24}
            m.save('text.screen', {'title': 'K', 'tiles': [{'entity': 'sensor.t', 'name': '', 'options': {'history_hours': 24}},
                                                            {'entity': 'sensor.co2', 'name': '', 'options': {'history_hours': 6}},
                                                            {'entity': 'light.a', 'name': ''}]})
            self.assertTrue(m.history_wake.is_set(), 'a save wakes the history task')
            # The sync loop never waits for history: the first state message goes out without it.
            await m.sync_one('text.screen', m.layouts['text.screen'], dirty=set())
            self.assertNotIn('history', m.ha.messages[1][1])
            m.ha.messages.clear()
            await m.refresh_histories()
            self.assertEqual(m.ha.stat_calls, [(['sensor.co2'], 6), (['sensor.t'], 24)], 'one statistics request per window')
            self.assertEqual(m.ha.history_calls, [('sensor.co2', 6)], 'REST only for the sensor without statistics')
            self.assertEqual(m.ha.dirty, {'sensor.t', 'sensor.co2'})
            self.assertTrue(m.ha.changed.is_set())
            # The next pass rebuilds just those tiles, now with history.
            dirty, m.ha.dirty = m.ha.dirty, set()
            await m.sync_one('text.screen', m.layouts['text.screen'], dirty=dirty)
            sent = {msg['entity']: msg for _, msg, _ in m.ha.messages}
            self.assertEqual(sent['sensor.t']['history'], {'hours': 24, 'values': [21.0] * 24})
            self.assertEqual(sent['sensor.co2']['history'], {'hours': 6, 'values': [1.0] * 24})
            self.assertNotIn('light.a', sent)
            # Fresh entries are not fetched again; unchanged values mark nothing dirty.
            m.ha.stat_calls.clear(); m.ha.history_calls.clear()
            await m.refresh_histories()
            self.assertEqual((m.ha.stat_calls, m.ha.history_calls, m.ha.dirty), ([], [], set()))
            for key in m.histories:
                m.histories[key] = (time.monotonic() - HISTORY_SECONDS, m.histories[key][1])
            await m.refresh_histories()
            self.assertEqual(len(m.ha.stat_calls), 2)
            self.assertEqual(m.ha.dirty, set(), 'same values: no resend')
            # A tile that left the layout drops out of the cache.
            edit_layout(m, 'text.screen', {'title': 'K', 'tiles': [{'entity': 'sensor.t', 'name': ''}]})
            await m.refresh_histories()
            self.assertEqual(set(m.histories), {('sensor.t', 24)})

    async def test_statistics_request_and_bucketing(self):
        ha = HomeAssistant(None, 'http://ha/api', 'token')
        now = time.time()
        calls = []
        async def request(kind, **data):
            calls.append((kind, data))
            # Hourly rows in milliseconds (HA 2023.9+); the newest hour has no row yet.
            return {'sensor.t': [{'start': (now - 24 * 3600 + h * 3600) * 1000, 'mean': 20 + h} for h in range(23)],
                    'sensor.e': [{'start': (now - 3600) * 1000, 'state': 12.5, 'sum': 100}],
                    'sensor.other': [{'start': 1, 'mean': 1}]}
        ha.request = request
        found = await ha.statistics({'sensor.t', 'sensor.e', 'sensor.none'}, 24)
        self.assertEqual(calls[0][0], 'recorder/statistics_during_period')
        self.assertEqual(calls[0][1]['period'], 'hour')
        self.assertEqual(calls[0][1]['statistic_ids'], ['sensor.e', 'sensor.none', 'sensor.t'])
        self.assertEqual(calls[0][1]['types'], ['mean', 'state'])
        self.assertEqual(set(found), {'sensor.t', 'sensor.e'}, 'no statistics: absent, so the REST history takes over')
        # A row that starts a hair before a bucket's end is that bucket's last value.
        self.assertEqual(found['sensor.t'][:3], [21, 22, 23])
        self.assertEqual(found['sensor.t'][23], 42, 'the running hour carries the previous mean forward')
        self.assertEqual(found['sensor.e'][23], 12.5, 'sum sensors give their state')
        self.assertIsNone(found['sensor.e'][0])
        await ha.statistics({'sensor.t'}, 6)
        self.assertEqual(calls[1][1]['period'], '5minute')


if __name__ == '__main__':
    unittest.main()
