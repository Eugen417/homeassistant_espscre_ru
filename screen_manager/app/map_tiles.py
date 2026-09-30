"""The streets under a map card, from Home Assistant's own `map_tiles` integration (app 0.4.33).

Home Assistant Core serves OpenStreetMap's Shortbread vector tiles at `/api/map_tiles/vector/{z}/{x}/{y}.mvt`: it
fetches them with its own identification, as the OpenStreetMap tile policy asks, and keeps them. The add-on asks Home
Assistant and never a tile server, and a request names a zoom and two whole numbers: no entity, no name, no place of a
person beyond which square of the map the card shows.

One source for the whole add-on: the raw tiles in a bounded cache for a week (Home Assistant's own TILE_TTL), the last
few read ones kept decoded, and a pause when Home Assistant has no tiles to give (an older Core, no internet). A card
is then drawn without streets, from the zones and the people alone, and nothing else changes.
"""
import asyncio
from collections import OrderedDict
import logging
import time

import vector_tiles
from map_card import LAYERS, VECTOR_MAX_ZOOM

LOG = logging.getLogger(__name__)

PATH = '/map_tiles/vector'
# Home Assistant keeps a tile this long before it asks again (map_tiles.const.TILE_TTL): ours are no fresher.
TILE_TTL = 7 * 24 * 60 * 60
# A dense city tile is some 350 KB as Home Assistant sends it: this holds a town's worth.
CACHE_BYTES = 24 * 1024 * 1024
# Decoded tiles are Python objects, several times their bytes: only a page's worth stays read.
DECODED_KEPT = 16
MAX_TILE_BYTES = 4 * 1024 * 1024
FETCH_SECONDS = 15
MAX_CONCURRENT = 4
# How long a Home Assistant without tiles is left alone before it is asked again.
COOLDOWN = 600


class TileSource:
    """Vector tiles through Home Assistant: `fetch(z, x, y)` is an awaitable of the tile's bytes (server.HomeAssistant),
    injected so the tests need no network."""

    def __init__(self, fetch, clock=time.monotonic):
        self.fetch, self.clock = fetch, clock
        self.raw = OrderedDict()       # (z, x, y) -> (bytes, fetched at)
        self.decoded = OrderedDict()   # (z, x, y) -> {layer: (extent, features)}
        self.bytes = 0
        self.quiet_until = 0.0
        self.said = False
        self.gate = asyncio.Semaphore(MAX_CONCURRENT)
        self.pending = {}

    def available(self):
        return self.clock() >= self.quiet_until

    def _quiet(self, error):
        self.quiet_until = self.clock() + COOLDOWN
        if not self.said:
            LOG.info('Home Assistant gives no map tiles (%s): map cards are drawn without streets, and the app asks '
                     'again in %d minutes', type(error).__name__, COOLDOWN // 60)
            self.said = True

    def _keep(self, key, raw):
        if key in self.raw:
            self.bytes -= len(self.raw.pop(key)[0])
        self.raw[key] = (raw, self.clock())
        self.bytes += len(raw)
        while self.bytes > CACHE_BYTES and self.raw:
            _, (old, _) = self.raw.popitem(last=False)
            self.bytes -= len(old)

    async def _raw(self, key):
        found = self.raw.get(key)
        if found and self.clock() - found[1] < TILE_TTL:
            self.raw.move_to_end(key)
            return found[0]
        if key in self.pending:
            return await self.pending[key]
        task = asyncio.ensure_future(self._fetch(key))
        self.pending[key] = task
        try:
            return await task
        finally:
            self.pending.pop(key, None)

    async def _fetch(self, key):
        async with self.gate:
            raw = await self.fetch(*key)
        if not isinstance(raw, (bytes, bytearray)) or len(raw) > MAX_TILE_BYTES:
            raise ValueError('not a tile')
        self._keep(key, bytes(raw))
        self.decoded.pop(key, None)
        return bytes(raw)

    async def tile(self, z, x, y):
        """The decoded layers a card draws of one tile, or None when there is none to be had."""
        key = (z, x, y)
        if type(z) is not int or type(x) is not int or type(y) is not int or not 0 <= z <= VECTOR_MAX_ZOOM \
                or not 0 <= x < (1 << z) or not 0 <= y < (1 << z):
            return None
        if key in self.decoded and key in self.raw:
            self.decoded.move_to_end(key)
            return self.decoded[key]
        raw = await self._raw(key)
        decoded = await asyncio.get_running_loop().run_in_executor(None, vector_tiles.decode, raw, LAYERS)
        self.decoded[key] = decoded
        while len(self.decoded) > DECODED_KEPT:
            self.decoded.popitem(last=False)
        return decoded

    async def tiles(self, keys):
        """{(z, x, y): decoded} of every tile of a view that Home Assistant gave; {} while it gives none."""
        if not keys or not self.available():
            return {}
        results = await asyncio.gather(*(self.tile(*key) for key in keys), return_exceptions=True)
        errors = [r for r in results if isinstance(r, BaseException)]
        if errors and len(errors) == len(results):
            self._quiet(errors[0])
            return {}
        if errors:
            LOG.info('%d of %d map tiles did not come (%s)', len(errors), len(results), type(errors[0]).__name__)
        else:
            self.said = False
        return {key: r for key, r in zip(keys, results) if r is not None and not isinstance(r, BaseException)}
