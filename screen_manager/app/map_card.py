"""A map card: where the people of a person tile are, drawn by the add-on as the card's picture.

The screen never sees a coordinate. The add-on reads the people and zones from Home Assistant, reads the streets from
Home Assistant's own vector tiles (vector_tiles.py) and draws the whole frame at exactly the size the screen asks for,
in the screen's own colours. The screen places that picture like a live camera's.

How the card frames its people (`framing`):
- `everyone`: the smallest view that holds everyone on the card with a place, and the zone each of them stands in.
- `home`: the home zone in the middle at a fixed distance. Who is outside the view is a small chip on its edge,
  pointing the way.
- `person`: the tile's own person in the middle at a fixed distance, the others as on `home`.

`distance` is how far a fixed view reaches: street, neighbourhood, town or region.

The look is calm on purpose: the streets are a quiet ground in the screen's greys, water and green a soft tint of the
screen's own palette, and the only strong colours on the card are the people and their zones.
"""
import math
import re
from pathlib import Path

TILE_PX = 256
# Home Assistant's proxy serves no vector tile past zoom 14; a closer view draws the zoom-14 tile larger, as MapLibre
# does. Vectors stay sharp at any scale, so this costs nothing but detail no card has room for anyway.
VECTOR_MAX_ZOOM = 14
MIN_ZOOM, MAX_ZOOM = 3.0, 17.5
MAX_LATITUDE = 85.05112878
EARTH_METRES = 6371000.0

FRAMINGS = ('everyone', 'home', 'person')
DISTANCES = {'street': 16.5, 'neighbourhood': 15.0, 'town': 13.0, 'region': 10.5}
DEFAULT_DISTANCE = 'neighbourhood'
# Room kept around everyone on `everyone`, as a share of the frame on each side.
FIT_PADDING = 0.14
# One person alone, or everyone on one spot, is shown at this distance: a fit around one point has no size.
SINGLE_ZOOM = 15.0

# The layers of Home Assistant's Shortbread tiles the card draws; every other layer is skipped while decoding.
LAYERS = frozenset(('ocean', 'water_polygons', 'water_lines', 'land', 'buildings', 'streets', 'place_labels'))


# ----- Where things are -----

def world(lat, lon, zoom):
    """Web Mercator pixels of a place at `zoom`, 256 per tile as every web map."""
    lat = max(-MAX_LATITUDE, min(MAX_LATITUDE, lat))
    size = TILE_PX * 2 ** zoom
    x = (lon + 180.0) / 360.0 * size
    s = math.sin(math.radians(lat))
    y = (0.5 - math.log((1 + s) / (1 - s)) / (4 * math.pi)) * size
    return x, y


def metres_between(lat1, lon1, lat2, lon2):
    """Great-circle distance, good enough for zones and a scale."""
    p1, p2 = math.radians(lat1), math.radians(lat2)
    dp, dl = p2 - p1, math.radians(lon2 - lon1)
    a = math.sin(dp / 2) ** 2 + math.cos(p1) * math.cos(p2) * math.sin(dl / 2) ** 2
    return 2 * EARTH_METRES * math.asin(min(1.0, math.sqrt(a)))


def metres_per_pixel(lat, zoom):
    return 2 * math.pi * EARTH_METRES * math.cos(math.radians(lat)) / (TILE_PX * 2 ** zoom)


class View:
    """What the frame shows: a middle, a zoom that need not be whole, and the frame's size in pixels."""

    def __init__(self, lat, lon, zoom, size):
        self.lat, self.lon = lat, lon
        self.zoom = max(MIN_ZOOM, min(MAX_ZOOM, zoom))
        self.width, self.height = size
        cx, cy = world(lat, lon, self.zoom)
        self.left, self.top = cx - self.width / 2.0, cy - self.height / 2.0

    def point(self, lat, lon):
        x, y = world(lat, lon, self.zoom)
        return x - self.left, y - self.top

    def inside(self, lat, lon, margin=0):
        x, y = self.point(lat, lon)
        return margin <= x <= self.width - margin and margin <= y <= self.height - margin

    @property
    def tile_zoom(self):
        return max(0, min(VECTOR_MAX_ZOOM, int(math.floor(self.zoom))))

    def tiles(self):
        """(zoom, x, y) of every vector tile under the frame."""
        tz = self.tile_zoom
        scale = 2 ** (self.zoom - tz)
        span = TILE_PX * scale
        count = 2 ** tz
        x0, x1 = int(math.floor(self.left / span)), int(math.floor((self.left + self.width) / span))
        y0, y1 = int(math.floor(self.top / span)), int(math.floor((self.top + self.height) / span))
        return [(tz, x % count, y) for y in range(max(0, y0), min(count - 1, y1) + 1) for x in range(x0, x1 + 1)]

    def tile_origin(self, tz, tx, ty):
        """Where tile (tz, tx, ty) starts on the frame, and how many frame pixels one tile pixel is."""
        scale = 2 ** (self.zoom - tz)
        span = TILE_PX * scale
        # A tile that wraps round the date line is drawn where the frame needs it.
        count = 2 ** tz
        column = tx
        while column * span + span < self.left:
            column += count
        while column * span > self.left + self.width:
            column -= count
        return column * span - self.left, ty * span - self.top, scale


