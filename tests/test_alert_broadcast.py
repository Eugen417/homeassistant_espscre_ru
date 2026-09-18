"""One alert for every screen (app 0.2.45): Home Assistant fires esp_screens_show_alert or
esp_screens_dismiss_alert, and the app calls that action on each screen that can show it."""
import asyncio
import importlib.util
from pathlib import Path
import re
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'screen_manager/app'))
sys.path.insert(0, str(ROOT / 'tests'))
from core import (ALERT_FIELDS, ALERT_MAX_TIMEOUT, BROADCAST_DISMISS, BROADCAST_EVENTS, BROADCAST_SHOW,  # noqa: E402
                  alert_data, alert_reference, alert_targets)

HAS_AIOHTTP = importlib.util.find_spec('aiohttp') is not None
if HAS_AIOHTTP:
    from aiohttp import WSMsgType
    from server import HomeAssistant, Manager

STATIC = ROOT / 'screen_manager/app/static'


class AlertData(unittest.TestCase):
    def test_complete_data_passes_through_typed(self):
        data = {'title': 'Mail!', 'subtitle': 'There is post', 'icon': 'mailbox', 'color': 'orange',
                'button_text': 'OK', 'timeout': 30, 'flash': True}
        self.assertEqual(alert_data(data), (data, []))
        self.assertEqual(list(alert_data(data)[0]), [name for name, *_ in ALERT_FIELDS], 'all seven, in field order')

    def test_missing_fields_become_empty_without_a_warning(self):
        empty = {'title': '', 'subtitle': '', 'icon': '', 'color': '', 'button_text': '', 'timeout': 0, 'flash': False}
        for data in ({}, None, 'text', {'title': None, 'timeout': '', 'flash': ''}):
            self.assertEqual(alert_data(data), (empty, []), data)

    def test_values_are_coerced_like_home_assistant_does(self):
        service, unusable = alert_data({'title': 42, 'timeout': '90', 'flash': 'on'})
        self.assertEqual((service['title'], service['timeout'], service['flash'], unusable), ('42', 90, True, []))
        self.assertEqual(alert_data({'timeout': 12.7})[0]['timeout'], 12)
        self.assertEqual(alert_data({'timeout': -5})[0]['timeout'], 0, 'the firmware clamps too')
        self.assertEqual(alert_data({'timeout': 10**9})[0]['timeout'], ALERT_MAX_TIMEOUT)
        self.assertEqual(alert_data({'flash': 1})[0]['flash'], True)
        self.assertEqual(alert_data({'flash': 'Off'})[0]['flash'], False)

    def test_one_unusable_value_is_left_empty_and_named(self):
        # A bare `Yes` in YAML arrives as a boolean; the rest of the alert still goes out.
        service, unusable = alert_data({'title': 'Door', 'button_text': True, 'timeout': 'soon', 'flash': 'maybe', 'icon': ['bell']})
        self.assertEqual(service, {'title': 'Door', 'subtitle': '', 'icon': '', 'color': '', 'button_text': '', 'timeout': 0, 'flash': False})
        self.assertEqual(unusable, ['icon', 'button_text', 'timeout', 'flash'])
        self.assertEqual(alert_data({'timeout': float('nan')})[1], ['timeout'])

    def test_limits_follow_the_firmware(self):
        header = (ROOT / 'components/smart_display/alert_overlay.h').read_text()
        self.assertEqual(int(re.search(r'MAX_TIMEOUT_SECONDS = (\d+);', header)[1]), ALERT_MAX_TIMEOUT)
        self.assertEqual(BROADCAST_EVENTS, {'esp_screens_show_alert': 'show_alert', 'esp_screens_dismiss_alert': 'dismiss_alert'})
        self.assertEqual(alert_reference()['broadcast'], {'show': BROADCAST_SHOW, 'dismiss': BROADCAST_DISMISS})


