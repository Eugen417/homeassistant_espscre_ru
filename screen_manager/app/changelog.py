"""What's new for the Update badge (app 0.2.73): the CHANGELOG sections since a screen's firmware.

The Dockerfile copies CHANGELOG.md next to this file; a development checkout has it one folder up.
Each `## <app> (firmware <fw>)` section gives its bullet lines as plain text, newest first.
"""
import re
from pathlib import Path

HEADER = re.compile(r'^## (\d+\.\d+\.\d+) \(firmware (\d+\.\d+\.\d+)\)')
BOLD = re.compile(r'\*\*(.+?)\*\*')
CODE = re.compile(r'`([^`]*)`')
LINK = re.compile(r'\[([^\]]+)\]\([^)]+\)')


def load(path=None, limit=20):
    """[{app, firmware, lines}] for the latest sections; [] without a changelog."""
    candidates = [Path(path)] if path else [Path(__file__).with_name('CHANGELOG.md'), Path(__file__).parent.parent / 'CHANGELOG.md']
    for candidate in candidates:
        if candidate.exists():
            return parse(candidate.read_text(), limit)
    return []


def parse(text, limit=20):
    sections = []
    for line in text.splitlines():
        match = HEADER.match(line)
        if match:
            if len(sections) >= limit:
                break
            sections.append({'app': match.group(1), 'firmware': match.group(2), 'lines': []})
        elif sections and line.startswith('- '):
            sections[-1]['lines'].append(plain(line[2:]))
    return sections


def plain(markdown):
    text = BOLD.sub(r'\1', markdown)
    text = CODE.sub(r'\1', text)
    text = LINK.sub(r'\1', text)
    return text.strip()
