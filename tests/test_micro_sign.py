"""One mu on the wire (app 0.2.134): Home Assistant's Greek letter becomes the micro sign the screens draw."""
import re
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'screen_manager/app'))
import header_bar  # noqa: E402
from core import one_mu, short, state_message  # noqa: E402

MU, MICRO = 'μ', 'µ'
TEXT_FONTS = ('headline', 'watch_value', 'label', 'sublabel', 'sublabel_big')


class MicroSignTests(unittest.TestCase):
    def test_the_greek_letter_folds_to_the_micro_sign_and_the_micro_sign_stays(self):
        self.assertEqual(one_mu(f'{MU}g/m³'), f'{MICRO}g/m³')
        self.assertEqual(short(f'{MU}s', 48), f'{MICRO}s')
        self.assertEqual(short(f'{MICRO}s', 48), f'{MICRO}s')

    def test_a_tile_carries_the_micro_sign_in_unit_name_state_and_options(self):
        states = {'sensor.dust': {'state': f'12 {MU}', 'attributes': {
            'unit_of_measurement': f'{MU}g/m³', 'friendly_name': f'Fine dust ({MU}g)', 'options': [f'{MU}A', 'mA']}}}
        message = state_message(0, {'entity': 'sensor.dust', 'name': ''}, states)
        self.assertEqual(message['a']['unit_of_measurement'], f'{MICRO}g/m³')
        self.assertEqual(message['name'], f'Fine dust ({MICRO}g)')
        self.assertEqual(message['a']['options'], [f'{MICRO}A', 'mA'])
        self.assertNotIn(MU, str(message))

    def test_the_screens_glyphs_hold_the_micro_sign_and_never_needed_the_greek_letter(self):
        self.assertIn(MICRO, header_bar.GLYPHS)
        self.assertIn(MICRO, header_bar.LEGACY_GLYPHS)
        self.assertNotIn(MU, header_bar.GLYPHS)
        text = (ROOT / 'packages/core.yaml').read_text()
        for font in TEXT_FONTS:
            block = re.search(r'id: ' + font + r'\n    size: [^\n]+\n    bpp: 4\n    glyphs: \[([^\]]*)\]', text)
            self.assertIsNotNone(block, font)
            self.assertIn(f"'{MICRO}'", block.group(1), font)
            self.assertNotIn(f"'{MU}'", block.group(1), font)


if __name__ == '__main__':
    unittest.main()
