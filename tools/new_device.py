"""Create local device, calibration and secrets files without overwriting any."""
import argparse
import base64
import os
from pathlib import Path
import re
import secrets

ROOT = Path(__file__).resolve().parents[1]


def create(directory, name, friendly_name, board="cyd"):
    if not re.fullmatch(r'[a-z][a-z0-9-]{0,30}', name) or name.endswith('-'):
        raise ValueError('Use a unique name of 1-31 characters: lowercase letters, digits, and dashes.')
    if board not in ("cyd", "guition"):
        raise ValueError("Unknown board profile")
    import json
    device = (ROOT / ('guition-device.example.yaml' if board == 'guition' else 'device.example.yaml')).read_text().replace('"guition-new"' if board == 'guition' else '"cyd-new"', json.dumps(name), 1)
    device = device.replace('"My Guition"' if board == 'guition' else '"My CYD"', json.dumps(friendly_name, ensure_ascii=False), 1)
    secret_text = (ROOT / 'secrets.yaml.example').read_text()
    replacements = [base64.b64encode(secrets.token_bytes(32)).decode(), secrets.token_urlsafe(24), secrets.token_urlsafe(18)]
    for value in replacements:
        secret_text = secret_text.replace('WILL_BE_UNIQUELY_GENERATED', value, 1)
    files = {'guition-device.yaml' if board == 'guition' else 'device.yaml': device, 'secrets.yaml': secret_text}
    if board == 'cyd': files['calibration.yaml'] = (ROOT / 'calibration.example.yaml').read_text()
    directory = Path(directory)
    if not directory.is_dir():
        raise ValueError('The target folder must already exist and contain the unpacked code.')
    for name in files:
        if (directory / name).exists():
            raise ValueError(f'{directory / name} already exists; nothing overwritten. Use a fresh copy of the folder.')
    created = []
    try:
        for name, contents in files.items():
            path = directory / name
            fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
            created.append(path)
            with os.fdopen(fd, 'w', encoding='utf-8') as handle:
                handle.write(contents)
    except Exception:
        for path in created:
            path.unlink(missing_ok=True)
        raise
    return created


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--board', choices=['cyd', 'guition'], default='cyd')
    parser.add_argument('--name', required=True, help='Unique ESPHome name, for example display-kitchen')
    parser.add_argument('--friendly-name')
    parser.add_argument('--directory', type=Path, default=ROOT)
    args = parser.parse_args()
    args.friendly_name = args.friendly_name or ('My Guition' if args.board == 'guition' else 'My CYD')
    try:
        for path in create(args.directory, args.name, args.friendly_name, args.board):
            print(f'Created: {path}')
        print('Fill in Wi-Fi locally in secrets.yaml. Keys are uniquely generated and are not shown.')
        print('Follow docs/GUITION.md; flash guition-device.yaml.' if args.board == 'guition' else 'Follow README.md; flash device.yaml.')
    except (ValueError, OSError) as exc:
        parser.exit(1, f'Error: {exc}\n')


if __name__ == '__main__':
    main()
