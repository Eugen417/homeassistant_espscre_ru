"""Build every board as a host program, run its self test, and render what it draws (tools/render/host.py).

For each variant (a board of the catalog, lying down and, where its glass is not square, standing up) this compiles the
real firmware for the host, starts it, and drives it over its API with no Home Assistant involved:

- the demo layout of diagnostics/send_layout.py (every kind of card, fixed states at a fixed moment);
- the firmware's own self test (ui_self_test): every page and overlay rendered, each page's cards and bar checked,
  and on every board the geometry check that nothing falls outside its area. A FAIL fails the run;
- PNGs of every page, of three alerts (plain, long, with a button), of an alert with a camera picture on the boards
  that draw pictures (made by the add-on's own camera_feed for the frame this screen's card makes for it), and of
  page 1 in Dark mode.

    python3 tools/render/run.py                       every variant, into .esphome/render/out/
    python3 tools/render/run.py guition cyd-portrait  those variants
    python3 tools/render/run.py --tree ../main --out .esphome/render/base
                                                      another tree (a checkout of an older commit), to compare with
                                                      tools/compare_renders.py

Run it with the Python of ESPHome (it needs aioesphomeapi and Pillow); ESPHOME names the esphome command, SDL2 must be
installed. It exits 1 when a variant does not build, does not start, or fails its self test.
"""
import argparse
import asyncio
import http.server
import io
import json
import os
import re
import shlex
import socket
import subprocess
import sys
import threading
import time
from datetime import datetime
from pathlib import Path
from zoneinfo import ZoneInfo

from aioesphomeapi import APIClient, LogLevel
from PIL import Image, ImageDraw

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(REPO / 'diagnostics'))
sys.path.insert(0, str(REPO / 'screen_manager' / 'app'))
import host  # noqa: E402
import send_layout  # noqa: E402

# Every render shows the same moment: Tuesday 15 September 2026, 10:08 in Amsterdam.
MOMENT = datetime(2026, 9, 15, 10, 8, tzinfo=ZoneInfo('Europe/Amsterdam'))
LONG_TITLE = 'The washing machine in the basement has finished its extra long cotton cycle'
LONG_SUBTITLE = ('The drum has been standing full of wet laundry for almost two hours now, so it will start to smell '
                 'soon. Hang it up in the attic or move it to the dryer, then tap the button to clear this reminder.')
ALERTS = (
    ('alert-plain', dict(title='Someone is at the door', subtitle='Front door camera\nTap Coming to let them know', icon='doorbell')),
    ('alert-long', dict(title=LONG_TITLE, subtitle=LONG_SUBTITLE, icon='washing-machine')),
    ('alert-button', dict(title='Doorbell', subtitle='', icon='doorbell', button_text='Coming')),
)
PROBE = re.compile(r'probe page=(-?\d+) applied=(-?\d+) shown=(\d+) tiles=(\d+) alert=(\d) pages=(\d+)')
HEADER = re.compile(r'state page=(-?\d+) name=\[(.*?)\] shown=\[(.*?)\] name_box=(-?\d+),(-?\d+),(-?\d+),(-?\d+)')
ALERT = re.compile(r'alert on=(\d) ' + ' '.join(f'{part}=(-?\\d+),(-?\\d+),(-?\\d+),(-?\\d+)'
                                              for part in ('card', 'frame', 'icon', 'title', 'subtitle', 'button')))
# A page's own title (app 0.2.123): a long one among them, the kind that stood in dots after a page change (GitHub #27).
PAGE_TITLES = ['Demo cards', 'Living room downstairs', 'Kitchen']


def overlap(a, b):
    return a[0] <= b[2] and b[0] <= a[2] and a[1] <= b[3] and b[1] <= a[3]


def alert_faults(state):
    """What is wrong with an alert card as LVGL placed it: a part outside the card, or two parts over each other."""
    parts = {name: box for name, box in state.items() if name != 'card' and box[2] >= box[0]}
    card, faults = state['card'], []
    for name, box in parts.items():
        if not (card[0] <= box[0] and box[2] <= card[2] and card[1] <= box[1] and box[3] <= card[3]):
            faults.append(f'{name} {box} outside the card {card}')
    names = list(parts)
    for i, one in enumerate(names):
        for other in names[i + 1:]:
            if overlap(parts[one], parts[other]):
                faults.append(f'{one} {parts[one]} over {other} {parts[other]}')
    return faults