class AlertTargets(unittest.TestCase):
    def test_only_screens_that_can_show_it_now(self):
        screens = [{'name': 'Kitchen', 'node': 'kitchen', 'online': True, 'firmware': '0.2.31'},
                   {'name': 'Hall', 'node': 'hall', 'online': True, 'firmware': '0.2.38'},
                   {'name': 'Attic', 'node': 'attic', 'online': False, 'firmware': '0.2.38'},
                   {'name': 'Garage', 'node': 'garage', 'online': True, 'firmware': '0.2.30'},
                   {'name': 'Shed', 'node': 'shed', 'online': True, 'firmware': 'unknown'},
                   {'name': 'New', 'node': None, 'online': True, 'firmware': '0.2.38'},
                   {'name': 'Hall again', 'node': 'hall', 'online': True, 'firmware': '0.2.38'}]
        ready, skipped = alert_targets(screens)
        self.assertEqual([s['name'] for s in ready], ['Kitchen', 'Hall'])
        self.assertEqual([(s['name'], reason) for s, reason in skipped],
                         [('Attic', 'offline'), ('Garage', 'firmware 0.2.30'), ('Shed', 'firmware unknown'), ('New', 'device name unknown')])
        self.assertEqual(alert_targets([]), ([], []))


def fake_ha():
    """Three paired screens as Home Assistant's registry describes them: two current, one too old."""
    class HA:
        online = True

        def __init__(self):
            self.registry, self.devices, self.states = [], [], {}
            for device, node, firmware in (('d1', 'kitchen-screen', '0.2.38'), ('d2', 'hall', '0.2.31'), ('d3', 'attic', '0.2.30')):
                self.registry += [{'entity_id': f'text.{device}_tiles', 'platform': 'esphome', 'original_name': 'Tile settings', 'device_id': device},
                                  {'entity_id': f'sensor.{device}_node', 'platform': 'esphome', 'original_name': 'Device name', 'device_id': device},
                                  {'entity_id': f'sensor.{device}_fw', 'platform': 'esphome', 'original_name': 'Screen firmware', 'device_id': device}]
                self.devices.append({'id': device, 'name': node.title()})
                self.states.update({f'text.{device}_tiles': {'state': 'Synced'}, f'sensor.{device}_node': {'state': node},
                                    f'sensor.{device}_fw': {'state': firmware}})
            self.areas = []
            self.changed = asyncio.Event()
            self.broadcasts = asyncio.Queue()
            self.calls, self.failing = [], set()

        async def call(self, action, data):
            self.calls.append((action, data))
            if action in self.failing:
                raise ConnectionError('Home Assistant refused the command.')
    return HA()


