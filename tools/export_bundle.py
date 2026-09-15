"""Export an allowlisted, neutral starter ZIP (never copies local secrets/logs)."""
import argparse
from pathlib import Path
import re
import zipfile

ROOT = Path(__file__).resolve().parents[1]
FILES = ['.gitignore', 'README.md', 'AGENTS.md', 'requirements.txt', 'device.example.yaml',
         'calibration.example.yaml', 'secrets.yaml.example', 'cyd_ui.h', 'light_controls.h', 'LICENSE',
         'guition_diagnostics.h', 'guition-4848s040.yaml', 'guition-device.example.yaml', 'TILE_CONFIGURATION.md', 'CYD_STABILITY.md', 'TEST_RESULTS.md', 'repository.yaml']
FOLDERS = ['docs', 'tools', 'tests', 'components', 'diagnostics', 'fonts']
SUFFIXES = {'.py', '.cpp', '.h', '.md', '.ttf', '.txt'}


def neutral_base(root):
    defaults = '\n'.join((root / name).read_text() for name in ['device.example.yaml', 'calibration.example.yaml'])
    replacements = dict(re.findall(r'^  (\w+): ("[^"\n]*")', defaults, re.M))
    base = (root / 'home-like-2432s028.yaml').read_text()
    def replace(match):
        key = match[1]
        return f'  {key}: {replacements.get(key, match[2])}'
    return re.sub(r'^  (\w+): ("[^"\n]*")', replace, base, flags=re.M)


def export(root, output):
    root, output = Path(root).resolve(), Path(output)
    paths = [root / name for name in FILES]
    for folder in FOLDERS:
        for path in (root / folder).rglob('*'):
            if path.is_file() and '__pycache__' not in path.parts and (path.suffix in SUFFIXES or path.name == 'LICENSE'):
                paths.append(path)
    # Dedicated distribution directories only; never recursively include owner YAML.
    for folder in ['screen_manager', 'packages', 'installers']:
        for path in (root / folder).rglob('*'):
            if path.is_file() and '__pycache__' not in path.parts and (path.suffix in SUFFIXES | {'.yaml', '.html', '.js', '.css'} or path.name == 'Dockerfile'):
                paths.append(path)
    for path in paths:
        if path.is_symlink() or not path.resolve().is_relative_to(root):
            raise ValueError(f'No files outside the source folder or symlinks allowed: {path}')
        if not path.is_file():
            raise ValueError(f'Missing bundle file: {path}')
    output.parent.mkdir(parents=True, exist_ok=True)
    # Exclusive mode refuses to replace a previously delivered bundle.
    with zipfile.ZipFile(output, 'x', zipfile.ZIP_DEFLATED) as archive:
        archive.writestr('cyd-display/home-like-2432s028.yaml', neutral_base(root))
        for path in sorted(set(paths)):
            archive.write(path, 'cyd-display/' + path.relative_to(root).as_posix())
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'dist' / 'cyd-starter.zip')
    args = parser.parse_args()
    try:
        print(f'Share this ZIP: {export(ROOT, args.output)}')
        print('No local secrets, device.yaml, calibration measurements, logs or firmware backups included.')
    except (OSError, ValueError) as exc:
        parser.exit(1, f'Error: {exc}\n')


if __name__ == '__main__':
    main()
