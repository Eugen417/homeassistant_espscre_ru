"""Release lint (app 0.2.78): the version numbers of a release agree, and the packages fetch only fonts that exist.

Every push to main is a release (docs/RELEASING.md), so these run with the rest of the suite in tools/check.sh and CI.
Standard library only.
"""
from pathlib import Path
import re
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'screen_manager/app'))
from core import FIRMWARE_VERSION  # noqa: E402

CHANGELOG = ROOT / 'screen_manager/CHANGELOG.md'
# An app release: "## 0.2.76 (firmware 0.2.63)"; the oldest ones name no firmware. The two "## Firmware 0.2.1x"
# sections are firmware-only releases between app 0.2.11 and 0.2.12 and carry no app version, so they don't count.
APP_HEADING = re.compile(r'^## (\d+)\.(\d+)\.(\d+)\b(.*)$', re.M)
FIRMWARE_IN_HEADING = re.compile(r'\(firmware (\d+\.\d+\.\d+)\)')
# The generator (tools/generate_packages.py) turns fonts/<file> into a raw GitHub URL on main.
FONT_URL = re.compile(r'https://raw\.githubusercontent\.com/MaxGramser/homeassistant_espscreen/[^/\s"\']+/(fonts/[^"\'\s]+)')


def app_headings():
    """[(version tuple, text after the version)] of the CHANGELOG, top to bottom."""
    return [((int(a), int(b), int(c)), rest) for a, b, c, rest in APP_HEADING.findall(CHANGELOG.read_text())]


def dotted(version):
    return '.'.join(map(str, version))


class ReleaseVersionTests(unittest.TestCase):
    def test_config_version_is_the_newest_changelog_entry(self):
        match = re.search(r'^version:\s*["\']?([^"\'\s]+)["\']?\s*$', (ROOT / 'screen_manager/config.yaml').read_text(), re.M)
        self.assertIsNotNone(match, 'screen_manager/config.yaml has no version line')
        headings = app_headings()
        self.assertTrue(headings, 'screen_manager/CHANGELOG.md has no "## x.y.z" heading')
        self.assertEqual(match.group(1), dotted(headings[0][0]),
                         'Home Assistant offers the update by config.yaml: bump it together with a new CHANGELOG entry')

    def test_newest_entry_names_the_shipped_firmware(self):
        version, rest = app_headings()[0]
        firmware = FIRMWARE_IN_HEADING.search(rest)
        if firmware:
            self.assertEqual(firmware.group(1), FIRMWARE_VERSION,
                             f'CHANGELOG {dotted(version)} names firmware {firmware.group(1)}, core.FIRMWARE_VERSION is {FIRMWARE_VERSION}')

    def test_changelog_versions_are_unique_and_newest_first(self):
        versions = [version for version, _ in app_headings()]
        duplicates = sorted({dotted(v) for v in versions if versions.count(v) > 1})
        self.assertEqual(duplicates, [], 'a CHANGELOG version appears twice')
        for newer, older in zip(versions, versions[1:]):
            self.assertGreater(newer, older, f'CHANGELOG {dotted(newer)} stands above {dotted(older)}')


class PackageFontTests(unittest.TestCase):
    def test_every_font_a_package_fetches_is_in_the_tree(self):
        packages = sorted((ROOT / 'packages').glob('*.yaml'))
        self.assertTrue(packages, 'no packages/*.yaml')
        seen = 0
        for package in packages:
            for path in FONT_URL.findall(package.read_text()):
                seen += 1
                self.assertTrue((ROOT / path).is_file(), f'{package.name} fetches {path}, which is not in the tree')
        # A changed URL form must not turn this into a test of nothing.
        self.assertGreater(seen, 0, 'no fonts/... URL found in packages/*.yaml')


if __name__ == '__main__':
    unittest.main()
