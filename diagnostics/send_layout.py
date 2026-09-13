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
    return validate_layout({'title': 'Demo kaarten', 'tiles': [
        {'entity': 'screen.clock', 'name': '', 'options': {'display': 'analog', 'background': 'none'}},
        {'entity': 'weather.demo', 'name': 'Buiten', 'options': {'display': 'forecast'}},
        {'entity': 'sensor.demo_temperatuur', 'name': 'Woonkamer', 'options': {'display': 'graph', 'icon': 'thermometer'}},
        {'entity': 'person.demo', 'name': 'Max', 'options': {}},
        {'entity': 'timer.demo', 'name': 'Eieren', 'options': {'icon': 'chef-hat'}},
        {'entity': 'sun.sun', 'name': 'Zon', 'options': {'display': 'sunpath'}},
        {'entity': 'sensor.demo_energie', 'name': 'Verbruik vandaag', 'options': {'display': 'graph', 'size': 'wide'}},
        {'entity': 'light.demo', 'name': 'Tafellamp', 'options': {'display': 'watch', 'size': 'wide', 'icon': 'lamp'}},
        {'entity': 'weather.demo_standaard', 'name': 'Weer nu', 'options': {}},
    ]})

def demo_states(now):
    end = now + timedelta(minutes=4, seconds=32)
    return {
        'weather.demo': {'state': 'partlycloudy', 'attributes': {'temperature': 18.4, 'temperature_unit': '°C'}},
        'weather.demo_standaard': {'state': 'rainy', 'attributes': {'temperature': 12.0, 'temperature_unit': '°C'}},
        'sensor.demo_temperatuur': {'state': '21.5', 'attributes': {'unit_of_measurement': '°C'}},
        'sensor.demo_energie': {'state': '7.42', 'attributes': {'unit_of_measurement': 'kWh'}},
        'person.demo': {'state': 'home', 'attributes': {'icon': 'mdi:account-child'}},
        'timer.demo': {'state': 'active', 'attributes': {'finishes_at': end.isoformat(), 'duration': '0:05:00', 'remaining': '0:05:00'}},
        'sun.sun': {'state': 'above_horizon', 'attributes': {'next_rising': (now + timedelta(hours=9)).isoformat(), 'next_setting': (now + timedelta(hours=2)).isoformat()}},
        'light.demo': {'state': 'on', 'attributes': {'brightness': 180}},
    }

def demo_forecast(now):
    conditions = ['sunny', 'partlycloudy', 'rainy', 'cloudy', 'lightning-rainy', 'snowy']
    return [{'datetime': (now + timedelta(days=i)).isoformat(), 'condition': conditions[i], 'temperature': 21 - i, 'templow': 11 + i}
            for i in range(6)]

def messages(inbox, rotate=0, digital=False, wide=False):
    now = datetime.now(timezone.utc)
    layout, states = demo_layout(), demo_states(now)
    if digital:
        layout['tiles'][0]['options'] = {'display': 'digital'}
    elif wide:
        layout['tiles'][0]['options'] = {'display': 'analog', 'size': 'wide'}
    layout['tiles'] = layout['tiles'][rotate:] + layout['tiles'][:rotate]
    out = [{'v': 1, 'op': 'layout', 'inbox': inbox, 'title': layout['title'], 'entities': [t['entity'] for t in layout['tiles']]}]
    for i, tile in enumerate(layout['tiles']):
        forecast = demo_forecast(now) if tile['entity'].startswith('weather.') else None
        message = state_message(i, tile, states, extras(tile, states, forecast, None))
        if tile['entity'].startswith('sensor.'):
            message['history'] = {'hours': 24, 'values': [round(18 + 4 * ((k * 7) % 11) / 10, 2) if k % 5 else None for k in range(24)]}
        out.append(message)
    return out

async def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--host', required=True)
    p.add_argument('--name', required=True, help='Verwachte DEVICE_NAME')
    p.add_argument('--secrets', type=Path, default=Path(__file__).resolve().parents[1] / 'secrets.yaml')
    p.add_argument('--rotate', type=int, default=0, help='Zet tegel N vooraan, zodat latere pagina\'s op pagina 1 komen')
    p.add_argument('--digital', action='store_true', help='Digitale klok op een enkele tegel in plaats van de analoge kalenderkaart zonder achtergrond')
    p.add_argument('--wide', action='store_true', help='Analoge klok dubbelbreed (wijzerplaat met digitale tijd en datum)')
    args = p.parse_args()
    client = APIClient(args.host, 6053, noise_psk=yaml.safe_load(args.secrets.read_text())['api_encryption_key'],
                       client_info='Demo layout', expected_name=args.name)
    await client.connect(login=True)
    try:
        entities, _ = await client.list_entities_services()
        inbox = next(e for e in entities if type(e).__name__ == 'TextInfo' and e.name == 'Tegelinstellingen')
        # The firmware answers through the inbox entity's state, not the log.
        replies = []
        def state(update):
            if getattr(update, 'key', None) == inbox.key and getattr(update, 'state', None):
                replies.append(update.state)
        client.subscribe_states(state)
        started = time.monotonic()
        for message in messages(inbox.object_id if hasattr(inbox, 'object_id') else 'text.inbox', args.rotate % 9, args.digital, args.wide):
            for packet in packets(message):
                client.text_command(inbox.key, packet)
                await asyncio.sleep(0.05)
        await asyncio.sleep(1.5)
        print(f'Demo-indeling verstuurd in {time.monotonic() - started:.1f}s; inbox meldt: {" / ".join(dict.fromkeys(replies)) or "(geen statuswijziging)"}')
        if not any(r in ('Indeling ontvangen', 'Gesynchroniseerd', 'Tegels laden') for r in replies):
            raise SystemExit('De firmware accepteerde de demo-indeling niet.')
    finally:
        await client.disconnect()

if __name__ == '__main__':
    asyncio.run(main())