def porch(width, height):
    """A drawn porch at night for the camera: sky, wall, a lit door, a lamp and a doormat inside a red frame, so a crop,
    a squeeze or an offset of the picture shows in a render. Deterministic, no photo."""
    image = Image.new('RGB', (width, height))
    draw = ImageDraw.Draw(image)
    for y in range(height):
        t = y / height
        draw.line([(0, y), (width, y)], fill=(int(24 + 40 * t), int(34 + 50 * t), int(60 + 40 * t)))
    draw.rectangle([0, int(height * 0.18), width, height], fill=(150, 120, 100))
    for row in range(int(height * 0.18), height, 24):
        draw.line([(0, row), (width, row)], fill=(128, 100, 84), width=2)
    door = [int(width * 0.40), int(height * 0.30), int(width * 0.60), int(height * 0.92)]
    draw.rectangle(door, fill=(40, 70, 60), outline=(230, 220, 190), width=8)
    draw.ellipse([int(width * 0.66), int(height * 0.34), int(width * 0.70), int(height * 0.42)], fill=(255, 220, 120))
    draw.rectangle([int(width * 0.36), int(height * 0.92), int(width * 0.64), height], fill=(90, 60, 40))
    draw.rectangle([0, 0, width - 1, height - 1], outline=(255, 64, 64), width=6)
    out = io.BytesIO()
    image.save(out, 'JPEG', quality=92)
    return out.getvalue()


class Pictures:
    """A little web server for the camera pictures the renders show, the way ESP Screens serves them (camera port)."""

    def __init__(self):
        self.files = {}
        files = self.files

        class Handler(http.server.BaseHTTPRequestHandler):
            def do_GET(self):
                body = files.get(self.path)
                self.send_response(200 if body else 404)
                self.send_header('Content-Type', 'image/bmp')
                self.send_header('Content-Length', str(len(body or b'')))
                self.end_headers()
                self.wfile.write(body or b'')

            def log_message(self, *args):
                pass

        self.server = http.server.ThreadingHTTPServer(('127.0.0.1', 0), Handler)
        threading.Thread(target=self.server.serve_forever, daemon=True).start()

    def url(self, name, body):
        self.files[f'/{name}'] = body
        return f'http://127.0.0.1:{self.server.server_address[1]}/{name}'


def alert_picture(item, camera):
    """(BMP bytes, box) of the camera picture an alert on this variant gets: sized by the add-on's own rule for the frame
    this screen's card makes for it (camera_feed.alert_box), encoded as the add-on encodes it."""
    import camera_feed
    import core
    raw = porch(*camera)
    screen = {'board': item.board, 'orientation': 'portrait' if item.rotation else 'landscape',
              'firmware_known': core.FIRMWARE_VERSION}
    box = camera_feed.alert_box(screen, camera_feed.picture_size(raw))
    return camera_feed.encode(raw, box, exact=box != camera_feed.box(screen, 'thumb')), box


def free(port):
    with socket.socket() as probe:
        return probe.connect_ex(('127.0.0.1', port)) != 0


