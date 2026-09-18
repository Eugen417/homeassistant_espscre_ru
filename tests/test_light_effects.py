"""A light's effects page (app 0.2.83 / firmware 0.2.70): effects, palettes, presets, speed and intensity of a WLED.

- The tile's state message carries the light's `effect` and the rows of its device: the selects and numbers Home
  Assistant lists for it, named in English as its frontend names them, with its icons, in a fixed preference order.
- A picker asks for its names when it opens (`esphome.screen_options`); the app answers one page per message, under
  the screen's 4096-byte limit, the effects alphabetically with "Solid" first, a select's options as Home Assistant
  lists them, and only for a light on the screen's layout or a select on such a light's device.
The values below are what Home Assistant 2026.9 reports for a WLED 16.0.1 in a Dutch installation.
"""
import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'screen_manager/app'))
import light_effects  # noqa: E402
import tile_icons  # noqa: E402
from core import ATTRS, encode, extras, state_message  # noqa: E402

LIGHT = 'light.vuelta'
EFFECTS = ['Solid', 'Blink', 'Breathe', 'Wipe', 'Wipe Random', 'Random Colors', 'Sweep', 'Dynamic', 'Colorloop', 'Rainbow',
           'TV Simulator', 'Dynamic Smooth', 'PS Volcano', 'Color Clouds', 'Slow Transition', 'Akemi', 'Sparkle+', 'Noise 1']
PALETTES = ['* Color 1', '* Color Gradient', 'Analogous', 'April Night', 'Aurora', 'Default', 'Rainbow']


def wled():
    """(states, device registry entries, registry index) of a WLED as Home Assistant reports it in Dutch."""
    states = {
        LIGHT: {'state': 'on', 'attributes': {'friendly_name': 'VUELTA', 'supported_color_modes': ['rgb'], 'effect': 'TV Simulator',
                                              'effect_list': EFFECTS, 'brightness': 255, 'supported_features': 36, 'icon': 'mdi:lamp'}},
        'select.vuelta_kleurenpalet': {'state': 'Aurora', 'attributes': {'friendly_name': 'VUELTA Kleurenpalet', 'options': PALETTES}},
        'select.vuelta_voorinstelling': {'state': 'unknown', 'attributes': {'friendly_name': 'VUELTA Voorinstelling', 'options': ['Lava lamp', 'Moving blue', 'Simple White', 'nice1']}},
        'select.vuelta_afspeellijst': {'state': 'unknown', 'attributes': {'friendly_name': 'VUELTA Afspeellijst', 'options': ['test']}},
        'select.vuelta_live_override': {'state': '0', 'attributes': {'friendly_name': 'VUELTA Live override', 'options': ['0', '1', '2']}},
        'number.vuelta_snelheid': {'state': '128', 'attributes': {'friendly_name': 'VUELTA Snelheid', 'min': 0, 'max': 255, 'step': 1}},
        'number.vuelta_intensiteit': {'state': '64', 'attributes': {'friendly_name': 'VUELTA Intensiteit', 'min': 0, 'max': 255, 'step': 1}},
        'switch.vuelta_nachtlampje': {'state': 'off', 'attributes': {'friendly_name': 'VUELTA Nachtlampje'}},
        'sensor.vuelta_ip': {'state': '192.168.146.106', 'attributes': {}},
    }
    keys = {LIGHT: ('segment', None), 'select.vuelta_kleurenpalet': ('color_palette', 'config'), 'select.vuelta_voorinstelling': ('preset', None),
            'select.vuelta_afspeellijst': ('playlist', None), 'select.vuelta_live_override': ('live_override', 'config'),
            'number.vuelta_snelheid': ('speed', 'config'), 'number.vuelta_intensiteit': ('intensity', 'config'),
            'switch.vuelta_nachtlampje': ('nightlight', 'config'), 'sensor.vuelta_ip': ('ip', 'diagnostic')}
    device = [{'entity_id': eid, 'platform': 'wled', 'device_id': 'wled1', 'translation_key': key, 'entity_category': category,
               'original_name': states[eid]['attributes'].get('friendly_name', '').replace('VUELTA ', '') or None}
              for eid, (key, category) in keys.items()]
    return states, device, {item['entity_id']: item for item in device}


