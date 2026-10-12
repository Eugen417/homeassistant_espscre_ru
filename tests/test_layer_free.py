"""Drawing that needs no buffers of its own: styles that make LVGL 9.5 render a widget into a separate layer.

Such a layer is a full-size buffer (width x height x 4 bytes) allocated on every redraw. The CYD has no PSRAM:
a slider fill rounded less than its track cost 15-20 KB per frame there, and when LVGL cannot get that memory
it retries inside the frame until the task watchdog resets the board.
"""
import re
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import profiles  # noqa: E402
PROFILES = ['checkout/cyd.yaml', 'checkout/guition.yaml', 'packages/cyd.yaml', 'packages/guition.yaml']
HEADERS = sorted((ROOT / 'components/smart_display').glob('*.h'))

# Style properties that give a widget a layer (lv_obj_style.c calculate_layer_type, lv_refr.c clip corners).
YAML_LAYER_STYLES = re.compile(r'^\s*(opa_layered|transform_rotation|transform_scale(?:_x|_y)?|transform_zoom|transform_skew(?:_x|_y)?'
                               r'|blend_mode|bitmap_mask_src|drop_shadow_\w+|blur_\w+):', re.M)
C_LAYER_STYLES = re.compile(r'lv_obj_set_style_(opa_layered|transform_rotation|transform_scale(?:_x|_y)?|transform_skew(?:_x|_y)?'
                            r'|blend_mode|bitmap_mask_src|drop_shadow_\w+|blur_\w+)\(|LV_STYLE_(OPA_LAYERED|TRANSFORM_ROTATION'
                            r'|TRANSFORM_SCALE\w*|TRANSFORM_SKEW\w*|BLEND_MODE|BITMAP_MASK_SRC|CLIP_CORNER)\b')


class LayerFreeTests(unittest.TestCase):
    def test_board_profiles_use_no_layer_styles(self):
        for name in PROFILES:
            text = profiles.text(name)
            self.assertEqual(YAML_LAYER_STYLES.findall(text), [], name)
            self.assertEqual(re.findall(r'^\s*clip_corner: true', text, re.M), [], name)

    def test_firmware_sets_no_layer_styles(self):
        for path in HEADERS:
            text = path.read_text()
            self.assertEqual(C_LAYER_STYLES.findall(text), [], path.name)
            self.assertNotRegex(text, r'lv_obj_set_style_clip_corner\([^;]*\btrue\b', path.name)

    def test_slider_fill_keeps_the_track_radius_in_code(self):
        """Every fill radius in the firmware is the same expression as a track radius set beside it."""
        statement = re.compile(r'(?:lv_obj_set_style_radius\((\w+(?:\.\w+)?),\s*|set_number\((\w+),\s*LV_STYLE_RADIUS,\s*)'
                               r'([^;]*?),\s*(LV_PART_MAIN|LV_PART_INDICATOR)\s*\)')
        checked = 0
        for path in HEADERS:
            text = path.read_text()
            found = [(m.group(1) or m.group(2), m.group(3).strip(), m.group(4)) for m in statement.finditer(text)]
            for widget, value, part in found:
                if part != 'LV_PART_INDICATOR':
                    continue
                checked += 1
                self.assertIn((widget, value, 'LV_PART_MAIN'), found, f'{path.name}: fill radius {value} of {widget} differs from its track')
        self.assertGreaterEqual(checked, 2)

    def test_yaml_lambdas_keep_the_track_radius(self):
        """A lambda in the YAML that rounds a fill gives it the same expression as the track it rounds beside it.

        The overlay slider's open script once set its track to 28 and its fill to 0: on a CYD that fill was drawn
        through a 96 x 160 ARGB layer of 61 KB on every redraw, and a drag while the heap was busy reset the board.
        """
        statement = re.compile(r'lv_obj_set_style_radius\(id\((\w+)\),\s*([^;]*?),\s*(LV_PART_MAIN|LV_PART_INDICATOR)\s*\)')
        checked = 0
        for name in PROFILES:
            found = [(m.group(1), m.group(2).strip(), m.group(3)) for m in statement.finditer(profiles.text(name))]
            for widget, value, part in found:
                if part != 'LV_PART_INDICATOR':
                    continue
                checked += 1
                self.assertIn((widget, value, 'LV_PART_MAIN'), found, f'{name}: fill radius {value} of {widget} differs from its track')
        self.assertGreaterEqual(checked, 0)

    def test_yaml_slider_fill_keeps_the_track_radius(self):
        for name in PROFILES:
            text = profiles.text(name)
            for match in re.finditer(r'^(\s+)- (?:slider|bar):\n((?:\1    .*\n|\s*\n)+)', text, re.M):
                block, indent = match.group(2), match.group(1) + '    '
                track = re.search(r'^' + indent + r'radius: (\S+)', block, re.M)
                fill = re.search(r'^' + indent + r'indicator:\n(?:' + indent + r'  .*\n)*?' + indent + r'  radius: (\S+)', block, re.M)
                if fill:
                    self.assertTrue(track and track.group(1) == fill.group(1), f'{name}: slider fill radius {fill.group(1)} differs from its track')


if __name__ == '__main__':
    unittest.main()