# ----- Who and what is on the card -----

class Zone:
    __slots__ = ('entity', 'name', 'lat', 'lon', 'radius', 'icon', 'home')

    def __init__(self, entity, name, lat, lon, radius, icon=None):
        self.entity, self.name, self.lat, self.lon, self.radius, self.icon = entity, name, lat, lon, radius, icon
        self.home = entity == 'zone.home'


class Person:
    __slots__ = ('entity', 'name', 'state', 'lat', 'lon', 'accuracy', 'colour', 'picture')

    def __init__(self, entity, name, state, lat=None, lon=None, accuracy=0, colour=0, picture=None):
        self.entity, self.name, self.state = entity, name, state
        self.lat, self.lon, self.accuracy, self.colour, self.picture = lat, lon, accuracy, colour, picture

    @property
    def placed(self):
        return self.lat is not None and self.lon is not None


def zones_of(states):
    """The zones Home Assistant has a place for, the home zone first."""
    found = []
    for entity, state in states.items():
        if not entity.startswith('zone.'):
            continue
        a = (state or {}).get('attributes') or {}
        try:
            lat, lon = float(a['latitude']), float(a['longitude'])
        except (KeyError, TypeError, ValueError):
            continue
        if a.get('passive'):
            continue
        found.append(Zone(entity, a.get('friendly_name') or entity.split('.', 1)[1], lat, lon,
                          float(a.get('radius') or 100.0), a.get('icon')))
    return sorted(found, key=lambda z: (not z.home, z.name))


def people_of(entities, states):
    """The people and trackers on the card, in the card's order, each with the colour slot it keeps."""
    out = []
    for slot, entity in enumerate(entities):
        state = states.get(entity) or {}
        a = state.get('attributes') or {}
        try:
            lat, lon = float(a['latitude']), float(a['longitude'])
        except (KeyError, TypeError, ValueError):
            lat = lon = None
        out.append(Person(entity, a.get('friendly_name') or entity.split('.', 1)[1], state.get('state'), lat, lon,
                          float(a.get('gps_accuracy') or 0), slot, a.get('entity_picture')))
    return out


def home_of(zones):
    return next((z for z in zones if z.home), None)


def zone_of(person, zones):
    """The zone a person stands in, the smallest when zones overlap."""
    if not person.placed:
        return None
    inside = [z for z in zones if metres_between(z.lat, z.lon, person.lat, person.lon) <= z.radius]
    return min(inside, key=lambda z: z.radius) if inside else None


def fit(points, size, reserve=(0, 0), margin=0):
    """The middle and zoom that hold every (lat, lon) in `points` in a frame of `size`, clear of the `reserve`d
    pixels at its top and bottom (the attribution above, the tile's name below) and `margin` from every edge (half a
    marker, so a marker at the edge of the fit is whole)."""
    xs, ys = zip(*(world(lat, lon, 0) for lat, lon in points))
    x0, x1, y0, y1 = min(xs), max(xs), min(ys), max(ys)
    top, bottom = reserve[0] + margin, reserve[1] + margin
    width, height = max(1, size[0] - 2 * margin), max(1, size[1] - top - bottom)
    if x1 - x0 < 1e-9 and y1 - y0 < 1e-9:
        zoom = SINGLE_ZOOM
    else:
        zoom = min(math.log2(width * (1 - 2 * FIT_PADDING) / max(x1 - x0, 1e-12)),
                   math.log2(height * (1 - 2 * FIT_PADDING) / max(y1 - y0, 1e-12)))
        zoom = min(zoom, SINGLE_ZOOM + 1.5)
    # Back from world pixels at zoom 0 to a place.
    # The middle of the free band goes in the middle of that band, so the frame's middle moves by half the difference.
    mx, my = (x0 + x1) / 2, (y0 + y1) / 2 + (bottom - top) / 2 / 2 ** zoom
    lon = mx / TILE_PX * 360.0 - 180.0
    n = math.pi - 2 * math.pi * my / TILE_PX
    lat = math.degrees(math.atan(math.sinh(n)))
    return lat, lon, zoom


