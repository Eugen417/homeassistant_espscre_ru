"""Push a demo layout with every card type straight into a screen's inbox.

Uses the same message builder as ESP Screen Manager, but with synthetic states,
so rendering can be checked without touching Home Assistant. The running
manager restores the real layout on its next keepalive (about two minutes).
"""
import argparse
import asyncio
from datetime import datetime, timedelta, timezone
from pathlib import Path
import sys
import time
import yaml
from aioesphomeapi import APIClient

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'screen_manager/app'))
from core import extras, packets, state_message, validate_layout  # noqa: E402

def demo_layout():
    return validate_layout({'title': 'Demo cards', 'tiles': [
        {'entity': 'screen.clock', 'name': '', 'options': {'display': 'analog', 'background': 'none'}},
        {'entity': 'weather.demo', 'name': 'Outside', 'options': {'display': 'forecast'}},
        {'entity': 'sensor.demo_temperature', 'name': 'Living room', 'options': {'display': 'graph', 'icon': 'thermometer'}},
        {'entity': 'person.demo', 'name': 'Max', 'options': {}},
        {'entity': 'timer.demo', 'name': 'Eggs', 'options': {'icon': 'chef-hat'}},
        {'entity': 'sun.sun', 'name': 'Sun', 'options': {'display': 'sunpath'}},
        {'entity': 'sensor.demo_energy', 'name': 'Usage today', 'options': {'display': 'graph', 'size': 'wide'}},
        {'entity': 'light.demo', 'name': 'Table lamp', 'options': {'display': 'watch', 'size': 'wide', 'icon': 'lamp'}},
        {'entity': 'weather.demo_default', 'name': 'Weather now', 'options': {}},
    ]})

# Direct controls on wide cards (firmware 0.2.19+): one card per control set.
def controls_layout():
    return validate_layout({'title': 'Direct control', 'tiles': [
        {'entity': 'cover.demo_curtain', 'name': 'Curtains Window covering', 'options': {'size': 'wide', 'icon': 'curtains'}},
        {'entity': 'climate.demo_ac', 'name': 'AC', 'options': {'size': 'wide'}},
        {'entity': 'media_player.demo_sonos', 'name': 'Sonos', 'options': {'size': 'wide', 'icon': 'speaker'}},
        {'entity': 'climate.demo_heating', 'name': 'Heating', 'options': {'size': 'wide', 'controls': 'mode', 'icon': 'radiator'}},
        {'entity': 'media_player.demo_radio', 'name': 'Sonos bedroom', 'options': {'size': 'wide', 'controls': 'playback', 'icon': 'radio'}},
        {'entity': 'vacuum.demo_robot', 'name': 'Pippa', 'options': {'size': 'wide'}},
        {'entity': 'switch.demo_desk', 'name': 'Desk', 'options': {'size': 'wide', 'background': 'blue', 'icon': 'power-socket-eu'}},
        {'entity': 'light.demo_table_lamp', 'name': 'Table lamp', 'options': {'size': 'wide', 'controls': 'brightness', 'icon': 'lamp'}},
        {'entity': 'number.demo_target', 'name': 'Target humidity', 'options': {'size': 'wide', 'icon': 'water-percent'}},
        {'entity': 'select.demo_mode', 'name': 'Heating mode', 'options': {'size': 'wide', 'icon': 'thermostat'}},
        {'entity': 'timer.demo_eggs', 'name': 'Eggs', 'options': {'size': 'wide', 'icon': 'chef-hat'}},
        {'entity': 'scene.demo_evening', 'name': 'Evening', 'options': {'size': 'wide', 'icon': 'sofa'}},
        {'entity': 'fan.demo_fan', 'name': 'Fan', 'options': {'size': 'wide'}},
        {'entity': 'cover.demo_shutter', 'name': 'Shutter', 'options': {'size': 'wide', 'controls': 'position', 'icon': 'window-shutter'}},
        {'entity': 'light.demo_ceiling', 'name': 'Ceiling light', 'options': {'size': 'wide', 'icon': 'ceiling-light'}},
        {'entity': 'weather.demo_outside', 'name': 'Outside', 'options': {'display': 'forecast'}},
        {'entity': 'script.demo_tv', 'name': 'Turn on TV', 'options': {'icon': 'television'}},
        {'entity': 'scene.demo_morning', 'name': 'Morning', 'options': {'icon': 'weather-sunset-up'}},
    ]})

