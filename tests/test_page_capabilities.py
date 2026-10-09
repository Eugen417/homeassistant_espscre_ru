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
            screen = dict(device_id='test-device', firmware_known='0.3.1', node='test', board='guition',
                          shape={'width': 480, 'height': 480, 'dpi': 170, 'look': 'standard', 'columns': 2, 'rows': 3})
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
            other_glass = {'width': 800, 'height': 480, 'dpi': 217, 'look': 'standard', 'columns': 3, 'rows': 3}
            for replacement in ({'device_id': 'other'}, {'firmware_known': '0.2.133'}, {'shape': other_glass}, {'node': 'other'}):
                unknown = Sender(AsyncMock())
                restarted.restore('text.test', {**screen, **replacement}, unknown)
                self.assertIsNone(unknown.last_protocol)
            # Another grid on the same glass is the same screen (firmware 0.53.0+: ESP Screens gives it one while it runs).
            regridded = Sender(AsyncMock())
            restarted.restore('text.test', {**screen, 'shape': {**screen['shape'], 'columns': 2, 'rows': 4}}, regridded)
            self.assertEqual(regridded.last_protocol, 2)
            self.assertEqual(path.read_bytes(), cached)
            sender.protocol, sender.tile_sizes = 1, {'single', 'wide', 'full'}
            restarted.remember('text.test', screen, sender)
            downgraded = Sender(AsyncMock())
            CapabilityCache(path).restore('text.test', screen, downgraded)
            self.assertEqual(downgraded.last_protocol, 1)
            self.assertNotIn('square', downgraded.last_tile_sizes)
            restarted.forget('text.test')
            self.assertEqual(CapabilityCache(path).records, {})

    def test_the_grids_a_screen_said_are_kept_for_editing_offline(self):
        # Firmware 0.53.0+: the grids a screen takes, so a grid can be chosen while it is offline.
        grids = {'upright': False, 'landscape': {'columns': 2, 'rows': 3, 'min': [1, 1], 'max': [3, 5]},
                 'portrait': {'columns': 2, 'rows': 3, 'min': [1, 1], 'max': [3, 5]}}
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'capabilities.json'
            screen = dict(device_id='test-device', firmware_known='0.53.0', node='test', board='guition')
            sender = Sender(AsyncMock())
            sender.protocol, sender.tile_sizes, sender.grids = 2, {'single'}, grids
            CapabilityCache(path).remember('text.test', screen, sender)
            offline = Sender(AsyncMock())
            CapabilityCache(path).restore('text.test', screen, offline)
            self.assertEqual(offline.last_grids, grids)
            self.assertIsNone(offline.grids)
            # A record with broken grids restores none.
            data = json.loads(path.read_text())
            data['screens']['text.test']['grids'] = {'upright': False}
            path.write_text(json.dumps(data))
            broken = Sender(AsyncMock())
            CapabilityCache(path).restore('text.test', screen, broken)
            self.assertIsNone(broken.last_grids)

    def test_untrusted_or_missing_cache_only_restricts_editor(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'capabilities.json'
            for contents in ('broken', 'null', '[]', '{"version":99,"screens":{}}',
                             json.dumps({'version': 1, 'screens': {'text.test': {'identity': {'device_id': 'test'}, 'protocol': 2, 'sizes': [None]}}})):
                path.write_text(contents)
                sender = Sender(AsyncMock())
                CapabilityCache(path).restore('text.test', {'device_id': 'test'}, sender)
                self.assertIsNone(sender.last_protocol)
