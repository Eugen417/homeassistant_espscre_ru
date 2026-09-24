"""Protocol 2 compilation and one serialized, acknowledged sender per screen.

Only the add-on knows older protocols. New firmware receives a complete page
configuration through a bounded transaction, followed by value-only updates.
The document revision used for saving is separate from this content revision.
"""
import asyncio
from copy import deepcopy
import json

from page_layout import compile_tiles, fingerprint, grid_of_record, new_id

PROTOCOL = 2
MAX_MESSAGE = 4096


class DeliveryError(RuntimeError):
    """A transient transport failure. Retry from a new device-granted session."""


class Refused(DeliveryError):
    """The device refused deterministic input. Do not repeatedly reload it."""


class Superseded(DeliveryError):
    """A newer saved document replaced this delivery target."""


def bounded(message):
    if len(json.dumps(message, ensure_ascii=False, separators=(",", ":")).encode()) > MAX_MESSAGE:
        raise Refused("A page message exceeds the screen's 4096-byte limit")
    return message


def configuration(record, region):
    """Editor workspace and legacy device settings never change this revision."""
    return fingerprint({"layout": record["layout"], "grid": record["sourceGrid"], "region": region})


def page_message(page, index, items, *, initial):
    message = {"op": "page" if initial else "bar", "p": index, "items": items}
    if initial:
        title = page["topbar"]["title"]
        message.update(id=page["id"], title=title.get("text", "") if title["source"] == "text" else "",
                       home_control=bool(page["topbar"]["leading"]), excluded=page["navigation"]["excludeFromPagination"])
    return message


def tile_message(message, tile, *, initial):
    message = deepcopy(message)
    message.pop("v", None)
    if initial:
        message.update(op="tile", slot=tile["slot"])
        message.setdefault("o", {})
    else:
        message["op"] = "state"
        # HA may change an automatic icon with the entity's state. This is a
        # resolved value, separate from the saved icon choice in configuration.
        message["icon"] = message.pop("o", {}).get("icon", "")
    return message


def prepare(inbox, record, region, values, bars):
    """Compile and bound the entire transaction before persistence or delivery.

    Initial states are supplied by the caller so this pure boundary has no HA
    side effects. The real delivery also checks refreshed values here.
    """
    revision = configuration(record, region)
    tiles = compile_tiles(record["layout"], grid_of_record(record))
    pages = record["layout"]["pages"]
    if len(values) != len(tiles) or len(bars) != len(pages):
        raise Refused("Incomplete resolved page configuration")
    initial_tiles = [tile_message(value, tile, initial=True) for value, tile in zip(values, tiles)]
    initial_bars = [page_message(page, i, bar, initial=True) for i, (page, bar) in enumerate(zip(pages, bars))]
    live_values = [tile_message(value, tile, initial=False) for value, tile in zip(values, tiles)]
    begin = {"op": "begin", "inbox": inbox, "title": record["layout"]["title"], "pages": len(pages),
             "tiles": len(tiles), "home": next(i for i, p in enumerate(pages) if p["id"] == record["layout"]["homePageId"]), **region}
    for message in [begin, *initial_tiles, *initial_bars, *live_values]:
        bounded({**message, "v": PROTOCOL, "session": "0" * 16, "seq": 0xFFFFFFFF, "rev": revision})
    return begin, initial_tiles, initial_bars, live_values