class Run:
    """One variant's program, driven over its API."""

    def __init__(self, build, out, pictures, camera):
        self.build, self.item, self.out, self.pictures, self.camera = build, build.variant, out, pictures, camera
        self.lines, self.warnings, self.failures = [], [], []

    async def call(self, name, **args):
        await self.client.execute_service(self.services[name], args)

    async def until(self, test, timeout, what, start=None):
        start, end = len(self.lines) if start is None else start, time.monotonic() + timeout
        while time.monotonic() < end:
            for line in self.lines[start:]:
                if test(line):
                    return line
            await asyncio.sleep(0.05)
        raise RuntimeError(f'{what}: nothing after {timeout} s')

    async def probe(self):
        start = len(self.lines)
        await self.call('render_probe')
        line = await self.until(lambda l: PROBE.search(l), 10, 'render_probe', start)
        return tuple(int(v) for v in PROBE.search(line).groups())

    async def page_done(self, page, timeout=20):
        """The page is placed and its cards drawn."""
        end = time.monotonic() + timeout
        while time.monotonic() < end:
            p = await self.probe()
            if p[0] == page and p[1] == page and p[2] > 0:
                return p
            await asyncio.sleep(0.2)
        raise RuntimeError(f'page {page + 1} never finished: {p}')

    async def snapshot(self, path):
        path.unlink(missing_ok=True)
        await self.call('render_png', path=str(path))
        end = time.monotonic() + 15
        while time.monotonic() < end:
            if path.exists():  # render_png writes <path>.part and renames it when it is complete
                with Image.open(path) as image:
                    return image.convert('RGB')
            await asyncio.sleep(0.05)
        raise RuntimeError(f'no snapshot at {path}')

    async def render(self, name, keep=True, timeout=25):
        """Snapshots until two in a row match (at least 0.4 s apart); the last is kept as <name>.png."""
        start, previous = time.monotonic(), None
        while True:
            image = await self.snapshot(self.out / f'{name}.ppm')
            if previous is not None and image.tobytes() == previous.tobytes():
                break
            if time.monotonic() - start > timeout:
                self.warnings.append(f'{name}: never two equal snapshots in {timeout} s, kept the last')
                break
            previous = image
            await asyncio.sleep(0.4)
        (self.out / f'{name}.ppm').unlink(missing_ok=True)
        if keep:
            image.save(self.out / f'{name}.png')
        return image

    async def send(self, message):
        from core import packets
        for packet in packets(message):
            self.client.text_command(self.inbox.key, packet)
            await asyncio.sleep(0.02)

    async def alert(self, name, reference, **given):
        """One alert over page 1, rendered and dismissed; page 1 must then look as it did."""
        args = dict(title='', subtitle='', icon='', color='', button_text='', timeout=0, flash=False)
        args.update(given)
        start = len(self.lines)
        await self.call('show_alert', **args)
        await self.until(lambda l: '[alert' in l and 'show "' in l, 10, f'{name}: the alert never showed', start)
        state = await self.state()
        self.failures += [f'{name}, as it opens: {fault}' for fault in alert_faults(state['boxes'])]
        if name == 'alert-camera':
            if state['boxes']['frame'][2] < state['boxes']['frame'][0]:
                self.failures.append(f'{name}: no frame for the picture while it loads')
            await self.render(f'{name}-waiting')
            await self.camera_link(start)
            state = await self.state()
            self.failures += [f'{name}, with its picture: {fault}' for fault in alert_faults(state['boxes'])]
        await self.render(name)
        start = len(self.lines)
        await self.call('dismiss_alert')
        await self.until(lambda l: 'dismissed: remote' in l, 10, f'{name}: the alert was never dismissed', start)
        await self.page_done(0)
        after = await self.render(f'_after-{name}', keep=False)
        if after.tobytes() != reference.tobytes():
            self.failures.append(f'{name}: page 1 looks different after the alert was dismissed')

    async def camera_link(self, start):
        body, box = alert_picture(self.item, self.camera)
        url = self.pictures.url(f'{self.item.key}-alert.bmp', body)
        await self.send({'v': 1, 'op': 'camera', 't': 'alert', 'e': 'camera.front_door', 'u': url})
        line = await self.until(lambda l: 'alert picture shown' in l or 'alert picture failed' in l, 20,
                                'alert-camera: the picture never loaded', start)
        if 'failed' in line:
            raise RuntimeError(f'alert-camera: {line}')
        self.warnings.append(f'alert-camera: a {self.camera[0]} x {self.camera[1]} camera, sent at {box[0]} x {box[1]}')

    async def state(self):
        """The page title as its label shows it, and every part of the alert card, read back now."""
        start = len(self.lines)
        await self.call('render_state')
        line = await self.until(lambda l: ALERT.search(l), 10, 'render_state', start)
        header = next(HEADER.search(l) for l in self.lines[start:] if HEADER.search(l))
        values = [int(v) for v in ALERT.search(line).groups()]
        boxes = {name: tuple(values[1 + 4 * i:5 + 4 * i]) for i, name in enumerate(('card', 'frame', 'icon', 'title', 'subtitle', 'button'))}
        return {'page': int(header[1]), 'name': header[2], 'shown': header[3], 'alert': values[0], 'boxes': boxes}

    async def swipe(self, forward):
        """A finger across the tiles, the way a page is changed on the glass; returns the title label as it was read back
        every 50 ms from the release until it had been the same for a second."""
        width, height = self.canvas
        y = int(height * 0.6)
        # From the edge of the glass, as a page is changed on the glass: a wipe over the tiles is never a page flip on the
        # capacitive boards (features/capacitive-touch.yaml, the edge swipe). The touch panel is read every few tens of
        # milliseconds, so the finger rests on the edge long enough to be seen there, then moves the way a finger does,
        # a little further at every read (LVGL's gesture, the CYD's page swipe, starts counting again when it stops).
        start, stop = (0.99, 0.3) if forward else (0.01, 0.7)
        points = [int(width * (start + (stop - start) * i / 14)) for i in range(15)]
        await self.call('render_finger', x=points[0], y=y, down=True)
        await asyncio.sleep(0.15)
        for x in points[1:]:
            await self.call('render_finger', x=x, y=y, down=True)
            await asyncio.sleep(0.02)
        await self.call('render_finger', x=points[-1], y=y, down=False)
        seen, steady, end = [], 0, time.monotonic() + 6
        while time.monotonic() < end:
            state = await self.state()
            seen.append(state)
            steady = steady + 1 if len(seen) > 1 and (state['page'], state['shown']) == (seen[-2]['page'], seen[-2]['shown']) else 0
            if steady >= 20:
                break
            await asyncio.sleep(0.05)
        return seen

    async def moments(self, pages):
        """A page change by a finger, forward and back to page 1, with the title read back in the moment after it."""
        if pages < 2:
            return 0
        # "Swipe between pages" is off until someone turns it on; its switch, as Home Assistant turns it on.
        self.client.switch_command(self.swipe_switch.key, True)
        await asyncio.sleep(0.3)
        await self.call('render_page', page=0)
        await self.page_done(0)
        count = 0
        for forward, target in ((True, 1), (False, 0)):
            seen = await self.swipe(forward)
            count += len(seen)
            if seen[-1]['page'] != target:
                self.failures.append(f'swipe {"forward" if forward else "back"}: page {seen[-1]["page"] + 1}, not {target + 1}')
                continue
            final = seen[-1]['shown']
            if seen[-1]['name'] != PAGE_TITLES[target]:
                self.failures.append(f'page {target + 1} is named {seen[-1]["name"]!r}, not {PAGE_TITLES[target]!r}')
            for state in seen:
                if state['page'] == target and '...' in state['shown'] and '...' not in final:
                    self.failures.append(f'page {target + 1}: its title stood as {state["shown"]!r} for a moment, then {final!r}')
                    break
            self.warnings.append(f'swipe to page {target + 1}: title {final!r}, {len(seen)} read-backs')
        return count

    async def self_test(self):
        start = len(self.lines)
        await self.call('ui_self_test')
        await self.until(lambda l: 'UI_TEST COMPLETE' in l, 120, 'the self test never completed', start)
        lines = self.lines[start:]
        checks = [l for l in lines if 'page_check=' in l]
        failed = [l for l in lines if 'page_check=FAIL' in l or 'GEOMETRY FAIL' in l or 'Light sliders: FAIL' in l]
        if not checks:
            self.failures.append('self test: no page_check line')
        self.failures += [f'self test: {l.strip()}' for l in failed]
        return len(checks)

    async def drive(self):
        self.client = APIClient('127.0.0.1', self.item.port, None)
        for _ in range(240):
            if self.process.poll() is not None:
                raise RuntimeError(f'the program exited with {self.process.returncode}')
            try:
                await self.client.connect(login=True)
                break
            except Exception:
                await asyncio.sleep(0.5)
        else:
            raise RuntimeError('the API never came up')
        entities, services = await self.client.list_entities_services()
        self.services = {s.name: s for s in services}
        self.inbox = next(e for e in entities if type(e).__name__ == 'TextInfo' and e.name == 'Tile settings')
        dark = next(e for e in entities if getattr(e, 'name', '') == 'Dark mode')
        self.swipe_switch = next(e for e in entities if getattr(e, 'name', '') == 'Swipe between pages')
        self.client.subscribe_logs(lambda m: self.lines.append(re.sub(r'\x1b\[[0-9;]*m', '', m.message.decode(errors='replace')
                                                                      if isinstance(m.message, bytes) else m.message)),
                                   log_level=LogLevel.LOG_LEVEL_DEBUG, dump_config=False)
        if 'render_skip_calibration' in self.services:
            await self.call('render_skip_calibration')
        await self.call('render_time', epoch=int(MOMENT.timestamp()))
        messages = send_layout.messages(self.inbox.object_id, now=MOMENT)
        messages[0]['page_titles'] = PAGE_TITLES
        for message in messages:
            await self.send(message)
        tiles = len(messages[0]['entities'])
        end = time.monotonic() + 30
        while True:
            p = await self.probe()
            if p[3] == tiles and p[1] == 0:
                break
            if time.monotonic() > end:
                raise RuntimeError(f'the layout never arrived: {p}, {tiles} tiles expected')
            await asyncio.sleep(0.2)
        pages = p[5]
        side = json.loads((REPO / 'screen_manager/app/boards.json').read_text())[self.item.board]['orientations']
        side = side['portrait' if self.item.rotation else 'landscape']
        self.canvas = (side['width'], side['height'])
        checks = await self.self_test()
        await self.moments(pages)
        for page in range(pages):
            await self.call('render_page', page=page)
            await self.page_done(page)
            await self.render(f'page-{page + 1}')
        await self.call('render_page', page=0)
        await self.page_done(0)
        page1 = await self.render('_page-1', keep=False)
        for name, given in ALERTS:
            await self.alert(name, page1, **given)
        if 'camera' in json.loads((REPO / 'screen_manager/app/boards.json').read_text())[self.item.board]:
            await self.send({'v': 1, 'op': 'camera', 't': 'alert', 'e': 'camera.front_door', 'u': ''})
            await self.alert('alert-camera', page1, title='Someone is at the door', subtitle='Front door', icon='doorbell',
                             color='orange', button_text='Coming')
        self.client.switch_command(dark.key, True)
        end = time.monotonic() + 10
        while (await self.snapshot(self.out / '_dark.ppm')).tobytes() == page1.tobytes():
            if time.monotonic() > end:
                raise RuntimeError('Dark mode never changed the screen')
            await asyncio.sleep(0.2)
        (self.out / '_dark.ppm').unlink(missing_ok=True)
        await self.render('page-1-dark')
        self.client.switch_command(dark.key, False)
        return pages, checks

    async def run(self):
        self.out.mkdir(parents=True, exist_ok=True)
        for old in self.out.glob('*.png'):
            old.unlink()
        # The host keeps a screen's settings between runs; every run starts from the firmware's own defaults.
        (Path.home() / '.esphome' / 'prefs' / f'{self.item.name}.prefs').unlink(missing_ok=True)
        log = open(self.out / 'program.log', 'w')
        self.process = subprocess.Popen([str(self.build.program)], stdout=log, stderr=subprocess.STDOUT, cwd=self.build.work)
        try:
            pages, checks = await self.drive()
            return f'{pages} pages, {checks} page checks'
        finally:
            try:
                await self.client.disconnect()
            except Exception:
                pass
            self.process.terminate()
            try:
                self.process.wait(5)
            except subprocess.TimeoutExpired:
                self.process.kill()
            (self.out / 'log.txt').write_text('\n'.join(self.lines) + '\n')


