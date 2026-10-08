"""Which way a screen is to hang, standing up or lying down (app 0.4.85, firmware 0.53.0+).

Chosen beside the mockup in the editor, for a screen whose glass is not square: the layout is laid out on the grid of
that way and goes to the screen with it (begin's "upright"), which turns, starts again and keeps the way. Kept here per
Home Assistant device until the screen has it and after, so a screen whose preferences were wiped (a USB flash) is turned
back the next time its layout goes out. A screen nobody turned has no entry: it hangs the way it was built.
Kept apart from the saved layouts, whose records an older app reads with a fixed set of fields."""
import json
import logging
import os
import tempfile

LOG = logging.getLogger(__name__)
WAYS = ('landscape', 'portrait')


class ScreenHang:
    def __init__(self, path):
        self.path = path
        try:
            data = json.loads(path.read_text())
            ways = data.get('ways') if isinstance(data, dict) and data.get('version') == 1 else None
        except (OSError, ValueError):
            ways = None
        self.ways = {str(k): v for k, v in (ways or {}).items() if v in WAYS}

    def get(self, device):
        return self.ways.get(device) if device else None

    def set(self, device, way):
        """Keep `way` ('landscape' or 'portrait') for the screen of `device`. True when it changed."""
        if not device or way not in WAYS or self.ways.get(device) == way:
            return False
        self.ways[device] = way
        self._save()
        return True

    def forget(self, device):
        if device and self.ways.pop(device, None) is not None:
            self._save()

    def _save(self):
        temporary = None
        try:
            self.path.parent.mkdir(parents=True, exist_ok=True)
            with tempfile.NamedTemporaryFile(mode='w', dir=self.path.parent, delete=False) as handle:
                temporary = handle.name
                json.dump({'version': 1, 'ways': self.ways}, handle)
                handle.flush()
                os.fsync(handle.fileno())
            os.replace(temporary, self.path)
        except OSError as error:
            LOG.warning('Could not keep which way the screens hang (%s)', type(error).__name__)
        finally:
            if temporary and os.path.exists(temporary):
                os.unlink(temporary)
