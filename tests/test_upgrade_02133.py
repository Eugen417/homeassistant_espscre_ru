"""Upgrade the main 0.2.133 JSON store through Manager's real startup order."""
import asyncio
import json
from pathlib import Path
import sys
import tempfile
from types import SimpleNamespace
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'screen_manager/app'))
from server import Manager
from page_layout import compile_tiles, grid_of_record


class Upgrade02133(unittest.TestCase):
    def test_discovery_finishes_migration_and_keeps_usable_tiles(self):
        # Same envelope and writer options as main 0.2.133; no shape sensor,
        # installed profile, network contact or mocked conversion function.
        cases = {
            'plain': [{'entity': 'light.a', 'name': '', 'slot': 0}],
            'future': [{'entity': 'light.a', 'slot': 0, 'options': {'future_option': True}}],
            'outside': [{'entity': 'light.a', 'slot': 0}, {'entity': 'light.b', 'slot': 80}],
            'duplicate': [{'entity': 'light.a', 'slot': 0}, {'entity': 'light.a', 'slot': 1}],
            'overlap': [{'entity': 'light.a', 'slot': 0}, {'entity': 'light.b', 'slot': 0}],
            'nan': [{'entity': 'light.a', 'slot': 0}, {'entity': 'light.b', 'slot': float('nan')}],
            'future_outside': [{'entity': 'light.a', 'slot': 0}, {'entity': 'light.b', 'slot': 80, 'options': {'future_option': True}}],
            'bad_options': [{'entity': 'light.a', 'slot': 0}, {'entity': 'light.b', 'slot': 1, 'options': {'display': 'unknown'}}],
        }
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'screens.json'
            path.write_text(json.dumps({'version': 1, 'screens': {
                'text.' + name: {'title': 'Screen', 'tiles': tiles} for name, tiles in cases.items()}}))
            original = path.read_bytes()
            ha = SimpleNamespace(online=False, registry=[], states={}, devices=[], areas=[], changed=asyncio.Event())
            manager = Manager(ha, path)
            self.assertTrue(all(r['format'] == 'legacy-v1' for r in manager.store.records().values()))
            ha.registry = [{'entity_id': 'text.' + name, 'platform': 'esphome', 'original_name': 'Tile settings',
                            'device_id': name} for name in cases]
            ha.states = {'text.' + name: {'state': 'Ready'} for name in cases}
            manager.screens()
            records = manager.refresh_page_records()
            for name in cases:
                with self.subTest(name=name):
                    record = records['text.' + name]
                    self.assertEqual(record['format'], 'pages-v2')
                    self.assertEqual(record['sourceGrid'], {'columns': 2, 'rows': 3})
                    tiles = compile_tiles(record['layout'], grid_of_record(record))
                    self.assertEqual([tile['entity'] for tile in tiles], ['light.a'])
                    self.assertEqual(len(record.get('migration', {}).get('droppedTiles', [])),
                                     0 if name in ('plain', 'future') else 1)
            self.assertEqual(path.with_name('screens.v1.backup.json').read_bytes(), original)
            self.assertEqual(Manager(ha, path).store.records(), records)
