"""Open/close the isolated touch diagnostic screen via encrypted ESPHome API."""
import argparse
import asyncio
from pathlib import Path
import yaml
from aioesphomeapi import APIClient, LogLevel

async def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('action', choices=['touch_diagnostics', 'end_touch_diagnostics', 'ui_state', 'light_controls_preview'])
    parser.add_argument('--host', default='cyd-2432s028.local')
    parser.add_argument('--name', default='cyd-2432s028', help='Verwachte DEVICE_NAME')
    parser.add_argument('--secrets', type=Path, default=Path(__file__).resolve().parents[1] / 'secrets.yaml')
    args = parser.parse_args()
    secret = yaml.safe_load(args.secrets.read_text())
    client = APIClient(args.host, 6053, noise_psk=secret['api_encryption_key'], expected_name=args.name, client_info='CYD touch diagnostics')
    await client.connect(login=True)
    try:
        def log(msg):
            text = msg.message.decode(errors='replace') if isinstance(msg.message, bytes) else msg.message
            if 'ui_diag' in text:
                print(text, flush=True)
        client.subscribe_logs(log, log_level=LogLevel.LOG_LEVEL_DEBUG, dump_config=False)
        _, services = await client.list_entities_services()
        await client.execute_service(next(s for s in services if s.name == args.action), {})
        await asyncio.sleep(0.5)
        print(args.action + ': sent')
    finally:
        await client.disconnect()

if __name__ == '__main__':
    asyncio.run(main())