WORDS = {'component.wled.entity.select.color_palette.name': 'Color palette', 'component.wled.entity.select.preset.name': 'Preset',
         'component.wled.entity.select.playlist.name': 'Playlist', 'component.wled.entity.number.speed.name': 'Speed',
         'component.wled.entity.number.intensity.name': 'Intensity', 'component.wled.entity.light.segment.name': 'Segment {segment}'}
ICONS = {'entity': {'wled': {'number': {'speed': {'default': 'mdi:speedometer'}},
                             'select': {'color_palette': {'default': 'mdi:palette-outline'}, 'playlist': {'default': 'mdi:play-speed'},
                                        'preset': {'default': 'mdi:playlist-play'}}}}}


def icon_of(eid, state, attrs, entry):
    return tile_icons.ha_icon(attrs) or tile_icons.default_glyph(eid, state, attrs, entry, ICONS)


class RowsOfTheDevice(unittest.TestCase):
    def test_the_light_gets_its_effect_and_the_rows_of_its_device(self):
        states, device, index = wled()
        self.assertIn('effect', ATTRS)
        extra = extras({'entity': LIGHT}, states, device=device, entries=index, words=WORDS, icon_of=icon_of, device_name='VUELTA')
        self.assertEqual([r['n'] for r in extra['rows']], ['Color palette', 'Preset', 'Playlist'])
        self.assertEqual([r['e'] for r in extra['rows']], ['select.vuelta_kleurenpalet', 'select.vuelta_voorinstelling', 'select.vuelta_afspeellijst'])
        self.assertEqual([(r['s'], r['c']) for r in extra['rows']], [('Aurora', 7), ('unknown', 4), ('unknown', 1)])
        self.assertEqual([r['i'] for r in extra['rows']], ['F0E0C', 'F0411', 'F08FF'])
        self.assertEqual(extra['nums'], [{'e': 'number.vuelta_snelheid', 'n': 'Speed', 'lo': 0, 'hi': 255, 'st': 1, 'v': 128, 'i': 'F04C5'},
                                         {'e': 'number.vuelta_intensiteit', 'n': 'Intensity', 'lo': 0, 'hi': 255, 'st': 1, 'v': 64}])
        message = state_message(0, {'entity': LIGHT, 'name': ''}, states, extra)
        self.assertEqual(message['a']['effect'], 'TV Simulator')
        self.assertNotIn('effect_list', message['a'])
        self.assertLess(len(encode(message).encode()), 1200)
        # The live override select (options 0/1/2), the switches and the diagnostic sensor stay off the page.
        self.assertEqual(light_effects.related(LIGHT, device, states), ('select.vuelta_kleurenpalet', 'select.vuelta_voorinstelling',
                                                                          'select.vuelta_afspeellijst', 'number.vuelta_snelheid', 'number.vuelta_intensiteit'))

    def test_names_fall_back_to_home_assistants_own_without_the_device_name(self):
        states, device, index = wled()
        extra = extras({'entity': LIGHT}, states, device=device, entries=index, words={}, device_name='VUELTA')
        self.assertEqual([r['n'] for r in extra['rows']], ['Kleurenpalet', 'Voorinstelling', 'Afspeellijst'])
        self.assertEqual([r['n'] for r in extra['nums']], ['Snelheid', 'Intensiteit'])
        self.assertNotIn('i', extra['rows'][0])
        # A name with a placeholder is no name ("Segment {segment}").
        self.assertEqual(light_effects.entity_name(LIGHT, index[LIGHT], states, WORDS), 'VUELTA')

    def test_hidden_disabled_and_empty_entities_are_left_out(self):
        states, device, index = wled()
        index['select.vuelta_kleurenpalet']['hidden_by'] = 'user'
        index['number.vuelta_snelheid']['disabled_by'] = 'integration'
        states['select.vuelta_afspeellijst']['attributes']['options'] = []
        extra = extras({'entity': LIGHT}, states, device=device, entries=index, words=WORDS)
        self.assertEqual([r['e'] for r in extra['rows']], ['select.vuelta_voorinstelling'])
        self.assertEqual([r['e'] for r in extra['nums']], ['number.vuelta_intensiteit'])
        # A light without such a device has no rows and no extra block.
        self.assertIsNone(extras({'entity': 'light.kitchen'}, {'light.kitchen': {'state': 'on', 'attributes': {}}}, device=[]))

    def test_at_most_three_selects_and_two_numbers_in_preference_order(self):
        states, device, index = wled()
        for n in range(4):
            eid = f'select.vuelta_extra_{n}'
            states[eid] = {'state': 'a', 'attributes': {'friendly_name': f'VUELTA Zzz {n}', 'options': ['a', 'b']}}
            device.append({'entity_id': eid, 'platform': 'wled', 'device_id': 'wled1', 'translation_key': f'extra_{n}', 'original_name': f'Zzz {n}'})
            eid = f'number.vuelta_more_{n}'
            states[eid] = {'state': '1', 'attributes': {'friendly_name': f'VUELTA More {n}', 'min': 0, 'max': 10}}
            device.append({'entity_id': eid, 'platform': 'wled', 'device_id': 'wled1', 'translation_key': f'more_{n}', 'original_name': f'More {n}'})
        extra = extras({'entity': LIGHT}, states, device=device, entries=index, words=WORDS)
        self.assertEqual([r['n'] for r in extra['rows']], ['Color palette', 'Preset', 'Playlist'])
        self.assertEqual([r['n'] for r in extra['nums']], ['Speed', 'Intensity'])


