"""Observe a screen over its encrypted API; does not wake it or operate HA devices."""
import argparse
import asyncio
from pathlib import Path
import time
import yaml
from aioesphomeapi import APIClient, LogLevel

async def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host', required=True)
    parser.add_argument('--name', required=True)
    parser.add_argument('--seconds', type=int, default=720)
    parser.add_argument('--secrets', type=Path, default=Path(__file__).resolve().parents[1] / 'secrets.yaml')
    args = parser.parse_args()
    if not 1 <= args.seconds <= 86400:
        parser.error('--seconds must be between 1 and 86400')
    key = yaml.safe_load(args.secrets.read_text())['api_encryption_key']
    client = APIClient(args.host, 6053, noise_psk=key, expected_name=args.name,
                       client_info='Read-only display observation')
    await client.connect(login=True)
    try:
        info = await client.device_info()
        print(f'Connected: {info.name}, firmware {info.project_version}, {info.compilation_time}', flush=True)
        _, actions = await client.list_entities_services()
        state_action = next(action for action in actions if action.name == 'ui_state')
        def log(message):
            line = message.message.decode(errors='replace') if isinstance(message.message, bytes) else message.message
            if '[ui_diag' in line or '[health' in line:
                print(line, flush=True)
        client.subscribe_logs(log, log_level=LogLevel.LOG_LEVEL_DEBUG, dump_config=False)
        end = time.monotonic() + args.seconds
        while time.monotonic() < end:
            await client.execute_service(state_action, {})
            await asyncio.sleep(min(30, max(0, end - time.monotonic())))
            # Confirms a live API response, not just a local connection flag.
            await client.device_info()
        print('Observation completed. Physical display quality still requires visual inspection.', flush=True)
    finally:
        await client.disconnect()

if __name__ == '__main__':
    asyncio.run(main())