def frame_view(framing, distance, people, zones, size, own=None, reserve=(0, 0), margin=0):
    """The view a card shows, and the people that fall outside it."""
    zoom = DISTANCES.get(distance, DISTANCES[DEFAULT_DISTANCE])
    placed = [p for p in people if p.placed]
    home = home_of(zones)
    if framing == 'home' and home:
        view = View(home.lat, home.lon, zoom, size)
    elif framing == 'person' and own is not None and own.placed:
        view = View(own.lat, own.lon, zoom, size)
    elif placed:
        points = [(p.lat, p.lon) for p in placed]
        # The zone someone stands in stays whole on the card, so its ring does not run off the edge.
        for p in placed:
            z = zone_of(p, zones)
            if z:
                d = z.radius / 111320.0
                w = d / max(0.01, math.cos(math.radians(z.lat)))
                points += [(z.lat + d, z.lon + w), (z.lat - d, z.lon - w)]
        lat, lon, fitted = fit(points, size, reserve, margin)
        view = View(lat, lon, fitted, size)
    elif home:
        view = View(home.lat, home.lon, zoom, size)
    else:
        view = View(0.0, 0.0, MIN_ZOOM, size)
    return view


# ----- The look -----
# The screen's own roles (components/smart_display/theme.h) where the card has one: the ground is PAGE_SOFT, the ink
# INK, the accent ACCENT, the halo CARD. The map's own tints (water, green, streets) are mixed from those, so light and
# dark stay one family with the rest of the screen.

def _mix(a, b, t):
    return tuple(round(a[i] + (b[i] - a[i]) * t) for i in range(3))


def _rgb(value):
    return ((value >> 16) & 255, (value >> 8) & 255, value & 255)


LIGHT = {
    'land': 0xF3F3F1, 'green': 0xE1EDDF, 'water': 0xCFE3EE, 'water_line': 0xB9D6E6, 'building': 0xE8E7E4,
    'road': 0xFFFFFF, 'road_case': 0xDCDCDA, 'major': 0xFFFFFF, 'major_case': 0xCFCFCC, 'path': 0xE2E2DF,
    'rail': 0xCDCDCA, 'place': 0x8A8A88, 'halo': 0xFFFFFF, 'ink': 0x1B1B1B, 'muted': 0x616161, 'card': 0xFFFFFF,
    'accent': 0x009FE3, 'zone_fill': 0x009FE3, 'zone_alpha': 30, 'zone_edge': 0x009FE3, 'shadow': 0x000000,
    'shadow_alpha': 60,
}
DARK = {
    'land': 0x1A1A1A, 'green': 0x1B251D, 'water': 0x0F2533, 'water_line': 0x0F2533, 'building': 0x1F1F1F,
    'road': 0x272727, 'road_case': 0x272727, 'major': 0x323232, 'major_case': 0x323232, 'path': 0x212121,
    'rail': 0x2A2A2A, 'place': 0x8A8A8A, 'halo': 0x1A1A1A, 'ink': 0xDADADA, 'muted': 0x999999, 'card': 0x1A1A1A,
    'accent': 0x0A93D2, 'zone_fill': 0x0A93D2, 'zone_alpha': 44, 'zone_edge': 0x2AA9E6, 'shadow': 0x000000,
    'shadow_alpha': 120,
}
# The people's colours, the tile palette's strong half (theme.h, the colour a tile paints its icon with), in the
# order Home Assistant hands out a map colour: the first person on the card is blue, the next orange, and so on.
PEOPLE = (0x009FE3, 0xEF7D14, 0x3C9A4A, 0x8E5CC9, 0xD9468F, 0x13897B, 0xD93A30, 0xC99A00)

# The widths of a street by kind, in pixels at a 170 dpi screen, at zoom 13, 15 and 17; between those they follow
# the zoom, as a real map's do. A kind absent from a zoom band is not drawn there.
STREETS = (
    # kinds, widths at (13, 15, 17), major (a darker edge)
    (('motorway', 'trunk'), (2.4, 5.0, 11.0), True),
    (('primary', 'secondary'), (1.8, 4.2, 10.0), True),
    (('tertiary',), (1.0, 3.2, 8.5), True),
    (('residential', 'unclassified', 'living_street'), (0.0, 2.0, 6.5), False),
    (('service', 'pedestrian'), (0.0, 0.9, 3.4), False),
)
PATHS = ('footway', 'cycleway', 'path', 'steps', 'track', 'bridleway')
RAILS = ('rail', 'light_rail', 'subway', 'narrow_gauge')
GREEN = frozenset(('park', 'forest', 'wood', 'grass', 'meadow', 'garden', 'village_green', 'recreation_ground',
                   'playground', 'cemetery', 'grave_yard', 'heath', 'scrub', 'grassland', 'nature_reserve',
                   'golf_course', 'allotments', 'orchard', 'vineyard', 'pitch'))
