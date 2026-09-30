"""Render map cards (app 0.4.33, firmware 0.20.0) with the real firmware on the host (tools/render/host.py) and the
add-on's own renderer: every size, light and Dark mode, framed around everyone, around home and around one person.

    python3 tools/render/map_tiles.py guition waveshare43 [--tiles DIR]

The program asks for its page's pictures the way it asks ESP Screens (the esphome.screen_camera event with an atlas,
`idx` and `dark`); this script answers as the add-on does (map_card.render_tile, tile_art.encode). The streets are Home
Assistant's vector tiles, read from `--tiles` (files z_x_y.mvt, as the add-on fetched them); without them a card is
drawn from its zones and people alone, as it is when Home Assistant has no tiles to give. The people and places are made
up: a household in the middle of Amsterdam.
"""
import argparse
import asyncio
import json
import os
import shlex
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import run  # noqa: E402
import host  # noqa: E402
import send_layout  # noqa: E402
import camera_feed  # noqa: E402
import core  # noqa: E402
import map_card  # noqa: E402
import tile_art  # noqa: E402
import vector_tiles  # noqa: E402
from PIL import Image, ImageDraw, ImageFont  # noqa: E402


def place(entity, name, state, lat=None, lon=None, radius=None):
    attributes = {'friendly_name': name}
    if lat is not None:
        attributes.update(latitude=lat, longitude=lon)
    if radius is not None:
        attributes['radius'] = radius
    return entity, {'state': state, 'attributes': attributes}


STATES = dict([
    place('zone.home', 'Home', '2', 52.35870, 4.86820, 90),
    place('zone.office', 'Office', '1', 52.37600, 4.89780, 140),
    place('zone.school', 'School', '0', 52.35290, 4.89100, 110),
    place('person.alex', 'Alex Morgan', 'home', 52.35878, 4.86835),
    place('person.jo', 'Jo', 'home', 52.35862, 4.86800),
    place('person.sam', 'Sam', 'Office', 52.37590, 4.89800),
    place('person.robin', 'Robin', 'not_home', 52.36420, 4.88310),
])
EVERYONE = ['person.jo', 'person.sam', 'person.robin']
NEIGHBOURS = [
    ('light.demo', 'Table lamp', {'state': 'on', 'attributes': {'brightness': 180}}),
    ('sensor.demo_temperature', 'Living room', {'state': '21.5', 'attributes': {'unit_of_measurement': '°C'}}),
    ('switch.demo_desk', 'Desk', {'state': 'off', 'attributes': {}}),
    ('binary_sensor.demo_door', 'Front door', {'state': 'off', 'attributes': {'device_class': 'door'}}),
    ('sensor.demo_energy', 'Usage today', {'state': '7.42', 'attributes': {'unit_of_measurement': 'kWh'}}),
    ('light.demo_ceiling', 'Ceiling light', {'state': 'off', 'attributes': {}}),
    ('scene.demo_evening', 'Evening', {'state': '2026-09-15T07:00:00+00:00', 'attributes': {}}),
    ('light.demo_hall', 'Hall', {'state': 'on', 'attributes': {'brightness': 120}}),
]
# (name of the shot, the map tiles of one page: entity, name, size, options)
PAGES = [
    ('full', [('person.alex', 'Family', 'full', {'map': EVERYONE})]),
    ('square', [('person.alex', 'Family', 'square', {'map': EVERYONE})]),
    ('tall-wide', [('person.alex', 'Family', 'tall', {'map': EVERYONE}),
                   ('person.jo', 'Home', 'wide', {'map': ['person.sam', 'person.robin'], 'framing': 'home'})]),
    ('singles', [('person.alex', 'Alex', 'single', {'framing': 'person'}),
                 ('person.sam', 'Sam', 'single', {'framing': 'person', 'distance': 'street'})]),
]


