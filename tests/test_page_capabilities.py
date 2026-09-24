"""Offline hints survive restart, but never authorize a transport session."""
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import AsyncMock, patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'screen_manager/app'))
from page_capabilities import CapabilityCache
from page_delivery import Sender


class CapabilityCacheTests(unittest.TestCase):
    def test_restart_offline_then_replacement_and_downgrade(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'capabilities.json'
            screen = dict(device_id='test-device', firmware_known='0.3.1', node='test', board='guition', shape={'columns': 2, 'rows': 3})
            sender = Sender(AsyncMock())
            sender.protocol, sender.tile_sizes = 2, {'single', 'square'}
            cache = CapabilityCache(path)
            cache.remember('text.test', screen, sender)
            cached = path.read_bytes()
            with patch.object(cache, '_save') as save:
                cache.remember('text.test', screen, sender)
                save.assert_not_called()
            restarted = CapabilityCache(path)
            offline = Sender(AsyncMock())
            restarted.restore('text.test', {'device_id': 'test-device', 'firmware_known': '0.3.1'}, offline)
            self.assertEqual(offline.last_protocol, 2)
            self.assertEqual(offline.last_tile_sizes, {'single', 'square'})
            self.assertIsNone(offline.protocol)
            self.assertIsNone(offline.session)
            self.assertIsNone(offline.confirmed)
            for replacement in ({'device_id': 'other'}, {'firmware_known': '0.2.133'}, {'shape': {'columns': 1, 'rows': 4}}, {'node': 'other'}):
                unknown = Sender(AsyncMock())
                restarted.restore('text.test', {**screen, **replacement}, unknown)
                self.assertIsNone(unknown.last_protocol)
            self.assertEqual(path.read_bytes(), cached)
            sender.protocol, sender.tile_sizes = 1, {'single', 'wide', 'full'}
            restarted.remember('text.test', screen, sender)
            downgraded = Sender(AsyncMock())
            CapabilityCache(path).restore('text.test', screen, downgraded)
            self.assertEqual(downgraded.last_protocol, 1)
            self.assertNotIn('square', downgraded.last_tile_sizes)
            restarted.forget('text.test')
            self.assertEqual(CapabilityCache(path).records, {})

    def test_untrusted_or_missing_cache_only_restricts_editor(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'capabilities.json'
            for contents in ('broken', 'null', '[]', '{"version":99,"screens":{}}',
                             json.dumps({'version': 1, 'screens': {'text.test': {'identity': {'device_id': 'test'}, 'protocol': 2, 'sizes': [None]}}})):
                path.write_text(contents)
                sender = Sender(AsyncMock())
                CapabilityCache(path).restore('text.test', {'device_id': 'test'}, sender)
                self.assertIsNone(sender.last_protocol)