PLACES = {'capital': 11.0, 'state_capital': 11.0, 'city': 11.0, 'town': 12.5, 'village': 13.5, 'suburb': 13.5,
          'quarter': 14.5, 'neighbourhood': 15.5}

# The add-on's own copy of the screens' Roboto (screen_manager/app/fonts, the image is built from screen_manager/ alone;
# tests/test_map_card.py keeps it equal to fonts/), so a name on a map is in the face the screen writes names in.
FONT_DIRS = (Path(__file__).resolve().parent / 'fonts',)
_FONTS = {}


def font(size, weight=500):
    from PIL import ImageFont
    key = (round(size), weight)
    if key not in _FONTS:
        for folder in FONT_DIRS:
            path = folder / ('Roboto-%d.ttf' % weight)
            if path.exists():
                _FONTS[key] = ImageFont.truetype(str(path), key[0])
                break
        else:
            _FONTS[key] = ImageFont.load_default()
    return _FONTS[key]


def _width_at(widths, zoom):
    stops = (13.0, 15.0, 17.0)
    if zoom <= stops[0]:
        return widths[0] * 2 ** (zoom - stops[0]) if widths[0] else 0.0
    for i in range(2):
        if zoom <= stops[i + 1]:
            t = (zoom - stops[i]) / (stops[i + 1] - stops[i])
            a, b = widths[i], widths[i + 1]
            return a + (b - a) * t
    return widths[2] * 2 ** (zoom - stops[2])


# ----- Drawing -----

SUPERSAMPLE = 3


class Canvas:
    """A frame drawn three times as large and scaled down at the end: Pillow draws without anti-aliasing."""

    def __init__(self, size, colour):
        from PIL import Image
        self.width, self.height = size
        self.s = SUPERSAMPLE
        self.image = Image.new('RGB', (self.width * self.s, self.height * self.s), colour)

    def finish(self):
        from PIL import Image
        return self.image.resize((self.width, self.height), Image.Resampling.LANCZOS)


def _parts(view, tiles, layer):
    """(feature, [[(x, y) on the supersampled canvas]]) of one layer over every tile."""
    s = SUPERSAMPLE
    for (tz, tx, ty), decoded in tiles.items():
        if layer not in decoded:
            continue
        extent, features = decoded[layer]
        ox, oy, scale = view.tile_origin(tz, tx, ty)
        k = TILE_PX * scale / extent * s
        bx, by = ox * s, oy * s
        for feature in features:
            yield feature, [[(bx + x * k, by + y * k) for x, y in part] for part in feature.parts]


def _area(ring):
    return sum(ring[i][0] * ring[i + 1][1] - ring[i + 1][0] * ring[i][1] for i in range(len(ring) - 1))


def _fill(canvas, view, tiles, layer, colour, keep=None):
    """Every polygon of a layer in one colour: outer rings filled, holes cut, through one mask per layer."""
    from PIL import Image, ImageDraw
    mask = Image.new('L', canvas.image.size)
    draw = ImageDraw.Draw(mask)
    touched = False
    for feature, parts in _parts(view, tiles, layer):
        if feature.kind != 3 or (keep and not keep(feature)):
            continue
        # Outer rings wind one way and holes the other (a positive area here is an outer ring, y pointing down).
        for ring in parts:
            if len(ring) >= 3 and _area(ring) > 0:
                draw.polygon(ring, fill=255)
                touched = True
        for ring in parts:
            if len(ring) >= 3 and _area(ring) < 0:
                draw.polygon(ring, fill=0)
    if touched:
        canvas.image.paste(colour, mask=mask)


def _stroke(draw, parts, width):
    if width <= 0:
        return
    w = max(1, round(width))
    for line in parts:
        if len(line) >= 2:
            draw.line(line, fill=None, width=w, joint='curve')