class ThePickersNames(unittest.TestCase):
    def test_effects_are_alphabetical_with_solid_first(self):
        states, _, _ = wled()
        names = light_effects.effect_options(states[LIGHT]['attributes'])
        self.assertEqual(names[:4], ['Solid', 'Akemi', 'Blink', 'Breathe'])
        self.assertEqual(names[-3:], ['TV Simulator', 'Wipe', 'Wipe Random'])
        self.assertEqual(len(names), len(EFFECTS))
        self.assertEqual(light_effects.effect_options({'effect_list': ['candle', 'off']}), ['candle', 'off'])
        self.assertEqual(light_effects.effect_options({}), [])

    def test_only_a_light_on_the_layout_or_a_select_of_its_device_is_answered(self):
        states, device, _ = wled()
        device_of = lambda light: device if light == LIGHT else []  # noqa: E731
        self.assertEqual(light_effects.options_for(LIGHT, states, [LIGHT], device_of)[0], 'Solid')
        self.assertIsNone(light_effects.options_for(LIGHT, states, ['light.other'], device_of))
        self.assertEqual(light_effects.options_for('select.vuelta_kleurenpalet', states, [LIGHT], device_of), PALETTES)
        self.assertIsNone(light_effects.options_for('select.vuelta_live_override', states, [LIGHT], device_of))
        self.assertIsNone(light_effects.options_for('select.vuelta_kleurenpalet', states, ['light.other'], device_of))
        self.assertIsNone(light_effects.options_for('switch.vuelta_nachtlampje', states, [LIGHT], device_of))
        # A light without the EFFECT feature answers nothing, whatever its attributes say.
        states[LIGHT]['attributes']['supported_features'] = 32
        self.assertIsNone(light_effects.options_for(LIGHT, states, [LIGHT], device_of))

    def test_pages_stay_under_the_message_limit(self):
        names = [f'Effect number {n} with a longer name' for n in range(400)]
        pages = light_effects.pages(names)
        self.assertGreater(len(pages), 1)
        self.assertEqual([n for page in pages for n in page], names)
        for i, page in enumerate(pages):
            self.assertLess(len(encode(light_effects.message(LIGHT, i, pages)).encode()), 4096)
        message = light_effects.message(LIGHT, 99, pages)
        self.assertEqual((message['op'], message['i'], message['n']), ('options', len(pages) - 1, len(pages)))
        # WLED's 216 effects fit in one page.
        wled_names = [f'Effect {n}' for n in range(216)]
        self.assertEqual(len(light_effects.pages(wled_names)), 1)
        self.assertEqual(light_effects.pages([]), [[]])
        self.assertEqual(light_effects.message(LIGHT, 0, light_effects.pages([]))['o'], [])

    def test_the_answer_is_what_the_firmware_parses(self):
        message = light_effects.message('select.x', 0, light_effects.pages(['A', 'B']))
        self.assertEqual(json.loads(encode(message)), {'v': 1, 'op': 'options', 'e': 'select.x', 'i': 0, 'n': 1, 'o': ['A', 'B']})


if __name__ == '__main__':
    unittest.main()
