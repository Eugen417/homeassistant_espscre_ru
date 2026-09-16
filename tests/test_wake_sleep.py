"""Wake and Sleep buttons for Home Assistant automations (app 0.2.53, firmware 0.2.45).

Both board profiles have to wire them the same way: Wake is a tap, Sleep is the standby time running out
right away, and a Sleep holds with Auto standby off until a tap, Wake or an alert ends it.
"""
import re
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'screen_manager/app'))
from core import FIRMWARE_VERSION, WAKE_SLEEP_MIN_FIRMWARE  # noqa: E402

PROFILES = ('home-like-2432s028.yaml', 'guition-4848s040.yaml', 'packages/cyd.yaml', 'packages/guition.yaml')


def section(text, top):
    """One top-level YAML section of a profile, up to the next one."""
    match = re.search(rf'^{top}:\n(.*?)(?=^[a-z_]+:|\Z)', text, re.M | re.S)
    return match[1] if match else ''


def item(text, key, value):
    """The list item in a section that carries `key: value`, up to the next item at the same depth."""
    for chunk in re.split(r'^  - ', text, flags=re.M)[1:]:
        if re.search(rf'^\s*{key}: "?{re.escape(value)}"?$', chunk, re.M):
            return chunk
    return ''


def script(text, name):
    return item(section(text, 'script'), 'id', name)


class Profiles(unittest.TestCase):
    def setUp(self):
        self.profiles = {name: (ROOT / name).read_text() for name in PROFILES}

    def test_the_firmware_that_carries_the_buttons_is_this_release_or_older(self):
        version = lambda text: tuple(int(part) for part in text.split('.'))
        self.assertLessEqual(version(WAKE_SLEEP_MIN_FIRMWARE), version(FIRMWARE_VERSION))

    def test_both_buttons_are_plain_controls_home_assistant_sees(self):
        for name, text in self.profiles.items():
            buttons = section(text, 'button')
            for label in ('Wake', 'Sleep'):
                button = item(buttons, 'name', label)
                self.assertTrue(button, f'{name}: no {label} button')
                self.assertIn('platform: template', button, name)
                self.assertNotIn('entity_category', button, f'{name}: {label} is a control, not a setting')
                self.assertNotIn('internal', button, f'{name}: {label} must reach Home Assistant')

    def test_wake_is_a_tap(self):
        for name, text in self.profiles.items():
            wake = item(section(text, 'button'), 'name', 'Wake')
            # A dimmed screen wakes the way the tap on its wake overlay does; one that is on only counts again.
            self.assertRegex(wake, r"return id\(display_dimmed\);'\s+then:\s+- script\.execute: wake_display\s+else:\s+"
                                   r"- lambda: 'id\(last_touch_ms\) = millis\(\);'", name)
            self.assertIn('id(sleep_requested) = false;', script(text, 'wake_display'), f'{name}: waking ends a Sleep')
            overlay = re.search(r'id: dim_wake_overlay\n.*?on_click:\s+- script\.execute: (\w+)', text, re.S)
            self.assertEqual(overlay[1], 'wake_display', f'{name}: a tap and Wake share one path')

    def test_sleep_dims_now_and_holds_with_auto_standby_off(self):
        for name, text in self.profiles.items():
            sleep = item(section(text, 'button'), 'name', 'Sleep')
            # Not during a touch calibration (said in the log); otherwise the flag, a closed alert, then the dim.
            self.assertRegex(sleep, r"return id\(calibration_active\);'\s+then:\s+- lambda: 'ESP_LOGI\(\"standby\", \"Sleep from Home Assistant ignored", name)
            steps = re.findall(r'script\.execute:(?:\s+id:)?\s+(\w+)|(id\(sleep_requested\) = true;)', sleep.split('else:', 1)[1])
            self.assertEqual([step[0] or 'flag' for step in steps], ['flag', 'alert_dismiss', 'dim_display'], name)
            self.assertIn('reason: "remote"', sleep, f'{name}: HA hears the alert was closed from Home Assistant')
            dim = script(text, 'dim_display')
            self.assertIn("lambda: 'return screen_settings::current.standby_enabled || id(sleep_requested);'", dim, name)
            apply = script(text, 'apply_screen_settings')
            self.assertIn('if (id(display_dimmed) && !settings.standby_enabled && !id(sleep_requested)) {', apply,
                          f'{name}: the minute tick must not undo a Sleep')
            self.assertEqual(len(re.findall(r'id\(sleep_requested\) = true;', text)), 1, f'{name}: only the button starts a Sleep')

    def test_the_flag_starts_false_and_is_never_saved(self):
        for name, text in self.profiles.items():
            flag = item(section(text, 'globals'), 'id', 'sleep_requested')
            self.assertIn('type: bool', flag, name)
            self.assertIn('restore_value: no', flag, name)
            self.assertIn("initial_value: 'false'", flag, name)

    def test_switching_auto_standby_off_still_wakes_the_screen(self):
        for name, text in self.profiles.items():
            switch = item(section(text, 'switch'), 'name', 'Auto standby')
            off = switch.split('turn_off_action:', 1)[1]
            # Only a real change wakes: a switch that was already off returns before the flag is touched.
            self.assertRegex(off, r'if \(!s\.standby_enabled\) return;\s+s\.standby_enabled = false;\s+(//[^\n]*\n\s+)?'
                                  r'id\(sleep_requested\) = false;', name)


if __name__ == '__main__':
    unittest.main()