class Study(run.Run):
    def __init__(self, build, out, pictures, streets):
        super().__init__(build, out, pictures, (1280, 720))
        self.streets = streets

    def answer(self, call):
        if call.service != 'esphome.screen_camera' or 'tiles' not in call.data:
            return
        task = asyncio.get_running_loop().create_task(self.live(dict(call.data)))
        task.add_done_callback(lambda t: t.exception() and 'superseded' not in str(t.exception()) and self.warnings.append(f'answer failed: {t.exception()!r}'))

    async def live(self, data):
        entities = data['tiles'].split(',')
        atlas = tile_art.parse(data.get('atlas', ''), self.canvas, len(entities))
        own = camera_feed.live_indexes(data, entities)
        if atlas is None or own is None:
            self.warnings.append(f'no atlas or indexes in {data}')
            return
        dark = camera_feed.live_dark(data)
        raws = []
        for n, entity in enumerate(entities):
            tile = self.compiled[own[n]]
            _, _, w, h, _, _ = atlas[2][n]
            view = map_card.view_for(tile, STATES, (w, h), self.board)
            streets = {key: self.streets[key] for key in view.tiles() if key in self.streets}
            raws.append(map_card.render_tile(tile, STATES, (w, h), self.board, dark, streets))
        grounds = [int(c, 16) for c in data['bg'].split(',')]
        body = tile_art.encode(raws, grounds, atlas, None, True)
        self.served += 1
        url = self.pictures.url(f'map-{self.served}.bmp', body)
        async with self.turn:
            await self.send({'v': 1, 'op': 'camera', 't': 'live', 'e': data['tiles'], 'u': url, 'view': int(data['view'])})

    async def picture(self, what, tries=4):
        for attempt in range(tries):
            start = len(self.lines)
            await self.call('render_live_reset')
            try:
                await self.until(lambda line: 'asked for the live tiles' in line, 10, f'{what}: the ask', start)
                await self.until(lambda line: 'live tiles loaded' in line, 15, f'{what}: the picture', start)
                await asyncio.sleep(0.8)
                return
            except RuntimeError:
                if attempt == tries - 1:
                    raise
                await asyncio.sleep(1)

    async def drive(self):
        self.served, self.turn = 0, asyncio.Lock()
        self.client = run.APIClient('127.0.0.1', self.item.port, None)
        for _ in range(240):
            if self.process.poll() is not None:
                raise RuntimeError(f'the program exited with {self.process.returncode}')
            try:
                await self.client.connect(login=True)
                break
            except Exception:
                await asyncio.sleep(0.5)
        entities, services = await self.client.list_entities_services()
        self.services = {s.name: s for s in services}
        self.inbox = next(e for e in entities if type(e).__name__ == 'TextInfo' and e.name == 'Tile settings')
        dark = next(e for e in entities if getattr(e, 'name', '') == 'Dark mode')
        self.subscribe_logs()
        self.client.subscribe_service_calls(self.answer)
        if 'render_skip_calibration' in self.services:
            await self.call('render_skip_calibration')
        await self.call('render_time', epoch=int(run.MOMENT.timestamp()))
        shape = json.loads((run.REPO / 'screen_manager/app/boards.json').read_text())[self.item.board]
        side = shape['orientations']['portrait' if self.item.rotation else 'landscape']
        grid = send_layout.Grid(side['columns'], side['rows'])
        self.canvas = (side['width'], side['height'])
        self.board = map_card.Board(shape)
        self.sender = send_layout.api_sender(self.client, services)
        region = dict(keepalive=120, clock_24h=True, numbers='point', group_min=1, percent_space=False)
        states = dict(STATES)
        tiles, neighbours = [], iter(NEIGHBOURS)
        pages = [(name, cards) for name, cards in PAGES if all(size != 'wide' and size != 'square' or grid.columns >= 2 for _, _, size, _ in cards)]
        for page, (_, cards) in enumerate(pages):
            cells = range(page * grid.slots, (page + 1) * grid.slots)
            taken = set()
            for entity, name, size, options in cards:
                # The first place on the page the tile fits whole (a double-width tile starts in a column with room).
                slot = next(c for c in cells if c not in taken and fits(grid, c, size, taken, cells))
                tiles.append(dict(entity=entity, name=name, slot=slot, options={'display': 'map', 'size': size, **options}))
                taken |= set(grid.footprint(slot, size))
            for cell in [c for c in range(page * grid.slots, (page + 1) * grid.slots) if c not in taken]:
                neighbour = next(neighbours, None)
                if neighbour:
                    tiles.append(dict(entity=neighbour[0], name=neighbour[1], slot=cell))
                    states[neighbour[0]] = neighbour[2]
        record = send_layout.migrate_legacy(dict(title='Map', tiles=tiles), grid)
        self.compiled = send_layout.compile_tiles(record['layout'], grid)
        values = [send_layout.state_message(i, tile, states) for i, tile in enumerate(self.compiled)]
        bars = [[{'k': 'clock'}] for _ in record['layout']['pages']]
        async with self.turn:
            await self.sender.synchronize(self.inbox.object_id, record, region, values, bars)
        shots = 0
        for look in ('light', 'dark'):
            if look == 'dark':
                self.client.switch_command(dark.key, True)
                await asyncio.sleep(1.0)
            for page, (name, _) in enumerate(pages):
                await self.call('render_page', page=page)
                await self.page_done(page)
                await self.picture(f'{name} {look}')
                await self.render(f'{name}-{look}')
                shots += 1
        self.client.switch_command(dark.key, False)
        return len(pages), shots


