"""Compare two folders of screen renders pixel for pixel and say exactly where they differ.

A round that changes how the screens are drawn has to prove that the boards which already ship look the same
afterwards. The host render harness draws the real firmware into a PNG per page and per card; this walks two
such folders and reports, per image, whether a single pixel moved.

    python3 tools/compare_renders.py before/cyd after/cyd
    python3 tools/compare_renders.py before after --boards cyd,guition --out diffs/

For every image that differs it prints the box the difference sits in, how many pixels moved, the largest
change in any channel, and which rows and columns are involved, so "the tiles ran 40 pixels into the page bar"
reads as a band of rows rather than as a number. With --out it also writes a picture per differing image:
the before, the after, and a map with every changed pixel marked, side by side.

Exits non-zero when anything differs, so it can stand in a gate.
"""
import argparse
import sys
from pathlib import Path

try:
    from PIL import Image, ImageChops, ImageDraw
except ImportError:  # pragma: no cover - the harness installs Pillow
    sys.exit('this needs Pillow (the render harness already has it)')


def flatten(diff):
    """One grey picture holding, per pixel, the largest change of any channel.

    Not `convert('L')`: that weighs the channels the way an eye does, so a single step of blue weighs 0.114
    and rounds away to nothing. A render that moved one channel by one would then read as identical, which is
    the one answer this tool may never give wrongly.
    """
    bands = diff.split()
    flat = bands[0]
    for band in bands[1:]:
        flat = ImageChops.lighter(flat, band)
    return flat


def runs(numbers):
    """Turn [3,4,5,9] into "3-5, 9": a difference reads as bands, not as a list of rows."""
    out, start, previous = [], None, None
    for n in numbers:
        if start is None:
            start = previous = n
        elif n == previous + 1:
            previous = n
        else:
            out.append((start, previous))
            start = previous = n
    if start is not None:
        out.append((start, previous))
    return ', '.join(str(a) if a == b else f'{a}-{b}' for a, b in out)


class Difference:
    """What changed between one before and one after image."""

    def __init__(self, name, before, after):
        self.name = name
        self.before, self.after = before, after
        self.size_changed = before.size != after.size
        self.box = self.count = self.worst = 0
        self.rows = self.columns = ''
        if self.size_changed:
            return
        diff = ImageChops.difference(before, after)
        self.box = diff.getbbox()
        if not self.box:
            return
        self.worst = max(band.getextrema()[1] for band in diff.split())
        flat = flatten(diff)
        pixels = flat.load()
        width, height = flat.size
        rows, columns = [], set()
        for y in range(height):
            hit = False
            for x in range(width):
                if pixels[x, y]:
                    self.count += 1
                    columns.add(x)
                    hit = True
            if hit:
                rows.append(y)
        self.rows = runs(rows)
        self.columns = runs(sorted(columns))

    @property
    def identical(self):
        return not self.size_changed and not self.box

    def line(self):
        if self.size_changed:
            return f'    {self.name}: SIZE {self.before.size} -> {self.after.size}'
        share = 100.0 * self.count / (self.before.size[0] * self.before.size[1])
        return (f'    {self.name}: {self.count} pixels ({share:.2f} %), box {self.box}, '
                f'largest change {self.worst}\n'
                f'        rows {self.rows}\n        columns {self.columns}')

    def picture(self, path):
        """Before, after, and a map of every pixel that moved, side by side."""
        if self.size_changed:
            return
        width, height = self.before.size
        gap = 8
        sheet = Image.new('RGB', (width * 3 + gap * 2, height), (255, 255, 255))
        sheet.paste(self.before, (0, 0))
        sheet.paste(self.after, (width + gap, 0))
        marks = self.after.copy().convert('L').convert('RGB')
        marks = Image.blend(marks, Image.new('RGB', self.after.size, (255, 255, 255)), 0.6)
        flat = flatten(ImageChops.difference(self.before, self.after))
        pixels, target = flat.load(), marks.load()
        for y in range(height):
            for x in range(width):
                if pixels[x, y]:
                    target[x, y] = (220, 30, 30)
        sheet.paste(marks, (width * 2 + gap * 2, 0))
        if self.box:
            draw = ImageDraw.Draw(sheet)
            x1, y1, x2, y2 = self.box
            draw.rectangle([width * 2 + gap * 2 + x1, y1, width * 2 + gap * 2 + x2 - 1, y2 - 1],
                           outline=(220, 30, 30))
        sheet.save(path)


def compare(before_dir, after_dir, out_dir=None):
    """Every PNG in `before_dir`, against the one of the same name in `after_dir`."""
    before_dir, after_dir = Path(before_dir), Path(after_dir)
    names = sorted(p.name for p in before_dir.glob('*.png'))
    if not names:
        print(f'    no renders in {before_dir}')
        return None
    identical, differing, missing = [], [], []
    for name in names:
        other = after_dir / name
        if not other.exists():
            missing.append(name)
            continue
        difference = Difference(name, Image.open(before_dir / name).convert('RGB'),
                                Image.open(other).convert('RGB'))
        if difference.identical:
            identical.append(name)
        else:
            differing.append(difference)
            if out_dir:
                Path(out_dir).mkdir(parents=True, exist_ok=True)
                difference.picture(Path(out_dir) / f'{before_dir.name}-{name}')
    extra = sorted(p.name for p in after_dir.glob('*.png') if p.name not in names)
    print(f'    {len(identical)} identical, {len(differing)} differing'
          + (f', {len(missing)} missing after' if missing else '')
          + (f', {len(extra)} only after' if extra else ''))
    for difference in differing:
        print(difference.line())
    for name in missing:
        print(f'    {name}: not rendered after')
    return not differing and not missing


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('before', help='folder of renders made before the change')
    parser.add_argument('after', help='folder of renders made after it')
    parser.add_argument('--boards', help='comma separated sub-folders to walk instead of the folders themselves')
    parser.add_argument('--out', help='write a before/after/difference picture per differing render here')
    args = parser.parse_args()

    good = True
    if args.boards:
        for board in args.boards.split(','):
            print(f'{board}:')
            result = compare(Path(args.before) / board, Path(args.after) / board, args.out)
            good = good and bool(result)
    else:
        print(f'{Path(args.before).name} -> {Path(args.after).name}:')
        good = bool(compare(args.before, args.after, args.out))
    print('every render is identical' if good else 'renders differ')
    return 0 if good else 1


if __name__ == '__main__':
    sys.exit(main())