class Sender:
    """All layout, value, history, option and image messages share this lock.

    send(message) must return the response to that particular API call. It must
    not substitute HA's cached text-entity state. alive() checks that the saved
    configuration being sent is still current after each asynchronous step.
    """

    def __init__(self, send):
        self.send = send
        self.lock = asyncio.Lock()
        self.session = None
        self.sequence = 0
        self.revision = None
        self.confirmed = None
        self.failed_revision = None
        self.failure = None
        self.values, self.bars = [], []
        self.phase = "waiting"
        self.protocol = None
        self.tile_sizes = {"single", "wide", "full"}
        self.last_protocol = None
        self.last_tile_sizes = set(self.tile_sizes)

    def disconnected(self):
        self.session = self.confirmed = self.protocol = None
        self.tile_sizes = {"single", "wide", "full"}
        self.phase = "waiting"
        self.failed_revision = self.failure = None

    async def _hello(self):
        request = new_id()
        answer = await self.send({"v": PROTOCOL, "op": "hello", "request": request})
        if isinstance(answer, dict) and answer.get("protocol") == PROTOCOL:
            session = answer.get("session", "")
            if answer.get("request") != request or len(session) != 16 or any(c not in "0123456789abcdef" for c in session):
                raise DeliveryError("The screen returned an unrelated session")
            if answer.get("status") != "Session:" + session:
                raise DeliveryError("The screen did not grant a session")
            sizes = answer.get("tile_sizes", ["single", "wide", "full"])
            self.tile_sizes = {size for size in sizes if isinstance(size, str)} if isinstance(sizes, list) else {"single", "wide", "full"}
            self.protocol, self.session, self.sequence = PROTOCOL, session, 0
            self.last_protocol, self.last_tile_sizes = PROTOCOL, set(self.tile_sizes)
            return PROTOCOL
        # This is an answer from the running old firmware, not cached registry metadata.
        if isinstance(answer, dict) and answer.get("protocol") in (None, 1) and answer.get("status") == "Error: protocol version":
            self.protocol, self.session = 1, None
            self.last_protocol, self.last_tile_sizes = 1, {"single", "wide", "full"}
            return 1
        raise DeliveryError("The running screen's protocol could not be verified")

    async def probe(self):
        async with self.lock:
            return await self._hello()

    async def _packet(self, message, revision):
        if not self.session:
            raise DeliveryError("No verified screen session")
        self.sequence += 1
        if self.sequence > 0xFFFFFFFF:
            raise DeliveryError("A fresh screen session is required")
        packet = bounded({**message, "v": PROTOCOL, "session": self.session, "seq": self.sequence, "rev": revision})
        answer = await self.send(packet)
        if not isinstance(answer, dict) or any(answer.get(k) != v for k, v in
                (("protocol", PROTOCOL), ("session", self.session), ("seq", self.sequence), ("rev", revision))):
            # Rejected packets do not advance the device's sequence. Preserve its
            # useful refusal, but never treat a stale success as acknowledgment.
            status = answer.get("status", "") if isinstance(answer, dict) else ""
            if status.startswith("Error: insufficient") or status.startswith("Error: invalid") or status.startswith("Error: incomplete"):
                raise Refused(status)
            raise DeliveryError(status or "No matching screen acknowledgment")
        if answer.get("status") not in ("Synced", "Loading tiles"):
            raise DeliveryError(answer.get("status") or "Screen requested synchronization")
        return answer

    async def synchronize(self, inbox, record, region, values, bars, alive=lambda: True):
        """values and bars are resolved snapshots for this validated document.

        Every message is size-checked before begin. Optional histories/images
        may arrive later; tile configuration and an initial HA value may not.
        """
        revision = configuration(record, region)
        if self.failed_revision == revision:
            raise Refused(self.failure)
        pages = record["layout"]["pages"]
        try:
            begin, initial_tiles, initial_bars, live_values = prepare(inbox, record, region, values, bars)
        except Refused as error:
            # No begin was sent, so an already applied document remains valid.
            # Resolved live values may shrink again without a layout edit.
            self.failure, self.phase = str(error), "refused"
            raise
        async with self.lock:
            def current():
                if not alive(): raise Superseded("A newer saved layout is waiting")
            try:
                current()
                replace = self.confirmed != revision or not self.session
                if replace:
                    self.phase = "applying"
                    if await self._hello() != PROTOCOL:
                        raise Refused("Update screen to use the new titlebar and layout")
                    if any(message["o"].get("size", "single") not in self.tile_sizes for message in initial_tiles):
                        raise Refused("Update screen to use taller tiles")
                    current()
                    answer = await self._packet(begin, revision)
                    self.revision = revision
                    current()
                    if not answer.get("applied"):
                        for message in [*initial_bars, *initial_tiles]:
                            await self._packet(message, revision)
                            current()
                        answer = await self._packet({"op": "commit"}, revision)
                        current()
                    if not answer.get("applied"):
                        raise DeliveryError("The complete page configuration was not activated")
                    # A resumed active revision still needs the latest values.
                    self.values, self.bars = [], []
                for i, message in enumerate(live_values):
                    if i >= len(self.values) or message != self.values[i]:
                        await self._packet(message, revision)
                        current()
                for i, (page, items) in enumerate(zip(pages, bars)):
                    if i >= len(self.bars) or items != self.bars[i]:
                        await self._packet(page_message(page, i, items, initial=False), revision)
                        current()
                self.values, self.bars = deepcopy(live_values), deepcopy(bars)
                self.confirmed, self.phase = revision, "applied"
                self.failed_revision = self.failure = None
                return revision
            except Refused as error:
                self.failed_revision, self.failure = revision, str(error)
                self.session = self.confirmed = None
                self.phase = "refused"
                raise
            except (Exception, asyncio.CancelledError):
                self.session = self.confirmed = None
                self.phase = "waiting"
                raise

    async def ping(self):
        async with self.lock:
            if not self.confirmed: return False
            try:
                answer = await self._packet({"op": "ping"}, self.confirmed)
                if not answer.get("applied"): raise DeliveryError("Screen needs synchronization")
                return True
            except (Exception, asyncio.CancelledError):
                self.disconnected()
                raise

    async def auxiliary(self, message, *, session, revision):
        """Drop a response if the request's view/configuration was superseded."""
        async with self.lock:
            if self.session != session or self.confirmed != revision: return False
            message = {k: v for k, v in message.items() if k not in ("v", "session", "seq", "rev")}
            await self._packet(message, revision)
            return True
