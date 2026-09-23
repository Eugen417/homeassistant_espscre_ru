"""Binary sensors in Home Assistant's words (app 0.2.62 / firmware 0.2.53): a door tile says Open or Closed.

The screen maps the device class itself (`tile_controls::binary_state_text`), from the `device_class` attribute every
state message has carried since app 0.2.23: nothing new on the wire, so it works with older apps too. These tests keep
its table equal to the add-on's `header_bar.BINARY_STATES`, which the top bar and the history card use.
"""
import json
import re
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import profiles  # noqa: E402
sys.path.insert(0, str(ROOT / 'screen_manager/app'))
import header_bar  # noqa: E402
import history_card  # noqa: E402
from core import state_message  # noqa: E402

CONTROLS = (ROOT / 'components/smart_display/tile_controls.h').read_text()
TILES = (ROOT / 'components/smart_display/runtime_tiles.h').read_text()
PROFILES = ('checkout/guition.yaml', 'checkout/cyd.yaml', 'packages/cyd.yaml', 'packages/guition.yaml')


ENGLISH = json.loads((ROOT / 'screen_manager/translations/en.json').read_text(encoding='utf-8'))


def firmware_words():
    """The firmware's table (app 0.2.90: keys into screen.ha.binary, in the screen's language) in English."""
    block = re.search(r'constexpr BinaryWords BINARY_WORDS\[\] = \{(.*?)\n\};', CONTROLS, re.S)
    words = ENGLISH['screen']['ha']['binary']
    found = {}
    for device_class, on, off in re.findall(r'\{"(\w+)", screen_text::txt::ha_binary_(\w+), screen_text::txt::ha_binary_(\w+)\}', block.group(1)):
        found[device_class] = (words[on], words[off])
    return found


class BinaryWords(unittest.TestCase):
    def test_the_screen_carries_the_add_on_s_words(self):
        self.assertEqual(firmware_words(), header_bar.BINARY_STATES)

    def test_top_bar_and_history_card_say_the_same(self):
        for device_class, (on, off) in header_bar.BINARY_STATES.items():
            attrs = {'device_class': device_class}
            for raw, words in (('on', on), ('off', off)):
                self.assertEqual(header_bar.value('binary_sensor.x', {'state': raw, 'attributes': attrs}), (words, None))
                self.assertEqual(history_card.state_label('binary_sensor', raw, attrs), words)
        # Without a class, or with one Home Assistant adds later, all of them say On and Off.
        for attrs in ({}, {'device_class': 'future_class'}):
            self.assertEqual(header_bar.value('binary_sensor.x', {'state': 'on', 'attributes': attrs}), ('On', None))
            self.assertEqual(history_card.state_label('binary_sensor', 'off', attrs), 'Off')
        self.assertIn('return screen_text::tr(on ? screen_text::txt::ha_on : screen_text::txt::ha_off);', CONTROLS)
        self.assertEqual((ENGLISH['screen']['ha']['on'], ENGLISH['screen']['ha']['off']), ('On', 'Off'))

    def test_the_state_message_already_carries_the_class(self):
        tile = {'entity': 'binary_sensor.front_door', 'name': ''}
        states = {'binary_sensor.front_door': {'state': 'on', 'attributes': {'device_class': 'door', 'friendly_name': 'Front door'}}}
        message = state_message(0, tile, states)
        self.assertEqual((message['state'], message['a']['device_class']), ('on', 'door'))
        self.assertNotIn('x', message)
        self.assertIn('tile.device_class=string(a["device_class"],24);', TILES)
        self.assertLessEqual(max(map(len, header_bar.BINARY_STATES)), 24)

    def test_tile_card_and_heading_use_them_for_binary_sensors_only(self):
        self.assertIn('else if (d == "binary_sensor" && (value == "on" || value == "off")) '
                      'value = tile_controls::binary_state_text(t.device_class, value == "on");', TILES)
        self.assertIn('if(t.domain()=="binary_sensor"&&(t.state=="on"||t.state=="off"))'
                      'return tile_controls::binary_state_text(t.device_class,t.state=="on");', TILES)
        # A card still waiting for its history must not take the words of the card opened before it.
        self.assertIn('if(history.entity==t.entity)for(const auto &pair:history.words)', TILES)

    def test_the_fonts_carry_every_word(self):
        letters = set(''.join(on + off for on, off in header_bar.BINARY_STATES.values()))
        self.assertLessEqual(letters, set(header_bar.GLYPHS))
        for name in PROFILES:
            text = profiles.resolved(name)
            for font in ('headline', 'watch_value', 'label', 'sublabel', 'sublabel_big'):
                block = re.search(r'id: ' + font + r'\n    size: \d+\n    bpp: 4\n    glyphs: \[(.*?)\]\n', text, re.S)
                self.assertIsNotNone(block, (name, font))
                glyphs = {a or b for a, b in re.findall(r"'([^'])'|\"(')\"", block.group(1))}
                self.assertLessEqual(letters, glyphs, (name, font))


if __name__ == '__main__':
    unittest.main()
