import json
import sys
import tempfile
import unittest
from pathlib import Path
import yaml
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'screen_manager/app'))
from firmware import Firmware

class WifiReuse(unittest.TestCase):
    def test_existing_wifi_reused_without_browser_values(self):
        with tempfile.TemporaryDirectory() as tmp:
            f = Firmware(tmp, tmp)
            path = Path(tmp) / 'secrets.yaml'
            original = '# keep comments\nwifi_ssid: example-private-network\nwifi_password: example-private-password\nother_secret: leave-me\n'
            path.write_text(original)
            self.assertEqual(f.wifi_status()['state'], 'ready')
            self.assertNotIn('example-private', json.dumps(f.status()))
            f.create({'board': 'guition', 'name': 'second-screen', 'friendly_name': 'Second'})
            self.assertEqual(path.read_text(), original)
            profile = f.profile('second-screen.yaml').read_text()
            self.assertIn('ssid: !secret wifi_ssid', profile)
            self.assertIn('password: !secret wifi_password', profile)
            self.assertNotIn('example-private', profile)

    def test_missing_keys_are_added_without_touching_other_secrets(self):
        with tempfile.TemporaryDirectory() as tmp:
            f = Firmware(tmp, tmp)
            path = Path(tmp) / 'secrets.yaml'
            original = '# keep comments\nother: keep\nwifi_ssid: ""\n'
            path.write_text(original)
            self.assertEqual(f.wifi_status(), {'state': 'missing', 'missing': ['wifi_password', 'wifi_ssid']})
            # Without values nothing is written: no profile, secrets untouched.
            with self.assertRaises(ValueError):
                f.create({'board': 'cyd', 'name': 'screen', 'friendly_name': 'Screen'})
            self.assertEqual(path.read_text(), original)
            self.assertFalse((Path(tmp) / 'screen.yaml').exists())
            f.create({'board': 'cyd', 'name': 'screen', 'friendly_name': 'Screen',
                      'wifi_ssid': 'home: net', 'wifi_password': 'p\\ss "word"'})
            text = path.read_text()
            self.assertTrue(text.startswith('# keep comments\nother: keep\n'))
            self.assertEqual(text.count('wifi_ssid'), 1)
            self.assertEqual(yaml.safe_load(text), {'other': 'keep', 'wifi_ssid': 'home: net', 'wifi_password': 'p\\ss "word"'})
            self.assertEqual(f.wifi_status()['state'], 'ready')
            self.assertTrue((Path(tmp) / 'screen.yaml').exists())

    def test_invalid_secrets_are_never_rewritten(self):
        with tempfile.TemporaryDirectory() as tmp:
            f = Firmware(tmp, tmp)
            path = Path(tmp) / 'secrets.yaml'
            path.write_text('wifi_ssid: [invalid')
            with self.assertRaises(ValueError):
                f.create({'board': 'cyd', 'name': 'screen', 'friendly_name': 'Screen',
                          'wifi_ssid': 'net', 'wifi_password': 'pw'})
            self.assertEqual(path.read_text(), 'wifi_ssid: [invalid')
            self.assertFalse((Path(tmp) / 'screen.yaml').exists())

    def test_install_checks_port_and_busy_slot_before_writing(self):
        with tempfile.TemporaryDirectory() as tmp:
            f = Firmware(tmp, tmp)
            data = {'board': 'cyd', 'name': 'screen', 'friendly_name': 'Screen', 'wifi_ssid': 'net', 'wifi_password': 'pw'}
            with self.assertRaises(ValueError):
                f.install({**data, 'target': '/dev/ttyUSB0'})
            self.assertFalse((Path(tmp) / 'screen.yaml').exists())
            self.assertFalse((Path(tmp) / 'secrets.yaml').exists())
            result = f.install(data)
            self.assertEqual(result['file'], 'screen.yaml')
            self.assertEqual(len(result['api_key']), 44)
            self.assertNotIn('job', result)
            self.assertIn(result['api_key'], f.profile('screen.yaml').read_text())
            meta = f.profile_names()['screen.yaml']
            self.assertEqual((meta['screen'], meta['api_key']), (True, result['api_key']))
            (Path(tmp) / 'other.yaml').write_text('esphome:\n  name: other\napi:\n  encryption:\n    key: !secret k\n')
            self.assertEqual((f.profile_names()['other.yaml']['screen'], f.profile_names()['other.yaml']['api_key']), (False, None))

    def test_first_install_and_invalid_file(self):
        with tempfile.TemporaryDirectory() as tmp:
            f = Firmware(tmp, tmp)
            self.assertEqual(f.wifi_status()['state'], 'new')
            f.create({'board': 'cyd', 'name': 'screen', 'friendly_name': 'Screen',
                      'wifi_ssid': 'network', 'wifi_password': ''})
            self.assertEqual(f.wifi_status()['state'], 'ready')
            (Path(tmp) / 'secrets.yaml').write_text('wifi_ssid: [invalid')
            self.assertEqual(f.wifi_status(), {'state': 'invalid'})