def draw_basemap(canvas, view, tiles, look, dpi_scale=1.0):
    from PIL import ImageDraw
    s = SUPERSAMPLE
    zoom = view.zoom
    _fill(canvas, view, tiles, 'land', _rgb(look['green']), lambda f: f.tags.get('kind') in GREEN)
    _fill(canvas, view, tiles, 'ocean', _rgb(look['water']))
    _fill(canvas, view, tiles, 'water_polygons', _rgb(look['water']))
    draw = ImageDraw.Draw(canvas.image)
    # Rivers and canals too narrow to be a polygon.
    water_width = {'river': (1.2, 3.0, 7.0), 'canal': (0.0, 2.2, 5.0), 'stream': (0.0, 0.8, 2.0)}
    for kind, widths in water_width.items():
        w = _width_at(widths, zoom) * dpi_scale * s
        if w > 0.4 * s:
            parts = [p for f, ps in _parts(view, tiles, 'water_lines') if f.tags.get('kind') == kind for p in ps]
            draw = ImageDraw.Draw(canvas.image)
            for line in parts:
                if len(line) >= 2:
                    draw.line(line, fill=_rgb(look['water']), width=max(1, round(w)), joint='curve')
    if zoom >= 14.5:
        _fill(canvas, view, tiles, 'buildings', _rgb(look['building']))
    draw = ImageDraw.Draw(canvas.image)
    streets = [(f, ps) for f, ps in _parts(view, tiles, 'streets') if not f.tags.get('tunnel')]
    # Paths first and faint, then rail, then streets from small to large: casings of all, then the fills.
    if zoom >= 15.5:
        w = max(0.6, 0.35 * (zoom - 14)) * dpi_scale * s
        for f, ps in streets:
            if f.tags.get('kind') in PATHS:
                for line in ps:
                    if len(line) >= 2:
                        draw.line(line, fill=_rgb(look['path']), width=max(1, round(w)))
    if zoom >= 12:
        w = max(0.7, 0.3 * (zoom - 11)) * dpi_scale * s
        for f, ps in streets:
            if f.tags.get('kind') in RAILS and not f.tags.get('service'):
                for line in ps:
                    if len(line) >= 2:
                        draw.line(line, fill=_rgb(look['rail']), width=max(1, round(w)))
    layers = []
    for kinds, widths, major in reversed(STREETS):
        w = _width_at(widths, zoom) * dpi_scale
        if w < 0.5:
            continue
        parts = [p for f, ps in streets if f.tags.get('kind') in kinds for p in ps]
        layers.append((parts, w * s, major))
    edge = 1.0 * dpi_scale * s
    for parts, w, major in layers:
        colour = _rgb(look['major_case'] if major else look['road_case'])
        for line in parts:
            if len(line) >= 2:
                draw.line(line, fill=colour, width=max(1, round(w + 2 * edge)), joint='curve')
    for parts, w, major in layers:
        colour = _rgb(look['major'] if major else look['road'])
        for line in parts:
            if len(line) >= 2:
                draw.line(line, fill=colour, width=max(1, round(w)), joint='curve')


def draw_places(canvas, view, tiles, look, dpi_scale, avoid):
    """A few names of towns and quarters, quiet and never over a person or a zone."""
    from PIL import ImageDraw
    s = SUPERSAMPLE
    draw = ImageDraw.Draw(canvas.image)
    size = 11.5 * dpi_scale * s
    face = font(size, 500)
    taken = list(avoid)
    shown = 0
    candidates = []
    for f, parts in _parts(view, tiles, 'place_labels'):
        kind, name = f.tags.get('kind'), f.tags.get('name')
        if not name or kind not in PLACES or not parts or not parts[0]:
            continue
        # A kind reads at its own zooms: a city from afar, a quarter up close.
        if not (PLACES[kind] - 2.0 <= view.zoom <= PLACES[kind] + 2.5):
            continue
        x, y = parts[0][0]
        candidates.append((-(f.tags.get('population') or 0), PLACES[kind], name, x, y))
    seen = set()
    for _, _, name, x, y in sorted(candidates):
        # A place's name is in every tile it lies near: once is enough.
        if name in seen:
            continue
        seen.add(name)
        box = draw.textbbox((x, y), name, font=face, anchor='mm')
        pad = 4 * s
        box = (box[0] - pad, box[1] - pad, box[2] + pad, box[3] + pad)
        if box[0] < 6 * s or box[1] < 6 * s or box[2] > canvas.width * s - 6 * s or box[3] > canvas.height * s - 6 * s:
            continue
        if any(box[0] < b[2] and b[0] < box[2] and box[1] < b[3] and b[1] < box[3] for b in taken):
            continue
        draw.text((x, y), name, font=face, anchor='mm', fill=_rgb(look['place']),
                  stroke_width=round(2 * s * dpi_scale), stroke_fill=_rgb(look['halo']))
        taken.append(box)
        shown += 1
        if shown >= 2:
            break


