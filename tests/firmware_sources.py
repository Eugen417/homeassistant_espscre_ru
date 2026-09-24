"""Expand local implementation includes for existing firmware source contracts."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / 'components/smart_display'

def runtime_source():
    text = (ROOT / 'runtime_tiles.h').read_text()
    return text.replace('#include "page_receiver.h"', (ROOT / 'page_receiver.h').read_text())