@unittest.skipUnless(HAS_AIOHTTP, 'Run using .venv-portal/bin/python for server tests')
class Broadcast(unittest.IsolatedAsyncioTestCase):
    async def test_show_and_dismiss_reach_every_current_screen(self):
        with tempfile.TemporaryDirectory() as tmp:
            m = Manager(fake_ha(), Path(tmp) / 'screens.json')
            with self.assertLogs('screen_manager', 'INFO') as logs:
                result = await m.broadcast(BROADCAST_SHOW, {'title': 'Mail!', 'icon': 'mailbox', 'flash': True})
            self.assertEqual(result, {'sent': 2, 'skipped': 1, 'failed': 0})
            data = {'title': 'Mail!', 'subtitle': '', 'icon': 'mailbox', 'color': '', 'button_text': '', 'timeout': 0, 'flash': True}
            self.assertEqual(sorted(m.ha.calls), [('esphome.hall_show_alert', data), ('esphome.kitchen_screen_show_alert', data)])
            self.assertIn('esp_screens_show_alert: 2 of 3 screens (not: Attic firmware 0.2.30)', logs.output[-1])
            m.ha.calls.clear()
            await m.broadcast(BROADCAST_DISMISS, {'title': 'ignored'})
            self.assertEqual(sorted(m.ha.calls), [('esphome.hall_dismiss_alert', {}), ('esphome.kitchen_screen_dismiss_alert', {})])

    async def test_a_screen_added_later_gets_the_next_alert(self):
        with tempfile.TemporaryDirectory() as tmp:
            ha = fake_ha()
            m = Manager(ha, Path(tmp) / 'screens.json')
            await m.broadcast(BROADCAST_SHOW, {'title': 'One'})
            ha.registry = ha.registry + [{'entity_id': 'text.d4_tiles', 'platform': 'esphome', 'original_name': 'Tile settings', 'device_id': 'd4'},
                                         {'entity_id': 'sensor.d4_node', 'platform': 'esphome', 'original_name': 'Device name', 'device_id': 'd4'},
                                         {'entity_id': 'sensor.d4_fw', 'platform': 'esphome', 'original_name': 'Screen firmware', 'device_id': 'd4'}]
            ha.devices = ha.devices + [{'id': 'd4', 'name': 'Study'}]
            ha.states.update({'text.d4_tiles': {'state': 'Synced'}, 'sensor.d4_node': {'state': 'study'}, 'sensor.d4_fw': {'state': '0.2.38'}})
            ha.calls.clear()
            self.assertEqual((await m.broadcast(BROADCAST_SHOW, {'title': 'Two'}))['sent'], 3)
            self.assertIn('esphome.study_show_alert', [action for action, _ in ha.calls])

    async def test_one_failing_screen_does_not_stop_the_others(self):
        with tempfile.TemporaryDirectory() as tmp:
            m = Manager(fake_ha(), Path(tmp) / 'screens.json')
            m.ha.failing.add('esphome.hall_show_alert')
            with self.assertLogs('screen_manager', 'INFO') as logs:
                result = await m.broadcast(BROADCAST_SHOW, {'title': 'Smoke', 'timeout': 'later'})
            self.assertEqual(result, {'sent': 1, 'skipped': 1, 'failed': 1})
            self.assertEqual(len(m.ha.calls), 2)
            self.assertTrue(any('unusable timeout left empty' in line for line in logs.output), logs.output)
            self.assertIn('1 of 3 screens (not: Attic firmware 0.2.30; Hall ConnectionError)', logs.output[-1])

    async def test_the_loop_keeps_the_order_and_survives_errors(self):
        with tempfile.TemporaryDirectory() as tmp:
            m = Manager(fake_ha(), Path(tmp) / 'screens.json')
            original = m.broadcast
            async def broadcast(event_type, data):
                if data.get('title') == 'boom':
                    raise RuntimeError('unexpected')
                return await original(event_type, data)
            m.broadcast = broadcast
            task = asyncio.create_task(m.alert_loop())
            for item in ((BROADCAST_SHOW, {'title': 'boom'}), (BROADCAST_SHOW, {'title': 'Door'}), (BROADCAST_DISMISS, {})):
                m.ha.broadcasts.put_nowait(item)
            with self.assertLogs('screen_manager', 'INFO'):
                for _ in range(50):
                    if len(m.ha.calls) == 4:
                        break
                    await asyncio.sleep(0.01)
            task.cancel()
            self.assertEqual([action.rsplit('_', 2)[-2:] for action, _ in m.ha.calls], [['show', 'alert']] * 2 + [['dismiss', 'alert']] * 2)

    async def test_home_assistant_queues_the_events_instead_of_calling_from_the_reader(self):
        class Message:
            type = WSMsgType.TEXT
            def __init__(self, data): self.data = data
            def json(self): return self.data
        class Socket:
            def __init__(self, messages): self.messages = messages
            def __aiter__(self): return self._iterate()
            async def _iterate(self):
                for message in self.messages:
                    yield Message(message)
        ha = HomeAssistant(None, 'http://ha/api', 'token')
        ha.ws = Socket([{'type': 'event', 'event': {'event_type': BROADCAST_SHOW, 'data': {'title': 'Mail!'}}},
                        {'type': 'event', 'event': {'event_type': 'call_service', 'data': {}}},
                        {'type': 'event', 'event': {'event_type': BROADCAST_DISMISS, 'data': {}}}])
        with self.assertRaises(ConnectionError):
            await ha.read()
        self.assertEqual([ha.broadcasts.get_nowait() for _ in range(ha.broadcasts.qsize())],
                         [(BROADCAST_SHOW, {'title': 'Mail!'}), (BROADCAST_DISMISS, {})])
        self.assertIn('*BROADCAST_EVENTS', (ROOT / 'screen_manager/app/server.py').read_text(), 'subscribed on connect')


class Page(unittest.TestCase):
    def test_cheatsheet_explains_the_event(self):
        import editor_sources
        cheatsheet = editor_sources.component('AlertsView')
        for marker in ('id="alerts-all"', '["alerts-all", "All screens"]', 'id="alerts-all-example"', 'id="alerts-all-copy"',
                       'esp_screens_show_alert', 'esp_screens_dismiss_alert', 'Settings → Claude'):
            self.assertIn(marker, cheatsheet, marker)
        for marker in ('const allYaml = computed', 'broadcast?.show', 'copyText(allYaml, undefined, \'YAML\')'):
            self.assertIn(marker, cheatsheet, marker)


if __name__ == '__main__':
    unittest.main()
