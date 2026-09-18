"""The packages a screen builds from: one shared core plus a board file, runtime tiles only (app 0.2.84+)."""
import importlib.util
import re
import subprocess
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import profiles  # noqa: E402

BOARDS = {'cyd': 'home-like-2432s028.yaml', 'guition': 'guition-4848s040.yaml'}
# The old manual profile bound tiles to fixed entities in the YAML. ESP Screens sends the tiles now, so
# none of this may come back: the fixed subscriptions, its tap handlers and scripts, its own vacuum card.
GONE = ['ha_state_tile1', 'tile1_brightness', 'tile6_climate_humidity', 'tile6_cover_state', 'smartdisplay_action',
        'publish_action', 'do_tile_action', 'open_vacuum_overlay', 'vacuum_action', 'vacuum_fan_speed', 'vacuum_overlay',
        'vacspd_quiet', 'vacuum_actions', 'DYNAMIC_TILES', 'TILE_COUNT', 'DIRECT_ACTIONS', 'TILE1_ENTITY',
        'light_controls::subscribe']


def load_checker():
    spec = importlib.util.spec_from_file_location('check_packages', ROOT / 'tools/check_packages.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class PackageTests(unittest.TestCase):
    def test_the_packages_fit_together(self):
        result = subprocess.run([sys.executable, str(ROOT / 'tools/check_packages.py')], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_every_entry_is_the_shared_core_plus_its_board(self):
        checker = load_checker()
        for board, profile in BOARDS.items():
            for name in (profile, f'packages/{board}.yaml'):
                self.assertEqual(sorted(checker.included(ROOT / name)), sorted(['packages/core.yaml', str(profiles.BOARDS[board].relative_to(ROOT))]), name)

    def test_the_published_entry_takes_everything_from_github(self):
        for board in BOARDS:
            package = (ROOT / 'packages' / f'{board}.yaml').read_text()
            self.assertNotIn('!secret', package)
            self.assertNotIn('type: local', package)
            self.assertIn('url: https://github.com/MaxGramser/homeassistant_espscreen.git', package)
            self.assertIn('FONT_DIR: "https://raw.githubusercontent.com/MaxGramser/homeassistant_espscreen/main/fonts"', package)
            # The fonts of the shared core come from that place; a checkout takes them from fonts/.
            self.assertIn('file: "${FONT_DIR}/Roboto-500.ttf"', profiles.CORE.read_text())
            self.assertIn('FONT_DIR: "fonts"', (ROOT / BOARDS[board]).read_text())

    def test_no_secret_or_local_path_under_packages(self):
        for path in sorted((ROOT / 'packages').rglob('*.yaml')):
            text = '\n'.join(line for line in path.read_text().split('\n') if not line.lstrip().startswith('#'))
            self.assertNotIn('!secret', text, path.name)
            self.assertNotIn('type: local', text, path.name)
            for section in ('sensor', 'binary_sensor', 'text_sensor'):
                block = re.search(r'^' + section + r':\n.*?(?=^[a-zA-Z_]+:|\Z)', text, re.M | re.S)
                self.assertFalse(block and re.search(r'^  - platform: homeassistant$', block[0], re.M), f'{path.name}: a {section} subscribes to Home Assistant')

    def test_the_manual_profile_is_gone_from_profiles_and_packages(self):
        for board, name in BOARDS.items():
            for entry in (name, f'packages/{board}.yaml'):
                text = '\n'.join(line for line in profiles.text(entry).split('\n') if not line.lstrip().startswith('#'))
                for key in GONE:
                    self.assertNotIn(key, text, f'{entry} still carries {key}')

    def test_runtime_tiles_keep_what_they_bind_and_open(self):
        for board in BOARDS:
            package = profiles.text(f'packages/{board}.yaml')
            for key in ['runtime_tiles::bind(9, id(tile10)', 'runtime_tiles::enabled = true;', 'id: open_value_overlay',
                        'id: ui_refresh', 'runtime_tiles::render(id(lbl_room));', 'id: climate_detail_overlay',
                        'id: color_detail_overlay']:
                self.assertIn(key, package, board)
        # The CYD keeps its resistive calibration on the screen itself; the Guition's GT911 needs none.
        self.assertIn('screen_calibration::setup(', profiles.resolved('packages/cyd.yaml'))
        self.assertNotIn('screen_calibration::setup(', profiles.resolved('packages/guition.yaml'))

    def test_the_core_carries_no_board_number_and_every_board_the_same_names(self):
        core = profiles.CORE.read_text()
        for needle in ('esp32:', 'platform: xpt2046', 'platform: gt911', 'platform: st7701s', 'platform: mipi_spi', 'psram:',
                       'set_raw_correction', 'camera_image', 'GPIO'):
            self.assertNotIn(needle, core, f'{needle} is a board\'s, not the core\'s')
        names = {board: set(profiles.substitutions_of(path)) for board, path in profiles.BOARDS.items()}
        shared = set.intersection(*names.values())
        self.assertGreater(len(shared), 100)
        for board, defined in names.items():
            own = defined - shared
            self.assertTrue(all(re.match(r'(TOUCH_AFFINE_|TOUCH_CAL_|EDGE_SWIPE_|ALERT_\w*IMAGE|CAMERA_)', n) for n in own), f'{board}: {sorted(own)}')

    def test_the_checker_refuses_a_fixed_home_assistant_subscription(self):
        source = (ROOT / 'tools/check_packages.py').read_text()
        self.assertIn("re.search(r'^  - platform: homeassistant$', match[0], re.M)", source)
        self.assertIn("for section in ('sensor', 'binary_sensor', 'text_sensor'):", source)


if __name__ == '__main__':
    unittest.main()