def draw_zones(canvas, view, zones, look, dpi_scale, marker, occupied=()):
    from PIL import Image, ImageDraw
    s = SUPERSAMPLE
    overlay = Image.new('RGBA', canvas.image.size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay)
    boxes = []
    for z in zones:
        x, y = view.point(z.lat, z.lon)
        r = z.radius / metres_per_pixel(z.lat, view.zoom)
        # A zone smaller than a marker is a marker's business when someone is in it, and otherwise still a place:
        # a small ring, so a school or an office stays findable from afar.
        if r < marker / 2 and z.entity in occupied:
            continue
        r = max(r, 5 * dpi_scale)
        if x + r < 0 or y + r < 0 or x - r > canvas.width or y - r > canvas.height:
            continue
        X, Y, R = x * s, y * s, r * s
        # A zone that fills the card is a tint, not a slab: its fill fades as it grows past the card.
        alpha = round(look['zone_alpha'] * min(1.0, 0.45 * min(canvas.width, canvas.height) / max(r, 1)))
        draw.ellipse((X - R, Y - R, X + R, Y + R), fill=_rgb(look['zone_fill']) + (max(8, alpha),),
                     outline=_rgb(look['zone_edge']) + (200,), width=max(1, round(1.3 * dpi_scale * s)))
        boxes.append((X - R, Y - R, X + R, Y + R))
    canvas.image.paste(Image.alpha_composite(canvas.image.convert('RGBA'), overlay).convert('RGB'))
    return boxes


def initials(name):
    words = [w for w in re.split(r'[\s\-_.]+', name or '') if w]
    if not words:
        return '?'
    if len(words) == 1:
        return words[0][:1].upper()
    return (words[0][:1] + words[-1][:1]).upper()


def _spread(points, gap):
    """Markers on one spot fanned out round it, so everyone at home stays visible."""
    out = list(points)
    for i in range(len(out)):
        group = [j for j in range(len(out)) if math.hypot(out[j][0] - out[i][0], out[j][1] - out[i][1]) < gap]
        if len(group) > 1 and group[0] == i:
            cx = sum(out[j][0] for j in group) / len(group)
            cy = sum(out[j][1] for j in group) / len(group)
            r = gap * 0.58
            for n, j in enumerate(group):
                a = -math.pi / 2 + 2 * math.pi * n / len(group)
                out[j] = (cx + r * math.cos(a), cy + r * math.sin(a))
    return out


def draw_people(canvas, view, people, look, dpi_scale, marker, names=False):
    """A disc per person, Home Assistant's own marker: the card's colour inside a ring in the person's colour,
    their initials in the middle. Who is outside the view is a small chip at the edge that points the way."""
    from PIL import Image, ImageDraw, ImageFilter
    s = SUPERSAMPLE
    placed = [p for p in people if p.placed]
    edge_margin = marker * 0.5 + 4 * dpi_scale
    inside = [p for p in placed if view.inside(p.lat, p.lon, -marker * 0.2)]
    outside = [p for p in placed if p not in inside]
    spots = _spread([view.point(p.lat, p.lon) for p in inside], marker * 1.1)
    chips = []
    for p in outside:
        x, y = view.point(p.lat, p.lon)
        cx, cy = canvas.width / 2, canvas.height / 2
        dx, dy = x - cx, y - cy
        t = min((cx - edge_margin) / abs(dx) if dx else 1e9, (cy - edge_margin) / abs(dy) if dy else 1e9)
        chips.append((p, (cx + dx * t, cy + dy * t), math.atan2(dy, dx)))
    shadow = Image.new('L', canvas.image.size)
    sd = ImageDraw.Draw(shadow)
    discs = [(p, x, y, marker) for p, (x, y) in zip(inside, spots)]
    discs += [(p, x, y, marker * 0.72) for p, (x, y), _ in chips]
    for p, x, y, d in discs:
        R = d / 2 * s
        oy = 1.2 * dpi_scale * s
        sd.ellipse((x * s - R, y * s - R + oy, x * s + R, y * s + R + oy), fill=look['shadow_alpha'])
    shadow = shadow.filter(ImageFilter.GaussianBlur(2.2 * dpi_scale * s))
    canvas.image.paste(_rgb(look['shadow']), mask=shadow)
    draw = ImageDraw.Draw(canvas.image)
    boxes, taken = [], []
    for p, (x, y), angle in chips:
        # The arrow first, so the disc sits on its root.
        d = marker * 0.72
        tip = d / 2 + 5 * dpi_scale
        ax, ay = x + math.cos(angle) * tip, y + math.sin(angle) * tip
        side = 4.5 * dpi_scale
        bx, by = x + math.cos(angle) * (d / 2 - 1), y + math.sin(angle) * (d / 2 - 1)
        nx, ny = -math.sin(angle) * side, math.cos(angle) * side
        draw.polygon([(ax * s, ay * s), ((bx + nx) * s, (by + ny) * s), ((bx - nx) * s, (by - ny) * s)],
                     fill=PEOPLE_RGB[p.colour % len(PEOPLE_RGB)])
    for p, x, y, d in discs:
        colour = PEOPLE_RGB[p.colour % len(PEOPLE_RGB)]
        R, ring = d / 2 * s, max(2, round(0.12 * d * s))
        draw.ellipse((x * s - R, y * s - R, x * s + R, y * s + R), fill=colour)
        r = R - ring
        draw.ellipse((x * s - r, y * s - r, x * s + r, y * s + r), fill=_rgb(look['card']))
        face = font(d * 0.40 * s, 500)
        draw.text((x * s, y * s + 0.5 * s), initials(p.name), font=face, anchor='mm', fill=_rgb(look['ink']))
        boxes.append((x * s - R, y * s - R, x * s + R, y * s + R))
    if names:
        # The first name beside each marker, on the card's colour, where it does not cover another marker.
        face = font(12.5 * dpi_scale * s, 500)
        for p, x, y, d in discs[:len(inside)]:
            text = (p.name or '').split(' ')[0]
            w = draw.textlength(text, font=face)
            h, pad = 19 * dpi_scale * s, 7 * dpi_scale * s
            left = x * s + d / 2 * s + 3 * dpi_scale * s
            if left + w + 2 * pad > canvas.width * s - 4 * s:
                left = x * s - d / 2 * s - 3 * dpi_scale * s - w - 2 * pad
            box = (left, y * s - h / 2, left + w + 2 * pad, y * s + h / 2)
            if any(box[0] < b[2] and b[0] < box[2] and box[1] < b[3] and b[1] < box[3] for b in boxes + taken):
                continue
            draw.rounded_rectangle(box, radius=h / 2, fill=_rgb(look['card']))
            draw.text((left + pad, y * s), text, font=face, anchor='lm', fill=_rgb(look['ink']))
            taken.append(box)
    return boxes


