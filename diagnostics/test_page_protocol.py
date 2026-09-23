"""Exercise real firmware transaction failures on an explicitly authorized test screen.

No HA actions. Stop the test manager before running. Includes deliberate invalid
messages and loss of each initialization acknowledgment, followed by recovery.
"""
import argparse
import asyncio
from copy import deepcopy
import json
from pathlib import Path
import sys
import time

import yaml
from aioesphomeapi import APIClient

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'screen_manager/app'))
from core import state_message
from layout_migrations import migrate_legacy
from page_delivery import Sender, configuration, prepare
from page_layout import compile_tiles, new_id
from send_layout import screen_grid


async def run(args):
    client = APIClient(args.host, 6053, noise_psk=yaml.safe_load(args.secrets.read_text())['api_encryption_key'],
                       client_info='Page protocol acceptance', expected_name=args.name)
    await client.connect(login=True)
    started = time.monotonic()
    checks = []
    def passed(name):
        checks.append(name)
        print('PASS:', name, flush=True)
    try:
        info = await client.device_info()
        assert info.mac_address.lower() == args.mac.lower(), 'Unexpected physical test board'
        entities, services = await client.list_entities_services()
        grid = await screen_grid(client, entities)
        inbox = next(e for e in entities if type(e).__name__ == 'TextInfo' and e.name == 'Tile settings').object_id
        service = next(s for s in services if s.name == 'screen_message')
        async def raw(message):
            answer = await client.execute_service(service, {'message': json.dumps(message, ensure_ascii=False, separators=(',', ':'))}, return_response=True)
            assert answer and answer.success, 'API service failed'
            return json.loads(answer.response_data)
        async def show(number):
            await client.execute_service(next(s for s in services if s.name == 'show_page'), {'page': number})
        region = {'keepalive': 120, 'clock_24h': True, 'numbers': 'point', 'group_min': 1, 'percent_space': False}
        record = migrate_legacy({'title': 'Protocol test', 'pages': 3, 'tiles': [
            {'entity': 'sensor.protocol_a', 'name': 'A', 'slot': 0, 'options': {'size': 'wide'}},
            {'entity': 'screen.page_1', 'name': 'Return', 'slot': grid.columns * grid.rows * 2},
        ]}, grid)
        record['layout']['homePageId'] = record['layout']['pages'][2]['id']
        record['layout']['pages'][1]['navigation']['excludeFromPagination'] = True
        def resolved(candidate):
            tiles = compile_tiles(candidate['layout'], grid)
            values = [state_message(i, tile, {'sensor.protocol_a': {'state': '21.5', 'attributes': {}}}) for i, tile in enumerate(tiles)]
            bars = [[{'k': 'text', 't': f'Page {i + 1}'}] for i in range(len(candidate['layout']['pages']))]
            return values, bars
        sender = Sender(raw)
        values, bars = resolved(record)
        await sender.synchronize(inbox, record, region, values, bars)
        async def status(current=sender):
            return await current._packet({'op': 'ping'}, current.confirmed)
        answer = await status()
        assert answer['page'] == 3 and answer['home'] == 3 and answer['pagination_count'] == 2, answer
        passed('Cold configuration selects non-first Home; excluded page does not count')
        await show(2)
        assert (await status())['page'] == 2
        passed('Numeric Show page still opens excluded full-order page 2')
        moved = deepcopy(record)
        moved['layout']['pages'] = [record['layout']['pages'][2], record['layout']['pages'][0], record['layout']['pages'][1]]
        values, bars = resolved(moved)
        await sender.synchronize(inbox, moved, region, values, bars)
        assert (await status())['page'] == 3
        passed('Reorder preserves viewed page by stable ID')
        moved['layout']['homePageId'] = moved['layout']['pages'][1]['id']
        values, bars = resolved(moved)
        await sender.synchronize(inbox, moved, region, values, bars)
        assert (await status())['page'] == 3 and (await status())['home'] == 2
        passed('Changing Home does not navigate the active page')
        moved['layout']['pages'].pop()
        values, bars = resolved(moved)
        await sender.synchronize(inbox, moved, region, values, bars)
        assert (await status())['page'] == 2
        passed('Deleting the active page falls back to the designated Home')
        for page in moved['layout']['pages']: page['navigation']['excludeFromPagination'] = True
        values, bars = resolved(moved)
        await sender.synchronize(inbox, moved, region, values, bars)
        assert (await status())['pagination_count'] == 0
        passed('All pages, including Home, may opt out of pagination')
        # Each replacement is unique; preserve the same complete document on retry.
        values, bars = resolved(record)
        begin, tiles, pages, _ = prepare(inbox, record, region, values, bars)
        boundaries = len(tiles) + len(pages) + 2  # begin, records, commit
        for cut in range(1, boundaries + 1):
            candidate = deepcopy(record); candidate['layout']['title'] = f'Interrupted {cut}'
            values, bars = resolved(candidate)
            sent, acknowledged = 0, []
            async def dropping(message):
                nonlocal sent
                answer = await raw(message)
                if message['op'] in ('begin', 'page', 'tile', 'commit'):
                    sent += 1; acknowledged.append((message['op'], answer['applied']))
                    if sent == cut: raise TimeoutError('Deliberately lost acknowledgement')
                return answer
            interrupted = Sender(dropping)
            try:
                await interrupted.synchronize(inbox, candidate, region, values, bars)
                raise AssertionError('Interruption did not happen')
            except TimeoutError: pass
            assert all(not applied for op, applied in acknowledged if op != 'commit'), acknowledged
            recovery = Sender(raw)
            await recovery.synchronize(inbox, candidate, region, values, bars)
            assert (await status(recovery))['applied']
        passed(f'Lost acknowledgments at all {boundaries} configuration boundaries recover without another save')
        sender = recovery
        old_session = sender.session
        await sender.probe()
        stale = await raw({'v': 2, 'op': 'begin', 'session': old_session, 'seq': 999, 'rev': sender.confirmed, **begin})
        assert stale['status'] == 'Error: obsolete message'
        passed('Delayed previous-session packets cannot replace the document')
        sender.confirmed = None
        await sender.synchronize(inbox, candidate, region, values, bars)
        revision = configuration(candidate, region)
        packet = {'v': 2, 'session': sender.session, 'seq': sender.sequence + 1, 'rev': revision, 'op': 'ping'}
        original = await raw(packet); repeat = await raw(packet)
        assert original == repeat
        changed = await raw({**packet, 'op': 'commit'})
        assert changed['status'] == 'Error: obsolete message'
        sender.sequence += 1
        passed('Exact packet retry is idempotent; changed packet with the same sequence is refused')
        initial = prepare(inbox, candidate, region, values, bars)[1][0]
        invalid = await raw({**initial, 'op': 'state', 'v': 2, 'session': sender.session, 'seq': sender.sequence + 1, 'rev': revision})
        assert invalid['status'].startswith('Error: outdated'), invalid
        passed('Live state cannot overwrite tile placement or options')
        old = await raw({'v': 1, 'op': 'layout', 'title': 'Old add-on', 'entities': []})
        assert old['status'] == 'Configuration problem. Update add-on.'
        restored = Sender(raw)
        await restored.synchronize(inbox, candidate, region, values, bars)
        assert (await status(restored))['applied']
        passed('Old add-on messages are refused and the new sender resumes its saved document')
        result = {'checks': checks, 'duration_seconds': round(time.monotonic() - started, 2), 'compiled': info.compilation_time}
        if args.output: args.output.write_text(json.dumps(result, indent=2) + '\n')
        print(f'{len(checks)} real-firmware protocol checks passed', flush=True)
    finally:
        await client.disconnect()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('host', 'name', 'mac'): parser.add_argument('--' + name, required=True)
    parser.add_argument('--secrets', required=True, type=Path)
    parser.add_argument('--output', type=Path)
    asyncio.run(run(parser.parse_args()))
