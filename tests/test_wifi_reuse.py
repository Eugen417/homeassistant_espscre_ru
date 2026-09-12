import json
import sys
import tempfile
import unittest
from pathlib import Path
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

    def test_missing_keys_do_not_overwrite_existing_secrets(self):
        with tempfile.TemporaryDirectory() as tmp:
            f = Firmware(tmp, tmp)
            path = Path(tmp) / 'secrets.yaml'
            path.write_text('other: keep\n')
            self.assertEqual(f.wifi_status()['state'], 'missing')
            with self.assertRaises(ValueError):
                f.create({'board': 'cyd', 'name': 'screen', 'friendly_name': 'Screen',
                          'wifi_ssid': 'replacement', 'wifi_password': 'replacement'})
            self.assertEqual(path.read_text(), 'other: keep\n')
            self.assertFalse((Path(tmp) / 'screen.yaml').exists())

    def test_first_install_and_invalid_file(self):
        with tempfile.TemporaryDirectory() as tmp:
            f = Firmware(tmp, tmp)
            self.assertEqual(f.wifi_status()['state'], 'new')
            f.create({'board': 'cyd', 'name': 'screen', 'friendly_name': 'Screen',
                      'wifi_ssid': 'network', 'wifi_password': ''})
            self.assertEqual(f.wifi_status()['state'], 'ready')
            (Path(tmp) / 'secrets.yaml').write_text('wifi_ssid: [invalid')
            self.assertEqual(f.wifi_status(), {'state': 'invalid'})
