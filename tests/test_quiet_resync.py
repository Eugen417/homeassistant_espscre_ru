"""A colour slider on Studio 1 turned red on release (app 0.2.55, firmware 0.2.47).

An automation set a screen setting to the value it already had on every light change. The screen reported
it, ESP Screen Manager saved the layout and resent the whole screen, and the screen drew its whole page
again while a finger was on a slider. Touch input stalled, and the GT911's stray (0, 0) contact became
the finger's last position, so LVGL moved the slider to its end on release. Each link is checked here.
"""
from firmware_sources import runtime_source
from manager_fixtures import with_screen_grid
import asyncio
import importlib.util
import re
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import profiles  # noqa: E402
sys.path.insert(0, str(ROOT / 'screen_manager/app'))
sys.path.insert(0, str(ROOT / 'tests'))
from core import validate_settings  # noqa: E402

PROFILES = ('checkout/cyd.yaml', 'checkout/guition.yaml', 'packages/cyd.yaml', 'packages/guition.yaml')
RUNTIME = runtime_source()
LIGHT_CONTROLS = (ROOT / 'components/smart_display/light_controls.h').read_text()


@unittest.skipUnless(importlib.util.find_spec('aiohttp'), 'Run using .venv-portal/bin/python for server tests')
class SettingEvents(unittest.IsolatedAsyncioTestCase):
    async def run_loop_for(self, manager, seconds):
        task = asyncio.create_task(manager.run())
        await asyncio.sleep(seconds)
        task.cancel()
        try:
            await task
        except asyncio.CancelledError:
            pass

    async def test_a_setting_the_screen_already_has_is_not_saved_or_resent(self):
        import test_scaling
        from server import Manager
        with tempfile.TemporaryDirectory() as tmp:
            ha = test_scaling.fake_ha(firmware='0.2.47')
            ha.setting_events = []
            m = Manager(with_screen_grid(ha), Path(tmp) / 'screens.json')
            m.save('text.screen', {'title': 'Studio', 'tiles': [{'entity': 'light.a', 'name': ''}],
                                   'settings': validate_settings({'standby_seconds': 86400})})
            ha.changed.set()
            await self.run_loop_for(m, 0.6)
            self.assertEqual([msg['op'] for _, msg, _ in ha.messages], ['layout', 'header', 'state'], 'first pass sends everything')
            ha.messages.clear()
            stored = m.store.get('text.screen')
            persisted = m.path.read_bytes()
            # An automation sets "Standby after" to 86400 on every light change; the screen reports it.
            ha.setting_events.append({'inbox': 'text.screen', 'key': 'standby_seconds', 'value': '86400'})
            ha.changed.set()
            await self.run_loop_for(m, 0.6)
            self.assertEqual(ha.messages, [], 'nothing changed: no resend of the whole screen')
            self.assertEqual(m.store.get('text.screen'), stored, 'and no saved revision change')
            self.assertEqual(m.path.read_bytes(), persisted, 'and no storage rewrite')
            self.assertEqual(ha.setting_events, [], 'the event is handled all the same')
            # A real change is kept (app 0.2.57), and not sent back: the screen has it already, and an echo that
            # arrives after the screen stepped further would undo that step (a held + on its settings page).
            rev = m.sent['text.screen']['rev']
            ha.setting_events.append({'inbox': 'text.screen', 'key': 'standby_seconds', 'value': '600'})
            ha.changed.set()
            await self.run_loop_for(m, 0.6)
            self.assertEqual(m.layouts['text.screen']['settings']['standby_seconds'], 600)
            self.assertEqual(ha.messages, [], 'nothing goes back to the screen')
            self.assertEqual(m.sent['text.screen']['layout']['settings']['standby_seconds'], 600, 'what the screen holds follows')
            self.assertEqual(m.sent['text.screen']['rev'], rev, 'the ping keeps the revision the screen has')
            # So is a brightness that pulls the standby brightness down with it.
            ha.setting_events.append({'inbox': 'text.screen', 'key': 'brightness', 'value': '10'})
            ha.changed.set()
            await self.run_loop_for(m, 0.6)
            settings = m.layouts['text.screen']['settings']
            self.assertEqual((settings['brightness'], settings['standby_brightness']), (10, 10))
            self.assertEqual(ha.messages, [])
            # The hourly repeat carries the kept values, so the screen and the stored layout agree.
            await m.sync_one('text.screen', m.layouts['text.screen'], force=True, screen=m.screen('text.screen'), dirty=set())
            layout = next(msg for _, msg, _ in ha.messages if msg['op'] == 'layout')
            self.assertEqual((layout['settings']['brightness'], layout['settings']['standby_seconds']), (10, 600))


class Firmware(unittest.TestCase):
    def setUp(self):
        self.profiles = {name: profiles.text(name) for name in PROFILES}

    def test_home_assistant_setting_the_same_value_writes_and_reports_nothing(self):
        # Firmware 0.2.49+: the setting entities change a value through settings_screen::set(), which stores,
        # applies and reports only a real change (tests/test_settings_screen.cpp checks that).
        screen = (ROOT / 'components/smart_display/settings_screen.h').read_text()
        self.assertRegex(screen, r'return SetResult::same;\n  changed\(key\.c_str\(\), reported\);')
        for name, text in self.profiles.items():
            for key in ('brightness', 'standby_brightness', 'night_brightness', 'standby_seconds'):
                self.assertIn(f"set_action:\n      - lambda: 'settings_screen::set(\"{key}\", (int32_t) lround(x));'", text, f'{name}: {key}')
            self.assertNotIn('runtime_tiles::setting_event("', text, f'{name}: settings are reported through set() only')

    def test_a_repeated_layout_does_not_redraw_the_page(self):
        block = RUNTIME[RUNTIME.index('if (op == "begin")'):RUNTIME.index('if (op == "ping")')]
        unchanged = block.index('if (begin == page_protocol::Begin::unchanged)')
        self.assertLess(unchanged, block.index('cancel_layout_input();'))
        self.assertLess(unchanged, block.index('model.begin('))
        commit = RUNTIME[RUNTIME.index('if (op == "commit")'):RUNTIME.index('if (op == "camera")')]
        self.assertLess(commit.index('if (transfer.active && model.ready())'), commit.index('layout_changed()'))

    def test_the_guition_hides_the_stray_gt911_contact_from_lvgl_and_the_edge_swipe(self):
        for name in ('checkout/guition.yaml', 'packages/guition.yaml'):
            text = self.profiles[name]
            self.assertIn('screen_input::ghost_touch.filter(', text, name)
            self.assertIn('lv_indev_set_read_cb(indev, filtered);', text, name)
            self.assertIn('runtime_tiles::touch_input::moved(', text, f'{name}: the shared handler sees every contact')
        for name in ('checkout/cyd.yaml', 'packages/cyd.yaml'):
            self.assertNotIn('ghost_touch', self.profiles[name], 'the CYD has its own XPT2046 filter')
        # The skip itself lives in the shared tree now (runtime_tiles::touch_input::moved), so every board that
        # uses it gets it: a board cannot take half of the handling any more, which is how a GT911 board once
        # ended up with on_touch and neither of the other two triggers.
        self.assertRegex(RUNTIME, r'if \(x == 0 && y == 0\) return;\s+screen_input::touch_guard\.update')

    def test_a_slider_that_jumps_on_release_is_logged(self):
        self.assertIn('screen_input::release_jump(row.held, value,', LIGHT_CONTROLS)
        self.assertIn('screen_input::release_jump(slider_held,raw,', RUNTIME)


if __name__ == '__main__':
    unittest.main()