PEOPLE_RGB = [_rgb(c) for c in PEOPLE]
ATTRIBUTION = '© OpenStreetMap'


def draw_attribution(canvas, look, dpi_scale):
    from PIL import ImageDraw
    s = SUPERSAMPLE
    draw = ImageDraw.Draw(canvas.image)
    face = font(9.5 * dpi_scale * s, 400)
    x, y = canvas.width * s - 7 * dpi_scale * s, 6 * dpi_scale * s
    draw.text((x, y), ATTRIBUTION, font=face, anchor='ra', fill=_rgb(look['muted']),
              stroke_width=round(1.6 * s * dpi_scale), stroke_fill=_rgb(look['halo']))


def marker_size(size, dpi_scale):
    """A marker is a finger's width on a big card and a little less on a small one."""
    short = min(size)
    return max(22.0, min(34.0, short * 0.2)) * dpi_scale if short < 170 * dpi_scale else 34.0 * dpi_scale


def render(size, people, zones, tiles, framing='everyone', distance=DEFAULT_DISTANCE, dark=False, dpi_scale=1.0,
           own=None, names=None, name=None, label_px=None, inset=None):
    """The card's picture: the streets, the zones, the people and the tile's name, in the look the screen is in.

    `tiles` are the decoded vector tiles of `view_for` ({} draws the card without streets); `name` goes on a pill at the
    bottom left in the card's own colours, `label_px` high as the screen writes a tile's name (its FONT_LABEL_SIZE)."""
    look = DARK if dark else LIGHT
    label_px = label_px or 18 * dpi_scale
    inset = inset if inset is not None else 8 * dpi_scale
    pill_h = round(label_px * 1.55)
    marker = marker_size(size, dpi_scale)
    view = frame_view(framing, distance, people, zones, size, own, reserve(dpi_scale, pill_h, inset, bool(name)), marker * 0.6)
    canvas = Canvas(size, _rgb(look['land']))
    if tiles:
        draw_basemap(canvas, view, tiles, look, dpi_scale)
    occupied = {z.entity for z in (zone_of(p, zones) for p in people) if z}
    zone_boxes = draw_zones(canvas, view, zones, look, dpi_scale, marker, occupied)
    if names is None:
        names = min(size) >= 200 * dpi_scale
    people_boxes = draw_people(canvas, view, people, look, dpi_scale, marker, names)
    if tiles and min(size) >= 150 * dpi_scale:
        draw_places(canvas, view, tiles, look, dpi_scale, people_boxes + zone_boxes)
    if tiles:
        draw_attribution(canvas, look, dpi_scale)
    if name:
        draw_name(canvas, name, look, label_px, pill_h, inset)
    return canvas.finish(), view


def reserve(dpi_scale, pill_h, inset, named):
    """The rows a fit keeps free: the attribution's line at the top, the name's pill at the bottom."""
    return 16 * dpi_scale, (pill_h + inset + 4 * dpi_scale) if named else 8 * dpi_scale


