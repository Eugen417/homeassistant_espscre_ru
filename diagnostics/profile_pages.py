"""Measure maximum-page heap on an explicitly selected test screen, without HA actions.

Run with no manager writing this screen. Synthetic sensor values never call HA.
Legacy protocol is a baseline measurement tool only, not device compatibility.
Wait for two five-minute diagnostic samples after initialization before comparing.
"""
import argparse
import asyncio
import json
from pathlib import Path
import sys
import time

import yaml
from aioesphomeapi import APIClient, LogLevel

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'screen_manager/app'))
from core import Grid, packets, state_message
from layout_migrations import migrate_legacy
from send_layout import api_sender, screen_grid


async def run(args):
    client = APIClient(args.host, 6053, noise_psk=yaml.safe_load(args.secrets.read_text())['api_encryption_key'],
                       client_info='Page memory diagnostics', expected_name=args.name)
    await client.connect(login=True)
    try:
        info = await client.device_info()
        if info.mac_address.lower() != args.mac.lower():
            raise RuntimeError('Connected screen MAC does not match the authorized test screen')
        entities, services = await client.list_entities_services()
        grid = await screen_grid(client, entities)
        inbox = next(e for e in entities if type(e).__name__ == 'TextInfo' and e.name == 'Tile settings')
        keys = {e.key: e.name for e in entities if e.name.startswith(('Heap ', 'Psram '))}
        started = time.monotonic()
        def diagnostic(update):
            if update.key in keys:
                print(json.dumps({'seconds': round(time.monotonic() - started, 2), 'metric': keys[update.key], 'value': update.state}), flush=True)
        client.subscribe_states(diagnostic)
        def log(message):
            text = message.message.decode(errors='replace') if isinstance(message.message, bytes) else message.message
            if any(mark in text for mark in ('[health', '[memory', 'not enough', 'Out of memory', 'page_check=', 'UI_TEST')):
                print(text, flush=True)
        client.subscribe_logs(log, log_level=LogLevel.LOG_LEVEL_DEBUG, dump_config=False)
        count = min(8, 64 // (grid.columns * grid.rows))
        layout = {'title': 'Page memory test', 'pages': count, 'header': {'items': [{'type': 'entity', 'entity': f'sensor.memory_{i}', 'show': 'always'} for i in range(6)]},
                  'tiles': [{'entity': f'sensor.memory_{i}', 'name': f'Measured sensor {i + 1}', 'slot': i, 'options': {'display': 'graph'}} for i in range(count * grid.columns * grid.rows)]}
        if args.long_text:
            layout['title'] = 'Screen ' + 'x' * 89
            layout['page_titles'] = [f'Page {index + 1} ' + 'x' * 89 for index in range(count)]
            for index, tile in enumerate(layout['tiles']):
                tile['name'] = (f'Sensor {index + 1} ' + 'x' * 80)[:80]
        states = {tile['entity']: {'state': str(20 + i / 10), 'attributes': {'unit_of_measurement': '°C'}} for i, tile in enumerate(layout['tiles'])}
        values = [state_message(i, tile, states) for i, tile in enumerate(layout['tiles'])]
        for message in values: message['history'] = {'hours': 24, 'values': [20 + (i % 7) / 10 for i in range(24)]}
        bar = [{'k': 'text', 't': str(20 + i / 10), 'i': 'F050F'} for i in range(6)]
        if args.long_text:
            for index, item in enumerate(bar): item['t'] = str(index + 1) + 'x' * 47
        if args.protocol == 1:
            messages = [{'v': 1, 'op': 'layout', 'inbox': inbox.object_id, 'title': layout['title'], 'pages': count,
                         'entities': list(states), 'slots': list(range(len(values))), 'keepalive': 3600,
                         **({'page_titles': layout['page_titles']} if args.long_text else {})}, *values,
                        {'v': 1, 'op': 'header', 'items': bar}]
            if any(service.name == 'screen_message' for service in services):
                # Use the same native transport as protocol 2 when the baseline
                # supports it. Chunking would bias heap and initialization time.
                send = api_sender(client, services).send
                for message in messages:
                    answer = await send(message)
                    assert not answer.get('status', '').startswith('Error'), answer
            else:
                for message in messages:
                    for packet in packets(message):
                        client.text_command(inbox.key, packet)
                        await asyncio.sleep(.09)
        else:
            record = migrate_legacy(layout, grid)
            sender = api_sender(client, services)
            await sender.synchronize(inbox.object_id, record, {'keepalive': 3600, 'clock_24h': True, 'numbers': 'point', 'group_min': 1, 'percent_space': False}, values, [bar] * count)
        print(json.dumps({'initialized_seconds': round(time.monotonic() - started, 2), 'pages': count, 'tiles': len(values), 'protocol': args.protocol, 'long_text': args.long_text, 'compiled': info.compilation_time}), flush=True)
        await client.execute_service(next(s for s in services if s.name == 'ui_self_test'), {})
        await asyncio.sleep(args.seconds)
        print('Measurement complete', flush=True)
    finally:
        await client.disconnect()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host', required=True)
    parser.add_argument('--name', required=True)
    parser.add_argument('--mac', required=True)
    parser.add_argument('--secrets', required=True, type=Path)
    parser.add_argument('--protocol', required=True, type=int, choices=(1, 2))
    parser.add_argument('--seconds', type=int, default=620)
    parser.add_argument('--long-text', action='store_true', help='Use maximum-length page titles, tile names and top-bar values')
    asyncio.run(run(parser.parse_args()))