def fits(grid, slot, size, taken, cells):
    try:
        footprint = set(grid.footprint(slot, size))
    except ValueError:
        return False
    first = slot - slot % grid.slots
    return footprint <= set(cells) and not footprint & taken and \
        (size != 'wide' and size != 'square' or (slot - first) % grid.columns + 2 <= grid.columns)


def streets_from(folder):
    """{(z, x, y): decoded} of every tile in `folder` (z_x_y.mvt)."""
    found = {}
    for path in sorted(Path(folder).glob('*_*_*.mvt')) if folder else ():
        try:
            z, x, y = (int(n) for n in path.stem.split('_'))
            found[(z, x, y)] = vector_tiles.decode(path.read_bytes(), map_card.LAYERS)
        except ValueError:
            continue
    return found


def sheet(out, key, names):
    font = ImageFont.truetype(str(run.REPO / 'fonts/Roboto-500.ttf'), 22)
    shots = [[out / key / f'{name}-{look}.png' for name in names] for look in ('light', 'dark')]
    if not shots[0][0].exists():
        return
    w, h = Image.open(shots[0][0]).size
    image = Image.new('RGB', (len(names) * (w + 24) + 24, 2 * (h + 24) + 64), '#f5f6f8')
    ImageDraw.Draw(image).text((24, 18), f'{key} · {w} × {h}', font=font, fill='#17202c')
    for row, paths in enumerate(shots):
        for col, path in enumerate(paths):
            if path.exists():
                with Image.open(path) as shot:
                    image.paste(shot, (24 + col * (w + 24), 64 + row * (h + 24)))
    image.save(out / f'sheet-{key}.png')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('variants', nargs='+')
    parser.add_argument('--out', type=Path, default=run.REPO / '.esphome' / 'map-tiles' / 'out')
    parser.add_argument('--tiles', type=Path, help='a folder of Home Assistant vector tiles, z_x_y.mvt')
    args = parser.parse_args()
    args.out = args.out.resolve()
    esphome = shlex.split(os.environ.get('ESPHOME', 'esphome'))
    streets = streets_from(args.tiles)
    pictures = run.Pictures()
    for key in args.variants:
        item = host.variant(key)
        build = host.Build(item, tree=run.REPO, work=None, esphome=esphome)
        started = time.monotonic()
        ok, output = build.compile()
        if not ok:
            print(f'{key}: BUILD FAILED\n{output[-3000:]}', flush=True)
            continue
        study = Study(build, args.out / key, pictures, streets)
        try:
            summary = asyncio.run(study.run())
            sheet(args.out, key, [name for name, _ in PAGES if (args.out / key / f'{name}-light.png').exists()])
        except Exception as error:
            summary = f'stopped: {type(error).__name__}: {error}'
        print(f'{key}: {summary} ({time.monotonic() - started:.0f} s) {study.warnings}', flush=True)


if __name__ == '__main__':
    main()
