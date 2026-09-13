"""Weather card data (hours, rain) and last-run times for scenes, scripts and buttons."""
import asyncio
from datetime import datetime, timezone
from pathlib import Path
import sys
import tempfile
import unittest
from zoneinfo import ZoneInfo

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'screen_manager/app'))
from core import extras, packets, state_message
import test_portal

TZ = ZoneInfo('Europe/Amsterdam')
NOW = datetime(2026, 9, 13, 11, 40, tzinfo=timezone.utc)


def hourly():
    return [{'datetime': f'2026-09-13T{h:02d}:00:00+00:00', 'condition': 'rainy' if h < 12 else 'partlycloudy', 'temperature': 18 + h / 10,
             'precipitation': 0.2 if h < 12 else 0.0, 'precipitation_probability': 60 if h < 12 else 5} for h in range(8, 30)]


def daily():
    return [{'datetime': f'2026-09-{13 + i}T10:00:00+00:00', 'condition': 'cloudy', 'temperature': 21 + i, 'templow': 12.1, 'precipitation': 1.7 if i else 0.0}
            for i in range(6)]


class WeatherExtras(unittest.TestCase):
    def test_hours_start_now_and_carry_rain(self):
        states = {'weather.huis': {'state': 'rainy', 'attributes': {'temperature': 18.4, 'humidity': 92, 'wind_speed': 12.2, 'wind_speed_unit': 'km/h'}}}
        extra = extras({'entity': 'weather.huis'}, states, daily(), TZ, hourly(), NOW)
        self.assertEqual([h['t'] for h in extra['hours']], ['13:00', '14:00', '15:00', '16:00', '17:00', '18:00', '19:00', '20:00'], 'local time, from the current hour')
        self.assertEqual(extra['hours'][0], {'t': '13:00', 'c': 'rainy', 'h': 19.1, 'p': 60, 'r': 0.2})
        self.assertEqual(extra['hours'][1]['r'], 0.0)
        self.assertEqual(len(extra['days']), 5)
        self.assertEqual(extra['days'][1], {'d': 'ma', 'c': 'cloudy', 'h': 22, 'l': 12.1, 'r': 1.7}, 'no probability when the provider gives none')
        message = state_message(0, {'entity': 'weather.huis', 'name': 'Buiten', 'options': {'display': 'forecast'}}, states, extra)
        self.assertEqual(message['a']['wind_speed_unit'], 'km/h')
        self.assertEqual(message['a']['humidity'], 92)
        self.assertLessEqual(len(packets(message)), 8, 'a full weather card still fits comfortably')

    def test_weather_without_forecasts_sends_nothing_extra(self):
        self.assertIsNone(extras({'entity': 'weather.huis'}, {}, None, TZ, None, NOW))
        self.assertEqual(extras({'entity': 'weather.huis'}, {}, daily(), TZ, [], NOW).keys(), {'days'})


class LastRun(unittest.TestCase):
    def test_scripts_scenes_and_buttons_report_when_they_last_ran(self):
        states = {'script.tv': {'state': 'off', 'attributes': {'last_triggered': '2026-09-13T08:10:49.403953+00:00'}},
                  'scene.avond': {'state': '2026-09-12T16:33:37.040291+00:00', 'attributes': {}},
                  'button.bel': {'state': '2026-09-13T09:00:00+00:00', 'attributes': {}},
                  'script.nieuw': {'state': 'off', 'attributes': {'last_triggered': None}},
                  'scene.nooit': {'state': 'unknown', 'attributes': {}}}
        self.assertEqual(extras({'entity': 'script.tv'}, states, None, TZ), {'last': 1789287049})
        self.assertEqual(extras({'entity': 'scene.avond'}, states, None, TZ), {'last': 1789230817})
        self.assertEqual(extras({'entity': 'button.bel'}, states, None, TZ), {'last': 1789290000})
        self.assertIsNone(extras({'entity': 'script.nieuw'}, states, None, TZ))
        self.assertIsNone(extras({'entity': 'scene.nooit'}, states, None, TZ))
        message = state_message(0, {'entity': 'script.tv', 'name': ''}, states, extras({'entity': 'script.tv'}, states, None, TZ))
        self.assertEqual(message['x'], {'last': 1789287049})
        self.assertNotIn('last_triggered', message['a'], 'the raw timestamp stays behind; the firmware gets the epoch')


class HourlySync(unittest.IsolatedAsyncioTestCase):
    async def test_manager_asks_for_daily_and_hourly_forecasts(self):
        with tempfile.TemporaryDirectory() as tmp:
            m = test_portal.ManagerTests().setup_manager(Path(tmp) / 'screens.json')
            m.ha.states['weather.huis'] = {'state': 'rainy', 'attributes': {'friendly_name': 'Huis', 'temperature': 18.4}}
            asked = []
            async def forecast(entity, kind='daily'):
                asked.append(kind)
                return daily() if kind == 'daily' else hourly()
            m.ha.forecast = forecast
            m.ha.registry.append({'entity_id': 'sensor.fw', 'platform': 'esphome', 'original_name': 'Schermfirmware'})
            m.ha.states['sensor.fw'] = {'state': '0.2.19'}
            m.save('text.screen', {'title': 'Thuis', 'tiles': [{'entity': 'weather.huis', 'name': '', 'options': {'display': 'forecast'}}]})
            await m.sync_one('text.screen', m.layouts['text.screen'])
            self.assertEqual(sorted(asked), ['daily', 'hourly'])
            sent = m.ha.messages[1][1]
            self.assertEqual(len(sent['x']['days']), 5)
            self.assertTrue(sent['x']['hours'])
            await m.sync_one('text.screen', m.layouts['text.screen'])
            self.assertEqual(len(asked), 2, 'both forecasts are cached')
