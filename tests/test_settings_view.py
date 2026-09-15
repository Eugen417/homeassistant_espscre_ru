"""Settings (app 0.2.45): New screen, Firmware & USB, firmware updates, Alerts and Claude moved off the main
page into a view of their own, so the header keeps one button and My screens keeps the editor."""
from pathlib import Path
import re
import unittest

STATIC = Path(__file__).resolve().parents[1] / 'screen_manager/app/static'


def between(text, start, end):
    return text.split(start, 1)[1].split(end, 1)[0]


class SettingsView(unittest.TestCase):
    def setUp(self):
        self.html = (STATIC / 'index.html').read_text()
        self.script = (STATIC / 'app.js').read_text()
        self.css = (STATIC / 'style.css').read_text()

    def test_header_keeps_only_the_settings_button(self):
        header = between(self.html, '<header>', '</header>')
        self.assertEqual(re.findall(r'<button id="([\w-]+)"', header), ['open-settings'])

    def test_every_tool_lives_in_the_settings_view_once(self):
        settings = between(self.html, '<section id="settings-view"', '</main>')
        home = between(self.html, '<div id="home-view">', '<section id="settings-view"')
        for element in ('new-screen', 'open-firmware', 'updates', 'update-all', 'auto-update', 'open-alerts',
                        'claude-install', 'claude-status', 'claude-path', 'claude-download', 'close-settings'):
            self.assertEqual(self.html.count(f'id="{element}"'), 1, element)
            self.assertIn(f'id="{element}"', settings, element)
            self.assertNotIn(f'id="{element}"', home, element)
        # The first-run button stays where a new user starts.
        self.assertIn('id="start"', home)
        self.assertIn('href="api/claude-skill.zip"', settings)

    def test_the_view_switches_by_hash_and_renders_what_it_shows(self):
        for marker in ('function showView', 'location.hash === "#settings"', 'window.addEventListener("hashchange", showView)',
                       '$("#home-view").hidden = settings', 'function renderClaude', 'api("claude-skill", { method: "POST" })',
                       'if (full) renderClaude();', '$("#new-screen").onclick = $("#start").onclick = openInstaller'):
            self.assertIn(marker, self.script, marker)
        self.assertNotIn('Not flashed yet? Firmware & USB', self.script, 'texts point to Settings')
        for marker in ('.settings-groups', '.settings-group', '.claude-skill', '.screen:hover:not(.selected)'):
            self.assertIn(marker, self.css, marker)

    def test_unselected_screens_get_a_grey_card(self):
        rule = between(self.css, '\n.screen {', '}')
        self.assertIn('background: #e9ecf1', rule)
        self.assertIn('background: white', between(self.css, '\n.screen.selected {', '}'))


if __name__ == '__main__':
    unittest.main()
