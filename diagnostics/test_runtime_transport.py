"""Verify invalid runtime commands are rejected by an actual screen; no HA actions."""
import argparse
import asyncio
import base64
import json
from pathlib import Path
import secrets

import yaml
from aioesphomeapi import APIClient, TextInfo, TextState

async def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host', required=True)
    parser.add_argument('--name', required=True)
    parser.add_argument('--secrets', type=Path, default=Path(__file__).resolve().parents[1] / 'secrets.yaml')
    args = parser.parse_args()
    key = yaml.safe_load(args.secrets.read_text())['api_encryption_key']
    client = APIClient(args.host, 6053, noise_psk=key, expected_name=args.name, client_info='Runtime protocol test')
    await client.connect(login=True)
    try:
        entities, _ = await client.list_entities_services()
        inbox = next(e for e in entities if isinstance(e, TextInfo) and e.name == 'Tegelinstellingen')
        assert inbox.max_length == 255
        queue = asyncio.Queue()
        client.subscribe_states(lambda s: queue.put_nowait(s.state) if isinstance(s, TextState) and s.key == inbox.key else None)
        messages = [
            {'v': 99, 'op': 'layout', 'title': 'Rejected', 'entities': []},
            {'v': 1, 'op': 'layout', 'title': 'Rejected', 'entities': ['lock.not_allowed']},
            {'v': 1, 'op': 'state', 'i': 99, 'entity': 'light.not_a_slot', 'a': {}},
        ]
        for message in messages:
            encoded = base64.b64encode(json.dumps(message).encode()).decode()
            assert len(encoded) <= 200
            client.text_command(inbox.key, secrets.token_hex(6) + '|0|1|' + encoded)
            while True:
                ack = await asyncio.wait_for(queue.get(), 5)
                if ack.startswith('Fout:'):
                    print('PASS: rejected invalid command — ' + ack)
                    break
        client.text_command(inbox.key, secrets.token_hex(6) + '|2|1|e30=')
        while True:
            ack = await asyncio.wait_for(queue.get(), 5)
            if ack == 'Fout: onvolledig bericht':
                print('PASS: missing chunks rejected')
                break
    finally:
        await client.disconnect()

if __name__ == '__main__':
    asyncio.run(main())
