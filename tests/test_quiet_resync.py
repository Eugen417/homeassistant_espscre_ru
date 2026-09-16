"""A colour slider on Studio 1 turned red on release (app 0.2.55, firmware 0.2.47).

An automation set a screen setting to the value it already had on every light change. The screen reported
it, ESP Screen Manager saved the layout and resent the whole screen, and the screen drew its whole page
again while a finger was on a slider. Touch input stalled, and the GT911's stray (0, 0) contact became
the finger's last position, so LVGL moved the slider to its end on release. Each link is checked here.
"""
import asyncio
import importlib.util
import re
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'screen_manager/app'))
sys.path.insert(0, str(ROOT / 'tests'))
from core import validate_settings  # noqa: E402

PROFILES = ('home-like-2432s028.yaml', 'guition-4848s040.yaml', 'packages/cyd.yaml', 'packages/guition.yaml')
RUNTIME = (ROOT / 'components/smart_display/runtime_tiles.h').read_text()
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
            m = Manager(ha, Path(tmp) / 'screens.json')
            m.save('text.screen', {'title': 'Studio', 'tiles': [{'entity': 'light.a', 'name': ''}],
                                   'settings': validate_settings({'standby_seconds': 86400})})
            ha.changed.set()
            await self.run_loop_for(m, 0.6)
            self.assertEqual([msg['op'] for _, msg, _ in ha.messages], ['layout', 'header', 'state'], 'first pass sends everything')
            ha.messages.clear()
            stored = m.layouts['text.screen']
            # An automation sets "Standby after" to 86400 on every light change; the screen reports it.
            ha.setting_events.append({'inbox': 'text.screen', 'key': 'standby_seconds', 'value': '86400'})
            ha.changed.set()
            await self.run_loop_for(m, 0.6)
            self.assertEqual(ha.messages, [], 'nothing changed: no resend of the whole screen')
            self.assertIs(m.layouts['text.screen'], stored, 'and no save')
            self.assertEqual(ha.setting_events, [], 'the event is handled all the same')
            # A real change is still saved and sent.
            ha.setting_events.append({'inbox': 'text.screen', 'key': 'standby_seconds', 'value': '600'})
            ha.changed.set()
            await self.run_loop_for(m, 0.6)
            self.assertEqual(m.layouts['text.screen']['settings']['standby_seconds'], 600)
            self.assertIn('layout', [msg['op'] for _, msg, _ in ha.messages])
            # So is a brightness that pulls the standby brightness down with it.
            ha.messages.clear()
            ha.setting_events.append({'inbox': 'text.screen', 'key': 'brightness', 'value': '10'})
            ha.changed.set()
            await self.run_loop_for(m, 0.6)
            settings = m.layouts['text.screen']['settings']
            self.assertEqual((settings['brightness'], settings['standby_brightness']), (10, 10))


class Firmware(unittest.TestCase):
    def setUp(self):
        self.profiles = {name: (ROOT / name).read_text() for name in PROFILES}

    def test_home_assistant_setting_the_same_value_writes_and_reports_nothing(self):
        for name, text in self.profiles.items():
            for key in ('brightness', 'standby_brightness', 'night_brightness', 'standby_seconds'):
                action = re.search(r'const auto before = s;\n(?:.*\n){1,6}?\s+if \(s == before\) return;\n'
                                   r'\s+if \(runtime_tiles::enabled\) runtime_tiles::settings_preference\.save\(&s\);\n'
                                   rf'\s+runtime_tiles::setting_event\("{key}",', text)
                self.assertTrue(action, f'{name}: {key} saves or reports a value it already has')

    def test_a_repeated_layout_does_not_redraw_the_page(self):
        block = RUNTIME[RUNTIME.index('if (op == "layout")'):RUNTIME.index('if (op == "ping")')]
        guard = re.search(r'if \((.*)\) \{\s+if \(layout_changed\) layout_changed\(\);\s+refresh_all\(\);\s+\}', block)
        self.assertTrue(guard, 'layout_changed/refresh_all must only run for a layout that differs')
        for reason in ('changed', 'moved', '!was_configured', 'rotation_changed', 'model.pages != previous_pages',
                       'model.title != previous_title'):
            self.assertIn(reason, guard[1])
        self.assertEqual(block.count('layout_changed()'), 1)

    def test_the_guition_hides_the_stray_gt911_contact_from_lvgl_and_the_edge_swipe(self):
        for name in ('guition-4848s040.yaml', 'packages/guition.yaml'):
            text = self.profiles[name]
            self.assertIn('cyd::ghost_touch.filter(', text, name)
            self.assertIn('lv_indev_set_read_cb(indev, filtered);', text, name)
            self.assertRegex(text, r'if \(point\.x == 0 && point\.y == 0\) continue;\s+cyd::touch_guard\.update', name)
        for name in ('home-like-2432s028.yaml', 'packages/cyd.yaml'):
            self.assertNotIn('ghost_touch', self.profiles[name], 'the CYD has its own XPT2046 filter')

    def test_a_slider_that_jumps_on_release_is_logged(self):
        self.assertIn('cyd::release_jump(row.held, value,', LIGHT_CONTROLS)
        self.assertIn('cyd::release_jump(slider_held,raw,', RUNTIME)


if __name__ == '__main__':
    unittest.main()