def sheet(out, keys):
    """One picture of every variant's page 1, alerts and Dark mode side by side, for a look at a glance."""
    names = ['page-1', 'alert-plain', 'alert-long', 'alert-button', 'alert-camera', 'page-1-dark']
    rows = [(key, [out / key / f'{name}.png' for name in names]) for key in keys]
    cell, gap = 320, 12
    width = len(names) * (cell + gap) + gap
    heights = []
    for _, paths in rows:
        sizes = [Image.open(p).size for p in paths if p.exists()]
        heights.append(max((round(h * cell / w) for w, h in sizes), default=0))
    image = Image.new('RGB', (width, sum(h + 30 + gap for h in heights) + gap), (236, 236, 236))
    draw, y = ImageDraw.Draw(image), gap
    for (key, paths), height in zip(rows, heights):
        draw.text((gap, y), key, fill=(20, 20, 20))
        for i, path in enumerate(paths):
            if path.exists():
                with Image.open(path) as shot:
                    image.paste(shot.resize((cell, round(shot.height * cell / shot.width)), Image.LANCZOS), (gap + i * (cell + gap), y + 20))
        y += height + 30 + gap
    image.save(out / 'sheet.png')


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    parser.add_argument('variants', nargs='*', help='variants to run (default: all of them)')
    parser.add_argument('--tree', type=Path, default=REPO, help='the source tree to build (default: this checkout)')
    parser.add_argument('--out', type=Path, default=REPO / '.esphome' / 'render' / 'out')
    parser.add_argument('--work', type=Path, help='where the host builds go (default: .esphome/render/build)')
    parser.add_argument('--camera', default='960x540', help='the camera picture of the camera alert, WxH')
    args = parser.parse_args()
    # The programs write their pictures from their own folder, so every path they get is absolute.
    args.out = args.out.resolve()
    items = [host.variant(key) for key in args.variants] or host.variants()
    busy = [str(item.port) for item in items if not free(item.port)]
    if busy:
        raise SystemExit(f'ports {", ".join(busy)} are in use; stop what listens there first')
    esphome = shlex.split(os.environ.get('ESPHOME', 'esphome'))
    camera = tuple(int(n) for n in args.camera.split('x'))
    pictures = Pictures()
    results, failed = {}, []
    tree = args.tree.resolve()
    for item in items:
        build = host.Build(item, tree=tree, work=args.work, esphome=esphome)
        started = time.monotonic()
        ok, output = build.compile()
        if not ok:
            failed.append(item.key)
            results[item.key] = {'status': 'build failed', 'log': output[-3000:]}
            print(f'{item.key}: BUILD FAILED\n{output[-2000:]}', flush=True)
            continue
        built = time.monotonic()
        run = Run(build, args.out / item.key, pictures, camera)
        try:
            summary = asyncio.run(run.run())
        except Exception as error:  # a program that crashed or stopped answering
            run.failures.append(f'{type(error).__name__}: {error}')
            summary = 'stopped'
        status = 'FAIL' if run.failures else 'PASS'
        if run.failures:
            failed.append(item.key)
        results[item.key] = {'status': status, 'summary': summary, 'failures': run.failures, 'notes': run.warnings,
                             'build_s': round(built - started, 1), 'render_s': round(time.monotonic() - built, 1)}
        print(f'{item.key}: {status} ({summary}; build {built - started:.0f} s, render {time.monotonic() - built:.0f} s)', flush=True)
        for line in run.failures:
            print(f'  {line}', flush=True)
    rendered = [item.key for item in items if (args.out / item.key / 'page-1.png').exists()]
    if rendered:
        sheet(args.out, rendered)
    args.out.mkdir(parents=True, exist_ok=True)
    (args.out / 'summary.json').write_text(json.dumps({'tree': str(tree), 'results': results}, indent=1) + '\n')
    last = f'{len(items) - len(failed)} of {len(items)} variants passed'
    (args.out / 'summary.txt').write_text(''.join(f"{key}: {result['status']} {result.get('summary', '')}\n"
                                                  + ''.join(f'  {line}\n' for line in result.get('failures', []))
                                                  for key, result in results.items()) + last + '\n')
    print(f'{last}; renders in {args.out}', flush=True)
    return 1 if failed else 0


if __name__ == '__main__':
    raise SystemExit(main())