def controls_states(now):
    end = now + timedelta(minutes=4, seconds=32)
    return {
        'cover.demo_curtain': {'state': 'open', 'attributes': {'current_position': 80, 'device_class': 'curtain', 'supported_features': 15}},
        'climate.demo_ac': {'state': 'cool', 'attributes': {'current_temperature': 21.5, 'temperature': 20, 'min_temp': 16, 'max_temp': 32, 'target_temp_step': 1.0, 'hvac_modes': ['off', 'heat_cool', 'cool', 'heat', 'fan_only', 'dry'], 'fan_modes': ['auto', 'low', 'medium', 'high'], 'fan_mode': 'low', 'swing_modes': ['off', 'both', 'vertical', 'horizontal'], 'swing_mode': 'off', 'supported_features': 425}},
        'media_player.demo_sonos': {'state': 'playing', 'attributes': {'volume_level': 0.17, 'is_volume_muted': False, 'media_title': 'TV', 'supported_features': 8321599}},
        'climate.demo_heating': {'state': 'heat', 'attributes': {'current_temperature': 19.5, 'temperature': 21, 'hvac_action': 'heating', 'hvac_modes': ['off', 'heat', 'auto']}},
        'media_player.demo_radio': {'state': 'playing', 'attributes': {'volume_level': 0.15, 'media_title': 'NPO Radio 2', 'supported_features': 8321599}},
        'vacuum.demo_robot': {'state': 'docked', 'attributes': {'battery_level': 100, 'fan_speed': 'max', 'fan_speed_list': ['quiet', 'balanced', 'turbo', 'max'], 'supported_features': 30524}},
        'switch.demo_desk': {'state': 'on', 'attributes': {}},
        'light.demo_table_lamp': {'state': 'on', 'attributes': {'brightness': 163}},
        'number.demo_target': {'state': '55', 'attributes': {'min': 30, 'max': 70, 'step': 5, 'unit_of_measurement': '%'}},
        'select.demo_mode': {'state': 'Comfort', 'attributes': {'options': ['Eco', 'Comfort', 'Boost']}},
        'timer.demo_eggs': {'state': 'active', 'attributes': {'finishes_at': end.isoformat(), 'duration': '0:05:00', 'remaining': '0:05:00'}},
        'scene.demo_evening': {'state': (now - timedelta(hours=3)).isoformat(), 'attributes': {}},
        'fan.demo_fan': {'state': 'off', 'attributes': {'percentage': 0}},
        'cover.demo_shutter': {'state': 'open', 'attributes': {'current_position': 35, 'supported_features': 15}},
        'light.demo_ceiling': {'state': 'off', 'attributes': {}},
        'weather.demo_outside': {'state': 'rainy', 'attributes': {'temperature': 18.4, 'temperature_unit': '°C', 'humidity': 92, 'wind_speed': 12.2, 'wind_speed_unit': 'km/h', 'apparent_temperature': 17.1}},
        'script.demo_tv': {'state': 'off', 'attributes': {'last_triggered': (now - timedelta(hours=2, minutes=8)).isoformat()}},
        'scene.demo_morning': {'state': (now - timedelta(days=1, hours=5)).isoformat(), 'attributes': {}},
    }

def demo_states(now):
    end = now + timedelta(minutes=4, seconds=32)
    return {
        'weather.demo': {'state': 'partlycloudy', 'attributes': {'temperature': 18.4, 'temperature_unit': '°C'}},
        'weather.demo_default': {'state': 'rainy', 'attributes': {'temperature': 12.0, 'temperature_unit': '°C'}},
        'sensor.demo_temperature': {'state': '21.5', 'attributes': {'unit_of_measurement': '°C'}},
        'sensor.demo_energy': {'state': '7.42', 'attributes': {'unit_of_measurement': 'kWh'}},
        'person.demo': {'state': 'home', 'attributes': {'icon': 'mdi:account-child'}},
        'timer.demo': {'state': 'active', 'attributes': {'finishes_at': end.isoformat(), 'duration': '0:05:00', 'remaining': '0:05:00'}},
        'sun.sun': {'state': 'above_horizon', 'attributes': {'next_rising': (now + timedelta(hours=9)).isoformat(), 'next_setting': (now + timedelta(hours=2)).isoformat()}},
        'light.demo': {'state': 'on', 'attributes': {'brightness': 180}},
    }

def demo_forecast(now):
    conditions = ['sunny', 'partlycloudy', 'rainy', 'cloudy', 'lightning-rainy', 'snowy']
    return [{'datetime': (now + timedelta(days=i)).isoformat(), 'condition': conditions[i], 'temperature': 21 - i, 'templow': 11 + i,
             'precipitation': [0, 0, 4.2, 0.3, 11.5, 2][i], 'precipitation_probability': [5, 20, 80, 30, 95, 60][i]}
            for i in range(6)]

