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
        raise ValueError('Gebruik een unieke naam van 1–31 tekens: kleine letters, cijfers en koppeltekens.')
    if board not in ("cyd", "guition"):
        raise ValueError("Onbekend bordprofiel")
    import json
    device = (ROOT / ('guition-device.example.yaml' if board == 'guition' else 'device.example.yaml')).read_text().replace('"guition-new"' if board == 'guition' else '"cyd-new"', json.dumps(name), 1)
    device = device.replace('"Mijn Guition"' if board == 'guition' else '"Mijn CYD"', json.dumps(friendly_name, ensure_ascii=False), 1)
    secret_text = (ROOT / 'secrets.yaml.example').read_text()
    replacements = [base64.b64encode(secrets.token_bytes(32)).decode(), secrets.token_urlsafe(24), secrets.token_urlsafe(18)]
    for value in replacements:
        secret_text = secret_text.replace('WORDT_UNIEK_GEGENEREERD', value, 1)
    files = {'guition-device.yaml' if board == 'guition' else 'device.yaml': device, 'secrets.yaml': secret_text}
    if board == 'cyd': files['calibration.yaml'] = (ROOT / 'calibration.example.yaml').read_text()
    directory = Path(directory)
    if not directory.is_dir():
        raise ValueError('De doelmap moet al bestaan en de uitgepakte code bevatten.')
    for name in files:
        if (directory / name).exists():
            raise ValueError(f'{directory / name} bestaat al; niets overschreven. Gebruik een nieuwe kopie van de map.')
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
    parser.add_argument('--name', required=True, help='Unieke ESPHome-naam, bijvoorbeeld display-keuken')
    parser.add_argument('--friendly-name')
    parser.add_argument('--directory', type=Path, default=ROOT)
    args = parser.parse_args()
    args.friendly_name = args.friendly_name or ('Mijn Guition' if args.board == 'guition' else 'Mijn CYD')
    try:
        for path in create(args.directory, args.name, args.friendly_name, args.board):
            print(f'Gemaakt: {path}')
        print('Vul wifi lokaal in secrets.yaml in. Sleutels zijn uniek gegenereerd en worden niet getoond.')
        print('Volg docs/GUITION.md; flash guition-device.yaml.' if args.board == 'guition' else 'Volg README.md; flash device.yaml.')
    except (ValueError, OSError) as exc:
        parser.exit(1, f'Fout: {exc}\n')


if __name__ == '__main__':
    main()