def draw_name(canvas, name, look, label_px, pill_h, inset):
    """The tile's name on a pill at the bottom left, as legible on streets as on a card: ink on the card's colour."""
    from PIL import ImageDraw
    s = SUPERSAMPLE
    draw = ImageDraw.Draw(canvas.image)
    face = font(label_px * s, 500)
    pad = pill_h * 0.42
    room = canvas.width - 2 * inset - 2 * pad
    text = name
    while text and draw.textlength(text, font=face) > room * s:
        text = text[:-1]
    if text != name:
        text = text.rstrip()[:-1] + '…' if len(text) > 1 else text
    width = draw.textlength(text, font=face) / s
    x, y = inset, canvas.height - inset - pill_h
    draw.rounded_rectangle((x * s, y * s, (x + width + 2 * pad) * s, (y + pill_h) * s), radius=pill_h / 2 * s,
                           fill=_rgb(look['card']))
    draw.text(((x + pad) * s, (y + pill_h / 2) * s), text, font=face, anchor='lm', fill=_rgb(look['ink']))


# ----- A map tile of a layout -----

# Places go into the mark on a grid of about this many metres, so a phone's drift does not ask for a new picture.
MARK_METRES = 25
MARK_DEGREES = MARK_METRES / 111320.0


def options_of(tile):
    options = tile.get('options') or {}
    framing = options.get('framing', FRAMINGS[0])
    distance = options.get('distance', DEFAULT_DISTANCE)
    return (framing if framing in FRAMINGS else FRAMINGS[0], distance if distance in DISTANCES else DEFAULT_DISTANCE,
            options.get('overlay', 'name') != 'none')


def shown(tile):
    """Everyone on the card: the tile's own person first, then who rides along, each once."""
    out = [tile['entity']]
    for entity in (tile.get('options') or {}).get('map') or ():
        if isinstance(entity, str) and entity not in out:
            out.append(entity)
    return out


def name_of(tile, states):
    """The tile's name as the screen would write it: its own, else Home Assistant's."""
    if tile.get('name'):
        return tile['name']
    attributes = (states.get(tile['entity']) or {}).get('attributes') or {}
    return attributes.get('friendly_name') or tile['entity'].split('.', 1)[1]


def fingerprint(tile, states):
    """The movement mark: a short hash of all a card is drawn from (its choices and name, where its people are on a
    grid of MARK_METRES, their states and names, the zones), never a place itself. The screen asks for a new picture
    when it changes and never otherwise."""
    import hashlib
    import json
    framing, distance, overlay = options_of(tile)
    people = []
    for entity in shown(tile):
        state = states.get(entity) or {}
        a = state.get('attributes') or {}
        try:
            cell = (round(float(a['latitude']) / MARK_DEGREES), round(float(a['longitude']) / MARK_DEGREES))
        except (KeyError, TypeError, ValueError):
            cell = None
        people.append((entity, state.get('state'), a.get('friendly_name'), cell))
    zones = [(z.entity, z.name, round(z.lat, 5), round(z.lon, 5), round(z.radius)) for z in zones_of(states)]
    what = [framing, distance, overlay, name_of(tile, states) if overlay else '', people, zones]
    return hashlib.sha1(json.dumps(what, sort_keys=True, default=str).encode()).hexdigest()[:12]


class Board:
    """What a screen's board says about drawing a card: its pixels per inch against the 170 of the 4-inch Guition every
    size here is written for, the size its tile names are in and the inset of a card's contents."""

    def __init__(self, shape=None):
        shape = shape or {}
        self.scale = max(0.6, min(2.0, float(shape.get('dpi') or 170) / 170.0))
        self.label = int((shape.get('fonts') or {}).get('label') or round(18 * self.scale))
        self.inset = round(((shape.get('spacing') or {}).get('tile_pad') or 12) * 0.7)


def view_for(tile, states, size, board):
    """The view a map tile shows at `size`: what its tiles are asked for with (View.tiles)."""
    framing, distance, overlay = options_of(tile)
    people, zones = people_of(shown(tile), states), zones_of(states)
    pill_h = round(board.label * 1.55)
    return frame_view(framing, distance, people, zones, size, people[0],
                      reserve(board.scale, pill_h, board.inset, overlay), marker_size(size, board.scale) * 0.6)


def render_tile(tile, states, size, board, dark=False, tiles=None):
    """The picture of one map tile of a layout at exactly `size`."""
    framing, distance, overlay = options_of(tile)
    people, zones = people_of(shown(tile), states), zones_of(states)
    image, _ = render(size, people, zones, tiles or {}, framing, distance, dark, board.scale, people[0],
                      name=name_of(tile, states) if overlay else None, label_px=board.label, inset=board.inset)
    return image