def demo_hourly(now):
    conditions = ['rainy', 'rainy', 'partlycloudy', 'partlycloudy', 'sunny', 'sunny', 'cloudy', 'lightning-rainy', 'rainy', 'cloudy']
    start = now.replace(minute=0, second=0, microsecond=0)
    return [{'datetime': (start + timedelta(hours=i)).isoformat(), 'condition': conditions[i], 'temperature': 18.4 + i * 0.6,
             'precipitation': [0.4, 0.2, 0, 0, 0, 0, 0, 2.1, 1.0, 0][i], 'precipitation_probability': [70, 55, 10, 5, 0, 0, 15, 85, 60, 20][i]}
            for i in range(10)]

def messages(inbox, rotate=0, digital=False, wide=False, controls=False):
    now = datetime.now(timezone.utc)
    layout, states = (controls_layout(), controls_states(now)) if controls else (demo_layout(), demo_states(now))
    if digital:
        layout['tiles'][0]['options'] = {'display': 'digital'}
    elif wide:
        layout['tiles'][0]['options'] = {'display': 'analog', 'size': 'wide'}
    layout['tiles'] = layout['tiles'][rotate:] + layout['tiles'][:rotate]
    out = [{'v': 1, 'op': 'layout', 'inbox': inbox, 'title': layout['title'], 'entities': [t['entity'] for t in layout['tiles']]}]
    for i, tile in enumerate(layout['tiles']):
        forecast = demo_forecast(now) if tile['entity'].startswith('weather.') else None
        hourly = demo_hourly(now) if tile['entity'].startswith('weather.') else None
        message = state_message(i, tile, states, extras(tile, states, forecast, None, hourly, now))
        if tile['entity'].startswith('sensor.'):
            message['history'] = {'hours': 24, 'values': [round(18 + 4 * ((k * 7) % 11) / 10, 2) if k % 5 else None for k in range(24)]}
        out.append(message)
    return out

async def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--host', required=True)
    p.add_argument('--name', required=True, help='Expected DEVICE_NAME')
    p.add_argument('--secrets', type=Path, default=Path(__file__).resolve().parents[1] / 'secrets.yaml')
    p.add_argument('--rotate', type=int, default=0, help='Put tile N first, so later pages land on page 1')
    p.add_argument('--digital', action='store_true', help='Digital clock on a single tile instead of the analog calendar card without a background')
    p.add_argument('--wide', action='store_true', help='Analog clock double-width (dial with digital time and date)')
    p.add_argument('--controls', action='store_true', help='Double-width cards with direct control (firmware 0.2.19+), three per page')
    args = p.parse_args()
    client = APIClient(args.host, 6053, noise_psk=yaml.safe_load(args.secrets.read_text())['api_encryption_key'],
                       client_info='Demo layout', expected_name=args.name)
    await client.connect(login=True)
    try:
        entities, _ = await client.list_entities_services()
        # Firmware built before the English translation still names this entity in Dutch.
        inbox = next(e for e in entities if type(e).__name__ == 'TextInfo' and e.name in ('Tile settings', 'Tegelinstellingen'))
        # The firmware answers through the inbox entity's state, not the log.
        replies = []
        def state(update):
            if getattr(update, 'key', None) == inbox.key and getattr(update, 'state', None):
                replies.append(update.state)
        client.subscribe_states(state)
        started = time.monotonic()
        count = len(controls_layout()['tiles']) if args.controls else 9
        for message in messages(inbox.object_id if hasattr(inbox, 'object_id') else 'text.inbox', args.rotate % count, args.digital, args.wide, args.controls):
            for packet in packets(message):
                client.text_command(inbox.key, packet)
                await asyncio.sleep(0.05)
        await asyncio.sleep(1.5)
        print(f'Demo layout sent in {time.monotonic() - started:.1f}s; inbox reports: {" / ".join(dict.fromkeys(replies)) or "(no status change)"}')
        # Firmware built before the English translation still reports the Dutch originals.
        accepted = ('Layout received', 'Synced', 'Loading tiles', 'Indeling ontvangen', 'Gesynchroniseerd', 'Tegels laden')
        if not any(r in accepted for r in replies):
            raise SystemExit('The firmware did not accept the demo layout.')
    finally:
        await client.disconnect()

if __name__ == '__main__':
    asyncio.run(main())
