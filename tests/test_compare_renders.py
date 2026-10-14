"""The render comparison has to find a difference, or it proves nothing.

tools/compare_renders.py is what a round uses to show that the boards which already ship look the same
afterwards. A comparison that always answers "identical" would pass every round and catch nothing, so these
tests hand it pictures that differ in known ways and check it says so, and where.
"""
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'tools'))

try:
    from PIL import Image
    import compare_renders
except ImportError:  # pragma: no cover - Pillow is in the render environment
    Image = None


@unittest.skipIf(Image is None, 'needs Pillow')
class RunsTests(unittest.TestCase):
    def test_rows_read_as_bands(self):
        # A mistake that moves a block of the screen shows up as a band of rows, not as a list of numbers.
        self.assertEqual(compare_renders.runs([]), '')
        self.assertEqual(compare_renders.runs([7]), '7')
        self.assertEqual(compare_renders.runs([3, 4, 5, 9]), '3-5, 9')
        self.assertEqual(compare_renders.runs([200, 201, 202, 203]), '200-203')


@unittest.skipIf(Image is None, 'needs Pillow')
class DifferenceTests(unittest.TestCase):
    def picture(self, colour=(10, 20, 30), size=(40, 30)):
        return Image.new('RGB', size, colour)

    def test_the_same_picture_twice_is_identical(self):
        d = compare_renders.Difference('page-1.png', self.picture(), self.picture())
        self.assertTrue(d.identical)
        self.assertEqual(d.count, 0)

    def test_one_pixel_of_one_channel_is_found(self):
        before, after = self.picture(), self.picture()
        after.load()[12, 5] = (10, 20, 31)
        d = compare_renders.Difference('page-1.png', before, after)
        self.assertFalse(d.identical)
        self.assertEqual(d.count, 1)
        self.assertEqual(d.box, (12, 5, 13, 6))
        self.assertEqual(d.rows, '5')
        self.assertEqual(d.columns, '12')
        self.assertEqual(d.worst, 1)

    def test_a_band_of_rows_is_reported_as_a_band(self):
        # The shape of the mistake this round actually made: the tile area a top bar too tall, so the cards
        # ran into the page bar. It has to read as the rows it moved.
        before, after = self.picture(), self.picture()
        pixels = after.load()
        for y in range(20, 26):
            for x in range(40):
                pixels[x, y] = (200, 0, 0)
        d = compare_renders.Difference('page-2.png', before, after)
        self.assertEqual(d.count, 6 * 40)
        self.assertEqual(d.rows, '20-25')
        self.assertEqual(d.columns, '0-39')
        self.assertIn('rows 20-25', d.line())

    def test_a_different_size_is_a_difference(self):
        d = compare_renders.Difference('page-1.png', self.picture(), self.picture(size=(40, 31)))
        self.assertFalse(d.identical)
        self.assertTrue(d.size_changed)
        self.assertIn('SIZE', d.line())


@unittest.skipIf(Image is None, 'needs Pillow')
class FolderTests(unittest.TestCase):
    def test_a_folder_of_renders_passes_only_when_every_one_matches(self):
        with tempfile.TemporaryDirectory() as tmp:
            before, after = Path(tmp) / 'before', Path(tmp) / 'after'
            before.mkdir(); after.mkdir()
            for name in ('page-1.png', 'page-2.png'):
                Image.new('RGB', (20, 20), (1, 2, 3)).save(before / name)
                Image.new('RGB', (20, 20), (1, 2, 3)).save(after / name)
            self.assertTrue(compare_renders.compare(before, after))
            changed = Image.new('RGB', (20, 20), (1, 2, 3))
            changed.load()[4, 4] = (9, 9, 9)
            changed.save(after / 'page-2.png')
            self.assertFalse(compare_renders.compare(before, after))

    def test_a_render_that_stopped_being_drawn_is_a_difference(self):
        with tempfile.TemporaryDirectory() as tmp:
            before, after = Path(tmp) / 'before', Path(tmp) / 'after'
            before.mkdir(); after.mkdir()
            Image.new('RGB', (20, 20), (1, 2, 3)).save(before / 'card-light.png')
            self.assertFalse(compare_renders.compare(before, after))


if __name__ == '__main__':
    unittest.main()
