"""A remote as a tile (firmware 0.22.0, GitHub #117): its card switches it on and off and lists its activities, as Home
Assistant's dialog does, and its keys are tiles that perform remote.send_command. tests/test_tile_controls.cpp checks
the firmware's routes and colours; these keep the app, the firmware and the editor in step with each other and with Home
Assistant (2026.10 core and frontend: remote/icons.json, remote/services.yaml, state_color.ts, more-info-remote.ts).
"""
from firmware_sources import firmware_domains, runtime_source
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'screen_manager/app'))
import catalogue  # noqa: E402
import core  # noqa: E402
import ha_catalogue  # noqa: E402
import header_bar  # noqa: E402
import tile_icons  # noqa: E402

COMPONENT = ROOT / 'components/smart_display'
MODEL = (COMPONENT / 'runtime_model.h').read_text()
CONTROLS = (COMPONENT / 'tile_controls.h').read_text()
RECEIVER = (COMPONENT / 'page_receiver.cpp').read_text()
TILES = runtime_source()
CARD = (ROOT / 'web/src/components/TileCard.vue').read_text()
PALETTE = (ROOT / 'web/src/model/tile-palette.ts').read_text()
LIBRARY = (ROOT / 'web/src/components/Library.vue').read_text()

ACTIONS = {'remote.turn_on', 'remote.turn_off', 'remote.toggle', 'remote.send_command', 'remote.learn_command',
           'remote.delete_command'}


def layout(entity, **options):
    return {'title': 'Home', 'tiles': [{'entity': entity, 'slot': 0, **({'options': options} if options else {})}]}


class TheApp(unittest.TestCase):
    def test_a_remote_is_a_tile_and_a_top_bar_item(self):
        self.assertIn('remote', core.DOMAINS)
        self.assertTrue(core.entity_id('remote.living_room'))
        core.validate_header({'items': [{'type': 'entity', 'entity': 'remote.living_room'}]})
        # The top bar colours it as Home Assistant's badges do: amber while on, the calm grey while off.
        self.assertEqual(header_bar.accent('remote.a', {'state': 'on'}), header_bar.AMBER)
        self.assertIsNone(header_bar.accent('remote.a', {'state': 'off'}))
        self.assertEqual(header_bar.DOMAIN_ICONS['remote'], 'remote')

    def test_a_layout_with_one_waits_for_firmware_0_22_0(self):
        self.assertEqual(catalogue.of_type('remote')['firmware'], '0.22.0')
        self.assertEqual(core.min_firmware(layout('remote.a')), (0, 22, 0))
        self.assertEqual(core.FIRMWARE_VERSION, '0.22.0')
        # Older firmware refuses the domain; this one takes it.
        self.assertIn('remote', firmware_domains())

    def test_taps_and_the_wide_card_switch(self):
        for tap in ('auto', 'toggle', 'none', 'detail'):
            core.validate_layout(layout('remote.a', tap=tap))
        with self.assertRaises(ValueError):
            core.validate_layout(layout('remote.a', tap='run'))
        caps = ha_catalogue.capabilities('remote.a', ACTIONS, {'state': 'on', 'attributes': {}}, {})
        self.assertTrue(caps['toggle'])
        self.assertEqual(caps['controls'], ['toggle'])
        self.assertEqual(core.resolve_controls({'entity': 'remote.a', 'options': {'size': 'wide'}}), 'toggle')

    def test_a_key_is_perform_action_with_send_command(self):
        # Home Assistant lists no commands, so a key is a tile of its own: remote.send_command with a typed command.
        tile = core.validate_layout(layout('remote.a', tap='action', action={'action': 'remote.send_command',
                                                                               'data': {'command': 'play', 'device': 'bluray'}}))['tiles'][0]
        self.assertEqual(tile['options']['tap'], 'action')
        self.assertEqual(tile['options']['action']['action'], 'remote.send_command')

    def test_the_activities_reach_the_screen(self):
        activities = [f'Activity {n}' for n in range(20)]
        states = {'remote.hub': {'state': 'on', 'attributes': {'supported_features': 4, 'current_activity': 'Watch TV',
                                                                'activity_list': ['Watch TV', *activities]}}}
        attributes = core.state_message(0, {'entity': 'remote.hub', 'name': ''}, states)['a']
        self.assertEqual(attributes['current_activity'], 'Watch TV')
        # Sixteen at most, as a select's options: the card pages through them.
        self.assertEqual(len(attributes['activity_list']), 16)
        self.assertEqual(attributes['activity_list'][0], 'Watch TV')
        # A remote without activities (Broadlink, Apple TV) sends neither.
        plain = core.state_message(0, {'entity': 'remote.ir', 'name': ''}, {'remote.ir': {'state': 'on', 'attributes': {}}})['a']
        self.assertNotIn('activity_list', plain)
        self.assertIn('if (a["activity_list"].is<JsonArray>())', RECEIVER)
        self.assertIn('next.activity=string(a["current_activity"],48);', RECEIVER)
        self.assertIn('activity.empty()', MODEL)


class HomeAssistantsWay(unittest.TestCase):
    def test_icons(self):
        # remote/icons.json: mdi:remote, mdi:remote-off while off.
        self.assertEqual((tile_icons.GLYPHS['remote'], tile_icons.GLYPHS['remote-off']), ('F0454', 'F0EC4'))
        self.assertEqual(tile_icons.DEFAULTS['remote'], 'remote')
        self.assertIn('if (d == "remote" && tile.state == "off") return "\\U000F0EC4";', TILES)
        self.assertIn('if (d == "remote") return "\\U000F0454";', TILES)
        self.assertIn('remote-off', tile_icons.BIG_GLYPHS)

    def test_colours(self):
        # state_color.ts colours a remote by state with no variable of its own: on is --state-active-color (amber).
        self.assertIn('d == "automation" || d == "remote" || d == "timer"', CONTROLS)
        self.assertIn('"automation", "remote", "timer", "camera"].includes(domain)) return c.AMBER', PALETTE)

    def test_the_card_is_home_assistants_dialog(self):
        # more-info-remote.ts: on and off, and the activities, each of which is remote.turn_on with `activity`.
        self.assertIn('d == "remote";', CONTROLS)
        self.assertIn('render_remote_detail(t,large,width,height,pad,top,bar,bar_x,bar_y);', TILES)
        self.assertIn('action("remote.turn_on",t.entity,"activity",option);', TILES)
        self.assertIn('render_select_detail(t,large,width,height,pad,top,on?t.extra().activity:std::string());', TILES)
        # The power key: in the top bar beside the activities, alone and large in the middle without them.
        self.assertIn('lv_obj_move_to_index(key,2);', TILES)
        self.assertIn('if(t.extra().options.empty()){', TILES)
        # The tile names the activity it runs, on the screen and in the editor's mockup.
        self.assertIn('value = t.extra().activity;', TILES)
        self.assertIn('if (domain.value === "remote" && c.state === "on" && a.current_activity)', CARD)
        self.assertIn('"remote"', LIBRARY)


if __name__ == '__main__':
    unittest.main()
