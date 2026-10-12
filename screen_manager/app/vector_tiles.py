"""The streets of a map card, read from Home Assistant's vector tiles.

Home Assistant Core's `map_tiles` integration serves OpenStreetMap's Shortbread vector tiles
(`/api/map_tiles/vector/{z}/{x}/{y}.mvt`), the same tiles its own map card draws with its own colours. A vector tile
carries what is there (a street of this kind, water, a park) and no look, so the add-on draws it in the screen's own
colours (map_card.py) instead of repainting someone else's picture.

This module only reads the Mapbox Vector Tile format (a protocol buffer, version 2 of the specification): the layers
asked for, each feature's kind and its geometry in the tile's own coordinates. No dependency: the add-on's image
carries Pillow but no protobuf decoder for this schema, and the format is small enough to read by hand.
"""
import gzip
import struct

# Geometry types of the specification.
POINT, LINE, POLYGON = 1, 2, 3
# A tile larger than this once unpacked is refused: a dense city tile at zoom 14 is under 1 MB.
MAX_BYTES = 8 * 1024 * 1024


def _varint(data, i):
    """The unsigned varint at `i`, and the index after it."""
    result = shift = 0
    while True:
        byte = data[i]
        i += 1
        result |= (byte & 0x7F) << shift
        if byte < 0x80:
            return result, i
        shift += 7
        if shift > 63:
            raise ValueError('varint too long')


def _fields(data, start=0, end=None):
    """(field number, wire type, value) of one message: a varint as an int, a length as a memoryview."""
    i, end = start, len(data) if end is None else end
    while i < end:
        key, i = _varint(data, i)
        number, wire = key >> 3, key & 7
        if wire == 0:
            value, i = _varint(data, i)
        elif wire == 2:
            size, i = _varint(data, i)
            if i + size > end:
                raise ValueError('field runs past its message')
            value, i = data[i:i + size], i + size
        elif wire == 1:
            value, i = data[i:i + 8], i + 8
        elif wire == 5:
            value, i = data[i:i + 4], i + 4
        else:
            raise ValueError('unknown wire type %d' % wire)
        yield number, wire, value


def _packed(data):
    """The unsigned varints of a packed field."""
    out, i, end = [], 0, len(data)
    append = out.append
    while i < end:
        byte = data[i]
        i += 1
        if byte < 0x80:
            append(byte)
            continue
        result, shift = byte & 0x7F, 7
        while True:
            byte = data[i]
            i += 1
            result |= (byte & 0x7F) << shift
            if byte < 0x80:
                break
            shift += 7
        append(result)
    return out


def _value(data):
    """A layer's value: a string, number or bool."""
    for number, wire, value in _fields(data):
        if number == 1:
            return bytes(value).decode('utf-8', 'replace')
        if number == 2:
            return struct.unpack('<f', value)[0]
        if number == 3:
            return struct.unpack('<d', value)[0]
        if number in (4, 5):
            return value
        if number == 6:
            return (value >> 1) ^ -(value & 1)
        if number == 7:
            return bool(value)
    return None


def geometry(commands, kind):
    """The parts of one feature in tile coordinates: points, lines, or polygon rings (outer rings and holes as the
    tile orders them; a ring's winding tells which, and drawing with even-odd needs neither)."""
    parts, part = [], None
    x = y = 0
    i, count = 0, len(commands)
    while i < count:
        command = commands[i]
        i += 1
        op, repeat = command & 7, command >> 3
        if op == 7:
            if part:
                part.append(part[0])
            continue
        for _ in range(repeat):
            if i + 1 >= count + 1 or i + 1 > count:
                return parts
            dx, dy = commands[i], commands[i + 1]
            i += 2
            x += (dx >> 1) ^ -(dx & 1)
            y += (dy >> 1) ^ -(dy & 1)
            if op == 1:
                part = [(x, y)]
                parts.append(part)
            elif part is not None:
                part.append((x, y))
    return parts


class Feature:
    """One thing on the map: its geometry type, its properties and its parts in the tile's own coordinates."""
    __slots__ = ('kind', 'tags', 'parts')

    def __init__(self, kind, tags, parts):
        self.kind, self.tags, self.parts = kind, tags, parts


def decode(raw, layers=None):
    """{layer name: (extent, [Feature])} of a tile, only the layers in `layers` when given. `raw` may be gzipped, as
    Home Assistant serves it."""
    if raw[:2] == b'\x1f\x8b':
        raw = gzip.decompress(raw)
    if len(raw) > MAX_BYTES:
        raise ValueError('tile too large')
    data = memoryview(raw)
    out = {}
    for number, wire, layer in _fields(data):
        if number != 3 or wire != 2:
            continue
        name, extent, keys, values, features = None, 4096, [], [], []
        for field, kind, value in _fields(layer):
            if field == 1:
                name = bytes(value).decode('utf-8', 'replace')
                if layers is not None and name not in layers:
                    break
            elif field == 5:
                extent = value
            elif field == 3:
                keys.append(bytes(value).decode('utf-8', 'replace'))
            elif field == 4:
                values.append(_value(value))
            elif field == 2:
                features.append(value)
        else:
            if name is None:
                continue
            decoded = []
            for feature in features:
                kind, tags, commands = 0, (), ()
                for field, wire, value in _fields(feature):
                    if field == 3:
                        kind = value
                    elif field == 2:
                        tags = _packed(value)
                    elif field == 4:
                        commands = _packed(value)
                properties = {}
                for k in range(0, len(tags) - 1, 2):
                    if tags[k] < len(keys) and tags[k + 1] < len(values):
                        properties[keys[tags[k]]] = values[tags[k + 1]]
                decoded.append(Feature(kind, properties, geometry(commands, kind)))
            out[name] = (extent, decoded)
    return out
