"""HA Ingress app. HA writes: text.set_value on discovered inboxes (or the screen_message action on firmware 0.2.33+), each screen's alert actions when an alert event for every screen fires, and one persistent notification when a nightly update stops."""
import asyncio
import contextlib
import hashlib
import json
import logging
import os
from pathlib import Path
import secrets
import time
from datetime import datetime, timedelta, timezone
import math
import claude_skill
from firmware import Firmware
import tile_icons
from updates import Updater

from aiohttp import ClientError, ClientSession, ClientTimeout, WSMsgType, web
from core import BROADCAST_EVENTS, BUILTIN, SETTINGS_BESIDE_BLOCK, TILE_EVENTS, TILE_RESULT_EVENT, apply_tile_event, layout_snapshot, match_screen, HEADER_MIN_FIRMWARE, NAME_TILE_SETTINGS, TILE_BACKGROUNDS, TRANSPORT_MIN_FIRMWARE, alert_data, alert_reference, alert_service, alert_targets, controls_catalogue, device_prefixes, discover, discover_screens, encode, extras, forecast_kinds, header_items, inbox_prefix, message_action, min_firmware, pack_slots, packets, revision, screen_items, state_message, validate_header, validate_layout, validate_settings
from core import SETTING_ENTITIES, SETTING_RULES, setting_action, setting_entities, setting_from_state
import header_bar
import history_card
from zoneinfo import ZoneInfo

LOG = logging.getLogger('screen_manager')

REGISTRY_EVENTS = ('entity_registry_updated', 'device_registry_updated', 'area_registry_updated')
# Keepalive cadence. Firmware 0.2.33+ gets a small ping carrying the layout revision; every
# layout message declares this number, so the firmware sizes its feed watchdog from it
# (firmware 0.2.22+). Firmware before 0.2.33 still gets the whole layout every round.
KEEPALIVE_SECONDS = 120
# Safety net for screens that answer every ping: everything again once an hour.
FULL_REPEAT_SECONDS = 3600
# Sensor history is refreshed in a background task, never inside the sync loop.
HISTORY_SECONDS = 300
# Inbox states after which a screen needs the whole layout again: a restart, a ping that
# did not match, or a tile state that never arrived. Guarded so a slow batch cannot loop.
# Firmware built before the English translation still reports the Dutch originals, so both
# forms are recognised until every board has been reflashed.
RESEND_STATES = frozenset({
    'Ready for tile configuration', 'Resend needed', 'Loading tiles',
    'Klaar voor tegelconfiguratie', 'Indeling opnieuw nodig', 'Tegels laden',
})
RESEND_GUARD_SECONDS = 120
FORECAST_SECONDS = 1800
# A screen that answers its ping (firmware 0.2.49+) is asked with this timeout, so a busy screen never holds
# up the others. An answer that the screen lacks something repeats everything, after this many seconds right
# after a full send, doubling up to RESEND_GUARD_SECONDS while it keeps failing.
ANSWER_TIMEOUT_SECONDS = 5
ANSWER_RETRY_SECONDS = 30
SERVICE_EVENTS = ('service_registered', 'service_removed')

def samples(events, begin, span):
    """24 values, one per bucket: the last known value at the end of each bucket (None until the first)."""
    events.sort(key=lambda pair: pair[0])
    position, value, out = 0, None, []
    for bucket in range(24):
        boundary = begin + (bucket + 1) * span / 24
        while position < len(events) and events[position][0] <= boundary:
            value = events[position][1]
            position += 1
        out.append(value)
    return out

class Refused(ConnectionError):
    """Home Assistant answered a command with an error; `detail` is its own message."""
    def __init__(self, detail=''):
        super().__init__('Home Assistant refused the command.')
        self.detail = detail


def rounded(value):
    try:
        value = float(value)
    except (TypeError, ValueError):
        return None
    return round(value, 3) if math.isfinite(value) else None

class HomeAssistant:
    registry_interval = 600

    def __init__(self, session, base, token):
        self.session, self.base, self.token = session, base.rstrip('/'), token
        self.ws = None
        self.next_id = 0
        self.pending = {}
        self.states = {}
        self.registry, self.devices, self.areas = [], [], []
        self.online = False
        self.changed = asyncio.Event()
        # Entity ids the manager cares about; None wakes it for every state change.
        self.relevant = None
        # Entity ids whose state changed since the manager last looked; it only rebuilds those tiles.
        self.dirty = set()
        self.registry_changed = asyncio.Event()
        self.setting_events = []
        self.time_zone = None
        # Home Assistant's unit system; a climate entity's temperature carries no unit of its own.
        self.units = {}
        # (event type, data) of alert events for every screen, for Manager.alert_loop.
        self.broadcasts = asyncio.Queue()
        # (event type, data) of tile events (app 0.2.51+), for Manager.tile_loop.
        self.tile_events = asyncio.Queue()
        # ESPHome actions that can answer (`esphome.<node>_screen_message` of firmware 0.2.49+), from Home
        # Assistant's own list; refreshed when a device registers its actions again.
        self.responses = set()
        self.services_changed = asyncio.Event()
        # History a detail card asks for when it opens (firmware 0.2.51+): {inbox, entity, hours}, for Manager.card_history_loop.
        self.history_requests = asyncio.Queue()

    async def request(self, kind, **data):
        if self.ws is None or self.ws.closed:
            raise ConnectionError("Home Assistant isn't connected.")
        self.next_id += 1
        key = self.next_id
        future = asyncio.get_running_loop().create_future()
        self.pending[key] = future
        try:
            await self.ws.send_json({'id': key, 'type': kind, **data})
            return await asyncio.wait_for(future, 15)
        finally:
            self.pending.pop(key, None)

    async def read(self):
        async for msg in self.ws:
            if msg.type != WSMsgType.TEXT:
                continue
            data = msg.json()
            if data.get('type') == 'result':
                future = self.pending.get(data.get('id'))
                if future and not future.done():
                    if data.get('success'):
                        future.set_result(data.get('result'))
                    else:
                        error = data.get('error') if isinstance(data.get('error'), dict) else {}
                        future.set_exception(Refused(str(error.get('message') or '')))
            elif data.get('type') == 'event':
                event = data.get('event', {})
                body = event.get('data', {})
                if event.get('event_type') == 'esphome.screen_setting':
                    self.setting_events.append(body)
                    self.changed.set()
                elif event.get('event_type') == 'esphome.screen_history':
                    self.history_requests.put_nowait(body)
                elif event.get('event_type') == 'state_changed':
                    eid = body.get('entity_id')
                    if body.get('new_state'):
                        self.states[eid] = body['new_state']
                    else:
                        self.states.pop(eid, None)
                    if self.relevant is None or eid in self.relevant:
                        self.dirty.add(eid)
                        self.changed.set()
                elif event.get('event_type') in REGISTRY_EVENTS:
                    self.registry_changed.set()
                elif event.get('event_type') in SERVICE_EVENTS:
                    if body.get('domain') == 'esphome':
                        self.services_changed.set()
                elif event.get('event_type') in BROADCAST_EVENTS:
                    # Queued, not handled here: the calls wait for results this reader has to deliver.
                    self.broadcasts.put_nowait((event['event_type'], body))
                elif event.get('event_type') in TILE_EVENTS:
                    self.tile_events.put_nowait((event['event_type'], body))
        raise ConnectionError('Home Assistant connection lost.')

    def describe_close(self, connected):
        """Why and after how long the websocket ended; helps explain unexpected reconnects in the log."""
        if connected is None:
            return ''
        ws = self.ws
        reason = ws.exception() if ws is not None else None
        return ' after %d s, close code %s%s' % (time.monotonic() - connected, ws.close_code if ws is not None else None,
                                            f', {type(reason).__name__}' if reason else '')

    async def registries(self):
        self.registry, self.devices, self.areas = await asyncio.gather(
            self.request('config/entity_registry/list'), self.request('config/device_registry/list'), self.request('config/area_registry/list'))

    async def fetch_services(self):
        """Which ESPHome actions answer: Home Assistant lists `response` for an action that can return one."""
        esphome = (await self.request('get_services') or {}).get('esphome', {})
        self.responses = {f'esphome.{name}' for name, spec in esphome.items() if isinstance(spec, dict) and spec.get('response')}

    async def refresh_services(self):
        """fetch_services for the connection loop: a list that cannot be read only means no answers are asked for."""
        try:
            await self.fetch_services()
        except (ConnectionError, TimeoutError, OSError, ValueError, TypeError, AttributeError) as error:
            LOG.warning('Reading the actions from Home Assistant failed (%s); screens are not asked for answers', type(error).__name__)
            self.responses = set()

    async def run(self):
        url = ('ws://supervisor/core/websocket' if self.base == 'http://supervisor/core/api'
               else self.base.replace('http://', 'ws://').replace('https://', 'wss://') + '/websocket')
        while True:
            reader, connected = None, None
            try:
                async with self.session.ws_connect(url, heartbeat=30, max_msg_size=16*1024*1024) as ws:
                    self.ws = ws
                    await ws.receive_json(timeout=15)
                    await ws.send_json({'type': 'auth', 'access_token': self.token})
                    if (await ws.receive_json(timeout=15)).get('type') != 'auth_ok':
                        raise ConnectionError('Home Assistant authentication failed.')
                    reader = asyncio.create_task(self.read())
                    await self.request('subscribe_events', event_type='state_changed')
                    await self.request('subscribe_events', event_type='esphome.screen_setting')
                    await self.request('subscribe_events', event_type='esphome.screen_history')
                    for event_type in (*REGISTRY_EVENTS, *BROADCAST_EVENTS, *TILE_EVENTS, *SERVICE_EVENTS):
                        await self.request('subscribe_events', event_type=event_type)
                    self.states = {s['entity_id']: s for s in await self.request('get_states')}
                    await self.registries()
                    await self.refresh_services()
                    try:
                        config = await self.request('get_config')
                        self.units = config.get('unit_system') or {}
                        self.time_zone = ZoneInfo(config.get('time_zone') or 'UTC')
                    except Exception:
                        self.time_zone = timezone.utc
                    self.online = True
                    self.changed.set()
                    connected = time.monotonic()
                    LOG.info('Home Assistant connected')
                    # Refresh the registry when HA reports a change (debounced), with a slow
                    # fallback; a full fetch is about 1 MB of JSON and used to run every 30 s.
                    fetched = time.monotonic()
                    while True:
                        waiters = [asyncio.ensure_future(self.registry_changed.wait()), asyncio.ensure_future(self.services_changed.wait())]
                        try:
                            done, _ = await asyncio.wait([reader, *waiters], timeout=30, return_when=asyncio.FIRST_COMPLETED)
                        finally:
                            for waiter in waiters:
                                waiter.cancel()
                        if reader in done:
                            await reader
                        if self.services_changed.is_set():
                            # A screen that reconnects registers its actions one after the other.
                            await asyncio.sleep(1)
                            self.services_changed.clear()
                            await self.refresh_services()
                            self.changed.set()
                        if self.registry_changed.is_set():
                            await asyncio.sleep(1)
                            self.registry_changed.clear()
                        elif time.monotonic() - fetched < self.registry_interval:
                            continue
                        await self.registries()
                        fetched = time.monotonic()
                        self.changed.set()
            except (ConnectionError, TimeoutError, OSError, ValueError) as error:
                LOG.warning('Home Assistant temporarily unavailable (%s)%s', type(error).__name__, self.describe_close(connected))
            except Exception as error:
                LOG.warning('Restarting connection (%s)%s', type(error).__name__, self.describe_close(connected))
            finally:
                if self.online:
                    LOG.info('Home Assistant connection closed%s', self.describe_close(connected))
                self.online = False
                self.ws = None
                if reader:
                    reader.cancel()
                    with contextlib.suppress(asyncio.CancelledError, Exception):
                        await reader
                for future in self.pending.values():
                    if not future.done():
                        future.set_exception(ConnectionError('Connection lost.'))
            await asyncio.sleep(5)

    async def send(self, inbox, message, action=None, respond=False):
        """One message to a screen: as one ESPHome action call (firmware 0.2.33+), else as text chunks.

        `respond`: wait for the screen's answer (firmware 0.2.49+), at most ANSWER_TIMEOUT_SECONDS, and return it
        (`status`, `rev`), or None when the answer carries nothing usable."""
        if action:
            domain, service = action.split('.', 1)
            data = {'domain': domain, 'service': service, 'service_data': {'message': encode(message)}}
            if not respond:
                await self.request('call_service', **data)
                return None
            result = await asyncio.wait_for(self.request('call_service', **data, return_response=True), ANSWER_TIMEOUT_SECONDS)
            response = (result or {}).get('response') if isinstance(result, dict) else None
            return response if isinstance(response, dict) else None
        for packet in packets(message):
            await self.request('call_service', domain='text', service='set_value',
                               service_data={'entity_id': inbox, 'value': packet})

    async def call(self, action, data):
        """One Home Assistant action, such as a screen's esphome.<node>_show_alert."""
        domain, service = action.split('.', 1)
        await self.request('call_service', domain=domain, service=service, service_data=data)

    async def fire(self, event_type, data):
        """One Home Assistant event of our own, such as the answer to a tile event."""
        await self.request('fire_event', event_type=event_type, event_data=data)

    async def set_state(self, entity_id, state, attributes):
        """A state this app publishes itself (the layout sensors). Home Assistant's websocket has no
        command for that, so this goes over the REST API with the same token."""
        async with self.session.post(f'{self.base}/states/{entity_id}',
                                     headers={'Authorization': 'Bearer ' + self.token},
                                     json={'state': state, 'attributes': attributes}) as response:
            response.raise_for_status()

    async def forecast(self, entity, kind='daily'):
        # Forecasts left the weather attributes in HA 2024.4; ask the service instead.
        result = await self.request('call_service', domain='weather', service='get_forecasts',
                                    service_data={'type': kind}, target={'entity_id': entity}, return_response=True)
        forecast = ((result or {}).get('response') or {}).get(entity, {}).get('forecast', [])
        return forecast if isinstance(forecast, list) else []

    async def statistics(self, entities, hours):
        """24 samples per entity from the recorder's statistics, all entities in one request.

        Hourly means for a day, five-minute means below that; a sum-only sensor (energy) gives its
        state. Entities without statistics (no state class) are absent and fall back to `history`."""
        period = 'hour' if hours >= 24 else '5minute'
        start = datetime.now(timezone.utc) - timedelta(hours=hours)
        result = await self.request('recorder/statistics_during_period', start_time=start.isoformat(),
                                    statistic_ids=sorted(entities), period=period, types=['mean', 'state'])
        begin, span, found = start.timestamp(), hours * 3600, {}
        for entity, rows in (result or {}).items():
            if entity not in entities or not isinstance(rows, list):
                continue
            events = []
            for row in rows:
                moment = row.get('start') if isinstance(row, dict) else None
                if not isinstance(moment, (int, float)):
                    continue
                if moment > 1e11:  # milliseconds since HA 2023.9
                    moment /= 1000
                events.append((moment, rounded(row.get('mean') if row.get('mean') is not None else row.get('state'))))
            found[entity] = samples(events, begin, span)
        return found

    async def state_changes(self, entity, hours):
        """(unix time, state) of every change in the last `hours`, beginning with the state at the start (REST history)."""
        start = (datetime.now(timezone.utc)-timedelta(hours=hours)).isoformat()
        async with self.session.get(self.base+'/history/period/'+start,
                params={'filter_entity_id':entity,'minimal_response':'','no_attributes':''},
                headers={'Authorization':'Bearer '+self.token}) as response:
            response.raise_for_status()
            raw = bytearray()
            async for chunk in response.content.iter_chunked(65536):
                raw.extend(chunk)
                if len(raw)>2*1024*1024: raise ValueError('History too large.')
            rows=json.loads(raw)
        changes=[]
        for row in rows[0] if rows else []:
            try:
                timestamp=datetime.fromisoformat(row.get('last_changed',row.get('last_updated','')).replace('Z','+00:00')).timestamp()
            except (ValueError,TypeError,AttributeError): continue
            changes.append((timestamp,row.get('state')))
        return changes

    async def history(self, entity, hours):
        begin=(datetime.now(timezone.utc)-timedelta(hours=hours)).timestamp()
        return samples([(timestamp,rounded(state)) for timestamp,state in await self.state_changes(entity,hours)], begin, hours*3600)

    async def statistic_rows(self, entity, hours):
        """The recorder's hourly statistics rows of one entity over the last `hours` (mean, min, max, state), from the
        hour the range begins in; empty for an entity without statistics."""
        start = datetime.now(timezone.utc) - timedelta(hours=hours + 1)
        result = await self.request('recorder/statistics_during_period', start_time=start.isoformat(), statistic_ids=[entity],
                                    period='hour', types=['mean', 'min', 'max', 'state'])
        rows = (result or {}).get(entity) if isinstance(result, dict) else None
        return rows if isinstance(rows, list) else []

class Manager:
    def __init__(self, ha, path):
        self.ha, self.path = ha, Path(path)
        # sent: per inbox what the screen holds ({'layout', 'header', 'states', 'rev'}); last: the
        # last full send; pinged: the last keepalive ping.
        self.layouts, self.sent, self.status, self.last, self.pinged = {}, {}, {}, {}, {}
        # (entity, hours) -> (monotonic, 24 samples); filled by history_loop, read by sync_one.
        self.histories = {}
        self.history_wake = asyncio.Event()
        self.forecasts = {}
        self.listeners = set()  # asyncio.Event per open /api/events stream
        self.firmware = Firmware(os.environ.get("ESPHOME_CONFIG", "/homeassistant/esphome"), self.path.parent)
        self.updates = Updater(self, self.path.parent / 'updates.json')
        self.skill_dir = claude_skill.skill_dir()
        self._registry_source, self._registry_index = None, {}
        self._items_source, self._items = None, []
        self._screens_key, self._screens = None, []
        self._watched_key, self._watched = None, set()
        self._seen_registry = None
        # Inbox entity ids seen this run with their Home Assistant device, and old inbox ids that a screen
        # now reports under a new id (see follow_renamed_inboxes); the updater follows a screen through them.
        self._inbox_devices, self.aliases = {}, {}
        # The layout snapshot each screen's sensor already carries, so it is only written when it changes.
        self.published = {}
        self._prefix_source, self._prefixes = None, {}
        # Screens that own their settings (firmware 0.2.49+): {device_id: {key: entity_id}}, per registry.
        self._settings_source, self._settings_index, self._settings_key = None, {}, None
        # Answers (firmware 0.2.49+): when to send everything again after one said the screen lacks something,
        # how often that failed in a row, and actions Home Assistant refused to answer for.
        self.retry_at, self.retries, self.no_answers = {}, {}, set()
        # History for detail cards (firmware 0.2.51+): (entity, hours) -> (monotonic, message), and the fetches under way.
        self.card_histories, self.card_history_fetches = {}, {}
        if self.path.exists():
            raw = json.loads(self.path.read_text())
            # Versioned persistent data. Never silently overwrite an unknown schema.
            if raw.get('version') != 1 or not isinstance(raw.get('screens'), dict):
                raise ValueError('Unknown storage version; data stays unchanged.')
            self.layouts = {key: validate_layout(value) for key, value in raw['screens'].items()}

    def inventory(self):
        """(screens, every tile/top-bar entity): the full walk over the registry, for the editor and saving."""
        return discover(self.ha.registry, self.ha.states, self.ha.devices, self.ha.areas)

    def screen_registry(self):
        """The few registry entries that describe screens; rebuilt only when HA delivers a new registry."""
        registry = getattr(self.ha, 'registry', [])
        if self._items_source is not registry:
            self._items_source, self._items = registry, screen_items(registry)
        return self._items

    def screens(self):
        """The paired screens, cached until the registry or one of the screens' own diagnostics changes.

        Costs a handful of dictionary lookups per screen instead of a walk over every entity in
        Home Assistant, so the sync loop can run it on every wake."""
        ha = self.ha
        items = self.screen_registry()
        key = (id(ha.registry), id(ha.devices), id(ha.areas), tuple(ha.states.get(item['entity_id'], {}).get('state') for item in items))
        if key != self._screens_key:
            self._screens_key, self._screens = key, discover_screens(items, ha.states, ha.devices, ha.areas)
            self.follow_renamed_inboxes(items)
        return [dict(screen) for screen in self._screens]

    def screen(self, inbox):
        screens = self.screens()
        inbox = self.aliases.get(inbox, inbox)
        return next((s for s in screens if s['id'] == inbox), None)

    def device_prefixes(self, device):
        registry = getattr(self.ha, 'registry', [])
        if self._prefix_source is not registry:
            self._prefix_source, self._prefixes = registry, {}
        if device not in self._prefixes:
            self._prefixes[device] = device_prefixes(registry, device)
        return self._prefixes[device]

    def follow_renamed_inboxes(self, items):
        """Keep a screen's layout and update history when its inbox entity gets a new entity id.

        Firmware 0.2.34 renamed the inbox from "Tegelinstellingen" to "Tile settings". Home Assistant then
        removes the old entity and registers the renamed one under a new id, which would orphan the layout
        stored under the old id. The screen is recognised by its device: an inbox this run saw on the same
        device, or (after a restart) a stored inbox id that carries one of the device's entity id prefixes."""
        current = {item['entity_id']: item.get('device_id') for item in items
                   if item.get('platform') == 'esphome' and item['entity_id'].startswith('text.') and item.get('device_id')
                   and item.get('original_name') in NAME_TILE_SETTINGS and not item.get('disabled_by')}
        stored = (set(self.layouts) | set(self.updates.hosts) | set(self.updates.results)) - set(current) - set(self.aliases)
        for new, device in current.items():
            olds = {old for old, seen in self._inbox_devices.items() if seen == device and old not in current}
            candidates = [old for old in stored if inbox_prefix(old) is not None]
            if candidates:
                prefixes = self.device_prefixes(device)
                olds |= {old for old in candidates if inbox_prefix(old) in prefixes}
            for old in sorted(olds - set(self.aliases)):
                self.rename_inbox(old, new)
        self._inbox_devices.update(current)

    def write_layouts(self, layouts):
        self.path.parent.mkdir(parents=True, exist_ok=True)
        temp = self.path.with_suffix('.tmp')
        with open(temp, 'w', encoding='utf8') as handle:
            os.chmod(temp, 0o600)
            json.dump({'version': 1, 'screens': layouts}, handle, ensure_ascii=False)
            handle.flush()
            os.fsync(handle.fileno())
        temp.replace(self.path)

    def rename_inbox(self, old, new):
        """Move everything kept under an old inbox id to the id the same screen reports under now."""
        moved = []
        if old in self.layouts and new not in self.layouts:
            layouts = {(new if key == old else key): value for key, value in self.layouts.items()}
            self.write_layouts(layouts)
            self.layouts = layouts
            moved.append('layout')
        elif old in self.layouts:
            LOG.warning('Screen %s already has a layout; the one stored under %s stays untouched', new, old)
        if self.updates.renamed(old, new):
            moved.append('update history')
        for store in (self.sent, self.status, self.last, self.pinged):
            store.pop(old, None)
        self.aliases = {**{key: (new if value == old else value) for key, value in self.aliases.items()}, old: new}
        self._inbox_devices.pop(old, None)
        LOG.info('Screen %s now reports as %s%s', old, new, f"; moved its {' and '.join(moved)}" if moved else '')
        if moved:
            self.history_wake.set()
            self.ha.changed.set()
            self.notify()

    def firmware_version(self, inbox, screen=None):
        if screen is None:
            screen = self.screen(inbox) or {}
        try:
            version=tuple(int(part) for part in screen.get('firmware','').split('.'))
            return version if len(version)==3 else None
        except (ValueError, TypeError):
            return None

    def supports_twenty(self, inbox):
        version=self.firmware_version(inbox)
        return bool(version) and version >= (0,2,7)

    def supports_header(self, inbox, screen=None):
        return (self.firmware_version(inbox, screen) or (0, 0, 0)) >= HEADER_MIN_FIRMWARE

    def supports_ping(self, inbox, screen=None):
        """Firmware 0.2.33+ answers a keepalive ping with its layout revision."""
        return (self.firmware_version(inbox, screen) or (0, 0, 0)) >= TRANSPORT_MIN_FIRMWARE

    def transport(self, inbox, screen=None):
        """The ESPHome action that takes a whole message (firmware 0.2.33+), or None for the text inbox."""
        if screen is None:
            screen = self.screen(inbox) or {}
        return message_action(screen.get('node')) if self.supports_ping(inbox, screen) else None

    # ----- Screen settings: owned by the screen (firmware 0.2.49+), else stored with the layout -----
    def setting_index(self):
        """{device_id: {key: entity_id}} for every screen that owns its settings; rebuilt per registry."""
        registry = getattr(self.ha, 'registry', [])
        if self._settings_source is not registry:
            by_device = {}
            for item in registry:
                if item.get('platform') == 'esphome' and item.get('device_id'):
                    by_device.setdefault(item['device_id'], []).append(item)
            index = {}
            for device, items in by_device.items():
                entities = setting_entities(items)
                if entities is not None:
                    index[device] = entities
            self._settings_source, self._settings_index = registry, index
        return self._settings_index

    def setting_entities(self, screen):
        """{key: entity_id} when this screen owns its settings, else None (they travel in the layout message)."""
        device = (screen or {}).get('device_id')
        return self.setting_index().get(device) if device else None

    def settings_view(self, screen):
        """What the editor shows under Screen settings.

        `owner` is 'screen' when the screen's own entities hold the settings, else 'layout'. `keys` are the
        settings this screen has, `unavailable` the ones Home Assistant cannot read or change right now (the
        screen is offline, or the entity is disabled); their value is None, not a default that could differ
        from what the screen has."""
        inbox = self.aliases.get(screen['id'], screen['id'])
        keys = [key for key in SETTING_RULES if key != 'show_clock' and (key != 'rotation' or screen.get('board') == 'guition')]
        entities = self.setting_entities(screen)
        if entities is None:
            try:
                values = validate_settings(self.layouts.get(inbox, {}).get('settings', {}))
            except ValueError:
                values = validate_settings({})
            if (self.firmware_version(inbox, screen) or (0, 0, 0)) < (0, 2, 44):
                keys = [key for key in keys if key not in ('auto_home', 'auto_home_seconds')]
            return {'owner': 'layout', 'values': values, 'keys': keys, 'unavailable': []}
        # Only the settings this screen has an entity for: one added in later firmware stays out of the panel.
        keys = [key for key in keys if key in entities]
        values = {key: setting_from_state(key, self.ha.states.get(entities[key])) for key in keys}
        return {'owner': 'screen', 'values': values, 'keys': keys, 'unavailable': [key for key in keys if values[key] is None]}

    def settings_states_key(self):
        """The states of every setting entity, so the sync loop notices a change the editor should show."""
        states = self.ha.states
        return tuple((entity, states.get(entity, {}).get('state')) for device in self.setting_index().values()
                     for entity in device.values())

    def store_settings(self, inbox, settings, screen=None, on_screen=False):
        """Settings of a screen that does not own them, kept with its layout.

        `on_screen`: the screen reported them itself. The layout message it holds is then brought in step
        without sending it back, which could undo a change it made after reporting this one; its revision
        stays, so the next ping still matches."""
        base = self.layouts.get(inbox) or {'title': (screen or {}).get('name') or 'Home', 'tiles': []}
        layout = validate_layout({**base, 'settings': settings})
        updated = {**self.layouts, inbox: layout}
        self.write_layouts(updated)
        self.layouts = updated
        if on_screen and inbox in self.sent:
            self.sent[inbox] = {**self.sent[inbox], 'layout': self.layout_message(inbox, layout, screen or self.screen(inbox) or {})}
        self.notify()

    def screen_setting_event(self, event):
        """A screen reported a setting that changed on it (its settings page, one of its entities).

        A screen that owns its settings needs nothing here: its entities carry the change. Older firmware gets
        it kept with the layout, without the layout being sent back."""
        inbox = self.aliases.get(event.get('inbox'), event.get('inbox'))
        if inbox not in self.layouts:
            return
        screen = self.screen(inbox)
        if screen is not None and self.setting_entities(screen) is not None:
            return
        key, value = event.get('key'), event.get('value')
        stored = validate_settings(self.layouts[inbox].get('settings', {}))
        if key not in stored or key == 'show_clock':
            return
        settings = dict(stored)
        settings[key] = value == '1' if type(stored[key]) is bool else int(value)
        if key == 'brightness':
            for dim in ('standby_brightness', 'night_brightness'):
                settings[dim] = min(settings[dim], settings[key])
        settings = validate_settings(settings)
        # A screen reporting what it already has (an automation setting the same value on every light change).
        if settings != stored:
            self.store_settings(inbox, settings, screen, on_screen=True)

    async def change_settings(self, inbox, changes):
        """Settings from the editor: on the screen itself when it owns them, else kept with its layout, which the
        sync loop then sends. Returns the settings as the editor shows them afterwards."""
        inbox = self.aliases.get(inbox, inbox)
        screen = self.screen(inbox)
        if screen is None:
            raise ValueError("This isn't a paired ESP screen. Refresh the overview.")
        view = self.settings_view(screen)
        if not isinstance(changes, dict) or not changes or set(changes) - set(view['keys']):
            raise ValueError('Unknown screen settings; refresh the management page.')
        if view['owner'] == 'screen' and not screen.get('online'):
            raise ValueError('This screen is offline. You can change its settings once it is back.')
        missing = [SETTING_ENTITIES[key][1] for key in changes if key in view['unavailable']]
        if missing:
            raise ValueError(f"{', '.join(missing)} can't be changed now: the entity is off in Home Assistant, or the screen is restarting.")
        # A setting Home Assistant cannot read (its entity is off) is checked at its default.
        wanted = {**{key: value for key, value in view['values'].items() if value is not None}, **changes}
        if 'brightness' in changes and type(changes['brightness']) is int:
            # A lower brightness pulls both dim levels down with it, as on the screen.
            for dim in ('standby_brightness', 'night_brightness'):
                if dim not in changes:
                    wanted[dim] = min(wanted.get(dim, SETTING_RULES[dim][0]), changes['brightness'])
        merged = validate_settings(wanted)
        if merged['rotation'] and screen.get('board') != 'guition':
            raise ValueError('Rotation requires a Guition with firmware 0.2.9 or newer.')
        if view['owner'] == 'layout':
            self.store_settings(inbox, merged, screen)
            self.ha.changed.set()
            return self.settings_view(screen)
        entities = self.setting_entities(screen)
        # Brightness first: the screen keeps both dim levels at or below it.
        for key in sorted(changes, key=lambda key: (key != 'brightness', list(SETTING_RULES).index(key))):
            if merged[key] != view['values'][key]:
                await self.ha.call(*setting_action(key, entities[key], merged[key]))
        return self.settings_view(screen)

    # ----- Answers from the screen (firmware 0.2.49+) -----
    def answers(self, inbox, screen):
        """True when the screen's message action can answer, so a ping learns at once what the screen holds."""
        action = self.transport(inbox, screen)
        return bool(action) and action in getattr(self.ha, 'responses', ()) and action not in self.no_answers

    def answered(self, inbox, screen, answer):
        """Act on a screen's answer to a ping: everything again when it lacks the layout or a tile, or refused one."""
        status = answer.get('status') if isinstance(answer, dict) else None
        if not isinstance(status, str):
            return
        if status not in RESEND_STATES and not status.startswith('Error'):
            self.retries.pop(inbox, None)
            return
        failures = self.retries.get(inbox, 0)
        due = self.last.get(inbox, 0) + min(RESEND_GUARD_SECONDS, ANSWER_RETRY_SECONDS * 2 ** failures)
        name = (screen or {}).get('name') or inbox
        if time.monotonic() >= due:
            LOG.info('%s answered "%s"; sending everything again', name, status)
            self.retries[inbox] = failures + 1
            self.retry_at.pop(inbox, None)
            self.sent.pop(inbox, None)
            self.ha.changed.set()
        elif inbox not in self.retry_at:
            LOG.info('%s answered "%s"; sending everything again in %d s', name, status, max(1, round(due - time.monotonic())))
            self.retries[inbox] = failures + 1
            self.retry_at[inbox] = due

    # ----- History on a detail card (firmware 0.2.51+) -----
    async def card_history_loop(self):
        """Answer the history a screen asks for when a card opens, a few at a time."""
        limit = asyncio.Semaphore(3)

        async def answer(request):
            async with limit:
                try:
                    await self.answer_history(request)
                except (ClientError, ConnectionError, TimeoutError, OSError, ValueError) as error:
                    # Nothing is sent or kept: the card says it has no history and asks again.
                    LOG.info('No history for %s (%s)', request.get('entity'), type(error).__name__)
                except Exception as error:
                    LOG.warning('History for %s was not sent (%s)', request.get('entity') if isinstance(request, dict) else '?', type(error).__name__)
        while True:
            request = await self.ha.history_requests.get()
            asyncio.ensure_future(answer(request))

    async def answer_history(self, request):
        """One screen's request: an entity on its own layout, a range the card offers, a screen that takes whole messages."""
        if not isinstance(request, dict):
            return
        inbox = self.aliases.get(request.get('inbox'), request.get('inbox'))
        entity = request.get('entity')
        try:
            hours = int(request.get('hours'))
        except (TypeError, ValueError):
            return
        layout, screen = self.layouts.get(inbox), self.screen(inbox) if isinstance(inbox, str) else None
        if hours not in history_card.RANGES or not layout or not screen or not screen.get('online'):
            return
        if entity not in {tile['entity'] for tile in layout['tiles']}:
            return
        what = history_card.kind(entity, self.ha.states.get(entity))
        action = self.transport(inbox, screen)
        if what is None or not action:
            return
        await self.ha.send(inbox, await self.card_history(entity, hours, what), action)

    async def card_history(self, entity, hours, what):
        """The history message, from a short cache; screens asking at the same time share one fetch."""
        key = (entity, hours)
        cached = self.card_histories.get(key)
        if cached and time.monotonic() - cached[0] < history_card.CACHE_SECONDS[hours]:
            return cached[1]
        fetch = self.card_history_fetches.get(key)
        if fetch is None:
            fetch = asyncio.ensure_future(self.build_card_history(entity, hours, what))
            self.card_history_fetches[key] = fetch
            fetch.add_done_callback(lambda _: self.card_history_fetches.pop(key, None))
        message = await asyncio.shield(fetch)
        self.card_histories[key] = (time.monotonic(), message)
        for old in [k for k, (moment, _) in self.card_histories.items() if time.monotonic() - moment > 3600]:
            del self.card_histories[old]
        return message

    async def build_card_history(self, entity, hours, what):
        """Numbers from the recorder's hourly statistics for a day or a week (the exact changes for an hour, or for an
        entity without statistics); states from their changes. A fetch that fails raises, so nothing is kept."""
        start, end = history_card.window(hours)
        tz = getattr(self.ha, 'time_zone', None)
        attrs = (self.ha.states.get(entity) or {}).get('attributes') or {}
        if what == 'timeline':
            return history_card.timeline(entity, hours, await self.ha.state_changes(entity, hours), start, end, tz, attrs)
        entry, unit = self.registry_index().get(entity), attrs.get('unit_of_measurement') or ''
        rows = await self.ha.statistic_rows(entity, hours) if hours > 1 else []
        if rows:
            means, extreme = history_card.statistic_changes(rows, 3600)
            return history_card.line(entity, hours, means, start, end, tz, entry, unit, extreme)
        changes = [(moment, header_bar.numeric(value)) for moment, value in await self.ha.state_changes(entity, hours)]
        return history_card.line(entity, hours, changes, start, end, tz, entry, unit)

    def registry_index(self):
        """Entity registry by id (display precision, entity category); rebuilt only when HA delivers a new registry."""
        registry = getattr(self.ha, 'registry', [])
        if self._registry_source is not registry:
            self._registry_source, self._registry_index = registry, {item['entity_id']: item for item in registry}
        return self._registry_index

    def device_entries(self, entity):
        """Registry entries on the device of `entity` (itself included); the index follows the registry object."""
        registry = getattr(self.ha, 'registry', [])
        if getattr(self, '_devices_source', None) is not registry:
            by_device = {}
            for item in registry:
                if item.get('device_id'):
                    by_device.setdefault(item['device_id'], []).append(item)
            self._devices_source, self._by_device = registry, by_device
        device = self.registry_index().get(entity, {}).get('device_id')
        return self._by_device.get(device, []) if device else []

    def related_entities(self, tile):
        """Entities a card reads besides its own: a vacuum's cleaning mode and water selects and its battery sensor, a
        cover's battery sensor."""
        from core import cover_related, vacuum_related
        if tile['entity'].startswith('vacuum.'):
            return tuple(vacuum_related(tile['entity'], self.device_entries(tile['entity']), self.ha.states).values())
        if tile['entity'].startswith('cover.'):
            return tuple(cover_related(tile['entity'], self.device_entries(tile['entity']), self.ha.states).values())
        return ()

    def header_message(self, layout):
        return header_bar.message(layout, self.ha.states, self.registry_index(), getattr(self.ha, 'units', {}), getattr(self.ha, 'time_zone', None))

    def needs_firmware(self, inbox, layout, screen=None):
        """Version string the screen must run first, or None when the layout can be sent."""
        needed=min_firmware(layout)
        if needed and not ((self.firmware_version(inbox, screen) or (0,0,0)) >= needed):
            return '.'.join(str(part) for part in needed)
        return None

    def save(self, inbox, data):
        screens, entities = self.inventory()
        # A page opened before a screen's inbox got a new id still saves to the right screen.
        inbox = self.aliases.get(inbox, inbox)
        screen=next((s for s in screens if s['id']==inbox), None)
        if screen is None:
            raise ValueError("This isn't a paired ESP screen. Refresh the overview.")
        layout = validate_layout(data)
        needed = self.needs_firmware(inbox, layout, screen)
        if needed:
            raise ValueError(f"Install screen firmware {needed} or newer first for these tiles.")
        # Settings change through their own call (change_settings). A page from before app 0.2.57 still sends them
        # with the tiles: a screen that owns its settings ignores them, so a stale form never undoes a change
        # made on the screen; other screens keep taking them, as before.
        if self.setting_entities(screen) is not None:
            layout.pop('settings', None)
        # A still-open older UI may save tiles without the new optional settings or top bar.
        if 'settings' not in layout and 'settings' in self.layouts.get(inbox, {}):
            layout['settings'] = self.layouts[inbox]['settings'].copy()
        if 'header' not in layout and 'header' in self.layouts.get(inbox, {}):
            layout['header'] = self.layouts[inbox]['header']
        # Firmware before the top bar only knows show_clock; it follows the clock item.
        if 'header' in layout and 'settings' in layout:
            layout['settings']['show_clock'] = any(item['type'] == 'clock' for item in layout['header']['items'])
        if 'settings' in layout and 'swipe_pages' not in data.get('settings',{}):
            layout['settings']['swipe_pages']=self.layouts.get(inbox,{}).get('settings',{}).get('swipe_pages',False)
        if 'settings' in layout and 'rotation' not in data.get('settings',{}):
            layout['settings']['rotation']=self.layouts.get(inbox,{}).get('settings',{}).get('rotation',0)
        if layout.get('settings',{}).get('rotation',0) and screen.get('board')!='guition':
            raise ValueError('Rotation requires a Guition with firmware 0.2.9 or newer.')
        old_tiles = {t['entity']:t for t in self.layouts.get(inbox,{}).get('tiles',[])}
        for tile in layout['tiles']:
            old_options=old_tiles.get(tile['entity'],{}).get('options',{})
            for key in ('background', 'icon', 'controls'):
                if 'options' in tile and key not in tile['options'] and key in old_options:
                    tile['options'][key]=old_options[key]
            if 'options' not in tile and 'options' in old_tiles.get(tile['entity'],{}):
                tile['options'] = old_tiles[tile['entity']]['options'].copy()
        # Restored options can widen a tile: an editor without positions packs again with
        # the real widths, and explicit positions are checked once more for overlap.
        if not any(isinstance(t, dict) and 'slot' in t for t in data.get('tiles', [])):
            for tile, slot in zip(layout['tiles'], pack_slots(layout['tiles'])):
                tile['slot'] = slot
        layout = validate_layout(layout)
        known = {e['id'] for e in entities} | set(BUILTIN)
        if any(t['entity'] not in known for t in layout['tiles']):
            raise ValueError('A chosen entity no longer exists. Look up the new entity.')
        if any(item['type'] == 'entity' and item['entity'] not in known and item['entity'] not in self.ha.states for item in header_items(layout)):
            raise ValueError('An entity in the top bar no longer exists. Choose a different one.')
        updated = {**self.layouts, inbox: layout}
        self.write_layouts(updated)
        self.layouts = updated
        self.sent.pop(inbox, None)
        self.status[inbox] = 'Saved; waiting for sync'
        self.history_wake.set()
        self.ha.changed.set()
        self.notify()

    def notify(self):
        for listener in self.listeners:
            listener.set()

    def pending_profiles(self, screens, profiles):
        """ESP Screens profiles without a paired screen: flashed but not yet added in Home Assistant, or not flashed yet.

        Pairing happens in Home Assistant itself, outside this page; the sidebar shows these so nobody wonders
        where the freshly flashed screen went."""
        nodes = {s.get('node') for s in screens}
        devices = {s.get('device') for s in screens}
        installed = getattr(self.firmware, 'installed', set())
        return [{'file': file, 'node': meta['node'], 'friendly': meta.get('friendly') or meta['node'] or file,
                 'installed': file in installed, 'api_key': meta.get('api_key')}
                for file, meta in profiles.items()
                if meta.get('screen') and meta.get('node') not in nodes and (meta.get('friendly') or None) not in devices]

    def watched_entities(self):
        """Entities whose state changes matter: tiles on any layout plus the screens' own diagnostics.

        Cached per (registry, layouts): both are replaced as whole objects when they change."""
        key = (id(getattr(self.ha, 'registry', [])), id(self.layouts))
        if key != self._watched_key:
            watched = {tile['entity'] for layout in self.layouts.values() for tile in layout['tiles']}
            watched |= {item['entity'] for layout in self.layouts.values() for item in header_items(layout) if item['type'] == 'entity'}
            watched |= {item['entity_id'] for item in self.screen_registry()}
            watched |= {eid for layout in self.layouts.values() for tile in layout['tiles'] for eid in self.related_entities(tile)}
            watched |= {eid for device in self.setting_index().values() for eid in device.values()}
            self._watched_key, self._watched = key, watched
        return set(self._watched)

    async def cached(self, store, key, ttl, fetch):
        entry=store.get(key)
        if not entry or time.monotonic()-entry[0]>ttl:
            try: value=await fetch()
            except Exception: value=[]
            entry=(time.monotonic(),value);store[key]=entry
        return entry[1]

    def forecast_due(self, entity):
        entry = self.forecasts.get(entity)
        hourly = self.forecasts.get((entity, 'hourly'))
        return not entry or not hourly or time.monotonic() - min(entry[0], hourly[0]) > FORECAST_SECONDS

    async def tile_message(self, index, tile):
        """The state message of one tile: state, options, extras, and the history the background task holds."""
        forecast=hourly=None
        if tile['entity'].startswith('weather.') and hasattr(self.ha,'forecast'):
            entity = tile['entity']
            # Only the forecasts the entity offers: asking Buienradar for hourly ones logged an error in Home
            # Assistant every half hour. What it lacks is kept as empty, so the cache still counts its age.
            kinds = forecast_kinds(self.ha.states.get(entity, {}).get('attributes', {}))
            async def nothing():
                return []
            forecast=await self.cached(self.forecasts, entity, FORECAST_SECONDS, (lambda: self.ha.forecast(entity)) if 'daily' in kinds else nothing)
            # Hourly forecasts feed the weather card's next-hours strip (0.2.23+); refreshed every half hour.
            hourly=await self.cached(self.forecasts, (entity,'hourly'), FORECAST_SECONDS, (lambda: self.ha.forecast(entity,'hourly')) if 'hourly' in kinds else nothing)
        # A vacuum's card also reads selects and the battery sensor of its device (app 0.2.46), a cover's card its battery (0.2.58).
        device=self.device_entries(tile['entity']) if tile['entity'].startswith(('vacuum.', 'cover.')) else None
        message=state_message(index,tile,self.ha.states,extras(tile,self.ha.states,forecast,getattr(self.ha,'time_zone',None),hourly,device=device))
        if tile['entity'].startswith('sensor.') and hasattr(self.ha,'history'):
            hours=tile.get('options',{}).get('history_hours',24)
            entry=self.histories.get((tile['entity'],hours))
            if entry is not None:
                message['history']={'hours':hours,'values':entry[1]}
            else:
                self.history_wake.set()
        return message

    def layout_message(self, inbox, layout, screen):
        tiles = layout['tiles']
        # Grid positions (firmware 0.2.26+; older firmware ignores them and packs the entities in order).
        message = {'v': 1, 'op': 'layout', 'inbox': inbox, 'title': layout['title'], 'entities': [t['entity'] for t in tiles],
                   'slots': [t['slot'] for t in tiles], 'keepalive': KEEPALIVE_SECONDS}
        if 'pages' in layout:
            message['pages'] = layout['pages']
        # A screen that owns its settings (firmware 0.2.49+) gets none: they would overwrite what changed on it.
        if 'settings' in layout and self.setting_entities(screen) is None:
            # `settings` is the fixed eleven-key block older firmware insists on; everything added
            # later travels as its own key, which firmware that predates it simply ignores.
            message['settings'] = {k:v for k,v in layout['settings'].items() if k not in SETTINGS_BESIDE_BLOCK}
            message['swipe_pages'] = layout['settings'].get('swipe_pages',False)
            message['auto_home'] = layout['settings'].get('auto_home',True)
            message['auto_home_seconds'] = layout['settings'].get('auto_home_seconds',120)
            if screen.get('board')=='guition':
                message['rotation'] = layout['settings'].get('rotation',0)
        # The revision the screen echoes on every ping (firmware 0.2.33+; older firmware ignores it).
        message['rev'] = revision(message)
        return message

    async def sync_one(self, inbox, layout, force=False, screen=None, dirty=None):
        """Send the screen what it lacks; True when something went out.

        `dirty` is the set of entity ids whose state changed since the last pass: only their tiles
        (and the top bar, when it shows one of them) are rebuilt, the other messages are reused as
        sent. None rebuilds every message and sends the differences; `force` sends everything."""
        if screen is None:
            screen = self.screen(inbox) or {}
        needed = self.needs_firmware(inbox, layout, screen)
        if needed:
            self.status[inbox]=f"Layout saved; firmware {needed}+ needed for these tiles"
            return False
        tiles = layout['tiles']
        layout_msg = self.layout_message(inbox, layout, screen)
        previous = self.sent.get(inbox)
        full = force or not previous or previous['layout'] != layout_msg or dirty is None
        # The top bar right after the layout (firmware 0.2.32+; older firmware draws the clock of show_clock).
        header_msg = None
        if self.supports_header(inbox, screen):
            bar = {item['entity'] for item in header_items(layout) if item['type'] == 'entity'}
            if full or previous['header'] is None or bar & dirty:
                header_msg = self.header_message(layout)
            else:
                header_msg = previous['header']
        states = []
        for i, tile in enumerate(tiles):
            reuse = not full and i < len(previous['states']) and tile['entity'] not in dirty and dirty.isdisjoint(self.related_entities(tile))
            if reuse and tile['entity'].startswith('weather.') and self.forecast_due(tile['entity']):
                reuse = False
            states.append(previous['states'][i] if reuse else await self.tile_message(i, tile))
        outgoing = []
        if force or not previous or layout_msg != previous['layout']:
            outgoing.append(layout_msg)
        if header_msg is not None and (force or not previous or header_msg != previous['header']):
            outgoing.append(header_msg)
        for i, message in enumerate(states):
            if force or not previous or i >= len(previous['states']) or message != previous['states'][i]:
                outgoing.append(message)
        action = self.transport(inbox, screen)
        for message in outgoing:
            # Save during transmission aborts the old batch, then starts a full new layout.
            if self.layouts.get(inbox) != layout:
                self.sent.pop(inbox, None)
                return True
            await self.ha.send(inbox, message, action)
        # `rev` is the revision of the layout message the screen holds: a layout brought in step without being
        # sent (settings the screen reported itself, see store_settings) keeps the one it had.
        sent_layout = any(message is layout_msg for message in outgoing)
        rev = layout_msg['rev'] if sent_layout or not previous else previous.get('rev', layout_msg['rev'])
        self.sent[inbox] = {'layout': layout_msg, 'header': header_msg, 'states': states, 'rev': rev}
        # The hourly timer follows a real full transmission; any message refreshes the screen's
        # feed window, so the ping timer follows whatever went out (a rebuild without differences,
        # after a registry refresh, must not postpone the ping).
        if force or not previous:
            self.last[inbox] = time.monotonic()
        if outgoing:
            self.pinged[inbox] = time.monotonic()
        self.status[inbox] = 'Sent to Home Assistant'
        # A screen that answers confirms a new layout at once: a ping after the batch says whether it holds
        # the layout and every tile, or which part is missing (firmware 0.2.49+).
        if sent_layout and self.answers(inbox, screen):
            await self.ping(inbox, screen)
        return bool(outgoing)

    async def ping(self, inbox, screen):
        """Keepalive for firmware 0.2.33+: the layout revision only; the screen asks for the rest itself. A screen
        that can answer (firmware 0.2.49+) is asked to, so a missing layout or tile is sent again right away."""
        sent = self.sent.get(inbox)
        if not sent:
            return
        message = {'v': 1, 'op': 'ping', 'rev': sent['rev'], 'keepalive': KEEPALIVE_SECONDS}
        action = self.transport(inbox, screen)
        if not self.answers(inbox, screen):
            await self.ha.send(inbox, message, action)
            self.pinged[inbox] = time.monotonic()
            return
        try:
            answer = await self.ha.send(inbox, message, action, respond=True)
        except TimeoutError:
            # Busy or gone: the text inbox and the next ping still tell.
            LOG.info('%s did not answer its ping within %d s', (screen or {}).get('name') or inbox, ANSWER_TIMEOUT_SECONDS)
            answer = None
        except Refused as error:
            if 'response' not in error.detail.lower():
                raise
            # Home Assistant listed an answer but will not give one: ask this screen no more, ping it as before.
            LOG.warning('Home Assistant gives no answer for %s (%s); pinging it without', action, error.detail)
            self.no_answers.add(action)
            await self.ha.send(inbox, message, action)
            answer = None
        self.pinged[inbox] = time.monotonic()
        self.answered(inbox, screen, answer)

    def take_dirty(self):
        """Entity ids that changed since the last pass, or None when everything must be rebuilt."""
        dirty = getattr(self.ha, 'dirty', None)
        if dirty is None:
            return None
        self.ha.dirty = set()
        registry = getattr(self.ha, 'registry', None)
        if registry is not self._seen_registry:
            # A new registry can change names and display precision: rebuild everything once.
            self._seen_registry = registry
            return None
        return dirty

    async def run(self):
        while True:
            try:
                await asyncio.wait_for(self.ha.changed.wait(), timeout=20)
            except TimeoutError:
                pass
            self.ha.changed.clear()
            await asyncio.sleep(0.25)
            if not self.ha.online:
                self.sent.clear()
                # States made over the REST API are gone once Home Assistant restarts: publish them again.
                self.published.clear()
                self.notify()
                continue
            for event in getattr(self.ha,'setting_events',[])[:]:
                try:
                    self.screen_setting_event(event)
                except (ValueError, TypeError, AttributeError) as error:
                    LOG.warning('A setting %s reported by %s was not kept (%s)', event.get('key') if isinstance(event, dict) else '?',
                                event.get('inbox') if isinstance(event, dict) else '?', error)
            if hasattr(self.ha,'setting_events'): self.ha.setting_events.clear()
            dirty = self.take_dirty()
            screens_key = self._screens_key
            screens = self.screens()
            changed = self._screens_key != screens_key
            self.ha.relevant = self.watched_entities()
            # A setting changed on a screen that owns them: the editor shows it.
            settings_key = self.settings_states_key()
            if settings_key != self._settings_key:
                self._settings_key, changed = settings_key, True
            for screen in screens:
                inbox = screen['id']
                before = self.status.get(inbox)
                if not screen['online']:
                    self.sent.pop(inbox, None)
                    self.status[inbox] = 'Screen offline; changes saved'
                    changed = changed or self.status[inbox] != before
                    continue
                if inbox not in self.layouts:
                    continue
                try:
                    now = time.monotonic()
                    pings = self.supports_ping(inbox, screen)
                    # The screen says it lost the layout (restart, mismatched ping, a state that never came).
                    if inbox in self.sent and screen['status'] in RESEND_STATES and now - self.last.get(inbox, 0) >= RESEND_GUARD_SECONDS:
                        self.sent.pop(inbox)
                    # An answer said so right after a full send; its wait is over (firmware 0.2.49+).
                    if inbox in self.retry_at and now >= self.retry_at[inbox]:
                        del self.retry_at[inbox]
                        self.sent.pop(inbox, None)
                    force = now - self.last.get(inbox, 0) >= (FULL_REPEAT_SECONDS if pings else KEEPALIVE_SECONDS)
                    if await self.sync_one(inbox, self.layouts[inbox], force, screen, dirty):
                        changed = True
                    if pings and inbox in self.sent and now - self.pinged.get(inbox, 0) >= KEEPALIVE_SECONDS:
                        await self.ping(inbox, screen)
                except Exception as error:
                    self.sent.pop(inbox, None)
                    self.status[inbox] = 'Sending failed; retrying automatically'
                    LOG.warning('Retrying screen sync (%s)', type(error).__name__)
                    await asyncio.sleep(1)
                changed = changed or self.status.get(inbox) != before
            await self.publish_layouts()
            if changed:
                self.notify()

    def history_keys(self):
        """(entity, hours) of every sensor tile on any screen."""
        return {(tile['entity'], tile.get('options', {}).get('history_hours', 24))
                for layout in self.layouts.values() for tile in layout['tiles'] if tile['entity'].startswith('sensor.')}

    async def refresh_histories(self):
        """Fetch the histories that are missing or older than HISTORY_SECONDS: one statistics request per
        window for all sensors at once, the REST history only for sensors without statistics. Changed
        values mark their entity dirty so the sync loop resends just those tiles."""
        wanted = self.history_keys()
        for key in [key for key in self.histories if key not in wanted]:
            del self.histories[key]
        now = time.monotonic()
        due = [key for key in wanted if key not in self.histories or now - self.histories[key][0] >= HISTORY_SECONDS]
        if not due:
            return
        found = {}
        if hasattr(self.ha, 'statistics'):
            for hours in sorted({hours for _, hours in due}):
                entities = {entity for entity, h in due if h == hours}
                try:
                    found.update({(entity, hours): values for entity, values in (await self.ha.statistics(entities, hours)).items()})
                except Exception as error:
                    LOG.warning('Fetching statistics failed (%s); falling back to per-sensor history', type(error).__name__)
        changed = set()
        for key in due:
            values = found.get(key)
            if values is None:
                try:
                    values = await self.ha.history(*key)
                except Exception:
                    values = []
            old = self.histories.get(key)
            self.histories[key] = (time.monotonic(), values)
            if old is None or old[1] != values:
                changed.add(key[0])
        if changed:
            dirty = getattr(self.ha, 'dirty', None)
            if dirty is not None:
                dirty.update(changed)
            self.ha.changed.set()

    async def history_loop(self):
        """Background refresh of sensor histories, so the sync loop never waits for Home Assistant's database."""
        while True:
            with contextlib.suppress(TimeoutError):
                await asyncio.wait_for(self.history_wake.wait(), 30)
            self.history_wake.clear()
            if not self.ha.online or not hasattr(self.ha, 'history'):
                continue
            try:
                await self.refresh_histories()
            except Exception as error:
                LOG.warning('Refreshing history failed (%s)', type(error).__name__)

    async def broadcast(self, event_type, data):
        """An alert event for every screen: the matching action on each screen that can show it, all at once."""
        action = BROADCAST_EVENTS[event_type]
        service_data, unusable = alert_data(data) if action == 'show_alert' else ({}, [])
        if unusable:
            LOG.warning('%s: unusable %s left empty', event_type, ', '.join(unusable))
        ready, skipped = alert_targets(self.screens())
        results = await asyncio.gather(*(self.ha.call(alert_service(screen['node'], action), service_data) for screen in ready),
                                       return_exceptions=True)
        failed = [(screen, type(result).__name__) for screen, result in zip(ready, results) if isinstance(result, BaseException)]
        notes = [f"{screen['name']} {reason}" for screen, reason in skipped + failed]
        LOG.info('%s: %d of %d screens%s', event_type, len(ready) - len(failed), len(ready) + len(skipped),
                 f" (not: {'; '.join(notes)})" if notes else '')
        return {'sent': len(ready) - len(failed), 'skipped': len(skipped), 'failed': len(failed)}

    async def tile_event(self, event_type, data):
        """One tile event: the screen it names, the changed layout, saved and pushed like the editor does."""
        screen = match_screen(self.screens(), data.get('screen'), self.layouts)
        inbox = self.aliases.get(screen['id'], screen['id'])
        layout = self.layouts.get(inbox) or {'title': screen['name'], 'tiles': []}
        self.save(inbox, apply_tile_event(layout, TILE_EVENTS[event_type], data))
        return screen

    async def tile_loop(self):
        """Tile events (app 0.2.51+) in arrival order, apart from the sync loop, which can be busy for a
        while. Every event gets an answer, so whoever fired it knows what happened."""
        queue = getattr(self.ha, 'tile_events', None)
        while queue is not None:
            event_type, data = await queue.get()
            answer = {'event': event_type, 'screen': data.get('screen') or '', 'entity': data.get('entity') or ''}
            try:
                screen = await self.tile_event(event_type, data)
                answer.update(ok=True, screen=screen['name'])
                LOG.info('%s: %s on %s', event_type, answer['entity'] or 'order', screen['name'])
                await self.publish_layouts()
            except Exception as error:
                answer.update(ok=False, error=str(error) if isinstance(error, ValueError) else type(error).__name__)
                LOG.warning('%s refused: %s', event_type, answer['error'])
            try:
                await self.ha.fire(TILE_RESULT_EVENT, answer)
            except Exception as error:
                LOG.warning('%s: no answer sent (%s)', event_type, type(error).__name__)

    async def publish_layouts(self):
        """Each screen's layout as a sensor in Home Assistant, so an assistant can read what is where.
        Published again after every change and after a reconnect; a state made this way is gone once
        Home Assistant restarts."""
        for screen in self.screens():
            inbox = self.aliases.get(screen['id'], screen['id'])
            layout, node = self.layouts.get(inbox), screen.get('node')
            if not layout or not node:
                continue
            snapshot = layout_snapshot(screen, layout)
            if self.published.get(inbox) == snapshot:
                continue
            try:
                await self.ha.set_state('sensor.esp_screens_' + node.replace('-', '_'), len(snapshot['tiles']),
                                        {'friendly_name': f"{screen['name']} tiles", 'icon': 'mdi:view-dashboard-outline', **snapshot})
                self.published[inbox] = snapshot
            except Exception as error:
                LOG.warning('Publishing the layout of %s failed (%s)', screen['name'], type(error).__name__)

    async def alert_loop(self):
        """Alert events for every screen, in arrival order and apart from the sync loop, which can be busy for a while."""
        queue = getattr(self.ha, 'broadcasts', None)
        while queue is not None:
            event_type, data = await queue.get()
            try:
                await self.broadcast(event_type, data)
            except Exception as error:
                LOG.warning('%s failed (%s)', event_type, type(error).__name__)

def create_app(manager, development=False):
    csrf = secrets.token_urlsafe(32)
    @web.middleware
    async def guard(request, handler):
        allowed = {'127.0.0.1', '::1'} if development else {'172.30.32.2'}
        if request.remote not in allowed:
            raise web.HTTPForbidden(text='Open this page through Home Assistant.')
        if request.method not in {'GET', 'HEAD'} and request.headers.get('X-Screen-CSRF') != csrf:
            raise web.HTTPForbidden(text='Refresh this page and try again.')
        try:
            response = await handler(request)
        except ValueError as error:
            return web.json_response({'error': str(error)}, status=400)
        except (TypeError, KeyError):
            return web.json_response({'error': 'Invalid input. Check the name, board, and chosen tiles.'}, status=400)
        if response.prepared:
            return response  # streamed (SSE) responses set their headers before prepare()
        response.headers['Cache-Control'] = 'no-store'
        response.headers['X-Content-Type-Options'] = 'nosniff'
        response.headers['Content-Security-Policy'] = "default-src 'self'; script-src 'self'; style-src 'self'; img-src 'self' data:; connect-src 'self'; base-uri 'none'; form-action 'self'"
        return response

    app = web.Application(middlewares=[guard], client_max_size=16*1024)
    static = Path(__file__).parent / 'static'

    # The page asks for its script and styles with a stamp of their content, so a browser that keeps old copies
    # anyway (Safari did after an update) never runs an old script against a new page.
    stamp = hashlib.sha1(b''.join((static / name).read_bytes() for name in ('app.js', 'style.css'))).hexdigest()[:12]
    page = (static / 'index.html').read_text().replace('"static/app.js"', f'"static/app.js?v={stamp}"').replace('"static/style.css"', f'"static/style.css?v={stamp}"')
    async def index(request):
        return web.Response(text=page, content_type='text/html')
    def light_payload(screens=None):
        """Screens and update status: everything that changes while the page is open."""
        if screens is None:
            screens = manager.screens()
        profiles = manager.firmware.profile_names()
        for screen in screens:
            screen['layout'] = manager.layouts.get(screen['id'], {'title': 'Home', 'tiles': []})
            screen['settings'] = manager.settings_view(screen)
            screen['delivery'] = manager.status.get(screen['id'], 'Choose your first tiles')
            screen['update'] = manager.updates.state_for(screen, profiles)
            screen['alert_action'] = alert_service(screen.get('node'))
            screen['dismiss_action'] = alert_service(screen.get('node'), 'dismiss_alert')
        return {'csrf': csrf, 'connected': manager.ha.online, 'screens': screens,
                'pending': manager.pending_profiles(screens, profiles),
                'updates': manager.updates.summary(screens, profiles)}
    async def inventory(request):
        if request.query.get('light') == '1':
            # The page polls the light form; entities, backgrounds and icons (~100 KB) only on demand.
            return web.json_response(light_payload())
        screens, entities = manager.inventory()
        payload = light_payload(screens)
        payload['entities'] = entities
        payload['backgrounds'] = TILE_BACKGROUNDS
        payload['controls'] = controls_catalogue()
        payload['icons'] = tile_icons.editor()
        payload['alerts'] = alert_reference()
        payload['claude_skill'] = claude_skill.status(manager.skill_dir)
        payload['builtin'] = [{'id': key, 'name': name, 'device': 'Built into the screen', 'area': '', 'state': 'ok'} for key, name in BUILTIN.items()]
        payload['header'] = {**header_bar.catalogue(), 'suggestions': {
            screen['id']: header_bar.suggestions(screen, entities, manager.ha.states, manager.registry_index()) for screen in payload['screens']}}
        return web.json_response(payload)
    async def header_preview(request):
        """The top bar as a screen would draw it right now, so the editor shows unsaved changes live."""
        data = await request.json()
        header = validate_header(data.get('header') if isinstance(data, dict) else None)
        return web.json_response({'items': header_bar.preview(header, manager.ha.states, manager.registry_index(),
                                                              getattr(manager.ha, 'units', {}), getattr(manager.ha, 'time_zone', None))})
    async def events(request):
        """Server-sent events: pushes the light inventory whenever it changes, so the page need not poll."""
        response = web.StreamResponse(headers={'Content-Type': 'text/event-stream', 'Cache-Control': 'no-store',
                                               'X-Accel-Buffering': 'no', 'X-Content-Type-Options': 'nosniff'})
        await response.prepare(request)
        wake, sent = asyncio.Event(), None
        wake.set()  # first event goes out right away
        manager.listeners.add(wake)
        try:
            while True:
                # Sync results are pushed at once; updater phases are picked up by the 3 s check.
                with contextlib.suppress(TimeoutError):
                    await asyncio.wait_for(wake.wait(), 3)
                wake.clear()
                body = json.dumps(light_payload(), ensure_ascii=False)
                if body != sent:
                    await response.write(f'data: {body}\n\n'.encode())
                    sent = body
                else:
                    await response.write(b': keepalive\n\n')
        except (ConnectionResetError, asyncio.CancelledError):
            pass
        finally:
            manager.listeners.discard(wake)
        return response
    async def save(request):
        manager.save(request.match_info['inbox'], await request.json())
        return web.json_response({'saved': True})
    async def change_settings(request):
        """Screen settings apply one change at a time, like the settings page on the screen (app 0.2.57)."""
        data = await request.json()
        try:
            view = await manager.change_settings(request.match_info['inbox'], data.get('settings') if isinstance(data, dict) else None)
        except Refused as error:
            raise ValueError(f"Home Assistant didn't take the change: {error.detail or 'no reason given'}.") from error
        except (ConnectionError, TimeoutError) as error:
            raise ValueError("Home Assistant isn't reachable right now. Try again in a moment.") from error
        return web.json_response(view)
    async def update_screen(request):
        data = await request.json() if request.can_read_body else {}
        return web.json_response(manager.updates.start(request.match_info['inbox'], data.get('host')))
    async def update_all(request):
        return web.json_response({'started': manager.updates.start_all()})
    async def update_settings(request):
        manager.updates.set_auto((await request.json()).get('auto'))
        return web.json_response(manager.updates.summary())
    async def inspector(request):
        screen=manager.screen(request.match_info['inbox'])
        inbox=manager.aliases.get(request.match_info['inbox'], request.match_info['inbox'])
        if screen is None: raise ValueError('Unknown screen.')
        layout=manager.layouts.get(inbox,{'tiles':[]})
        return web.json_response({'screen':screen,
            'delivery':manager.status.get(inbox), 'layout':layout,
            'tiles':[{'entity':t['entity'],'state':manager.ha.states.get(t['entity'],{}).get('state'),
                      'attributes':state_message(i,t,manager.ha.states)['a'],
                      'options':t.get('options',{})} for i,t in enumerate(layout['tiles'])]})
    async def install_claude_skill(request):
        """Settings → Claude → Install: writes the skill into Home Assistant's configuration folder, only on request."""
        return web.json_response(claude_skill.install(manager.skill_dir))
    async def download_claude_skill(request):
        """Settings → Claude → Download: the same skill as a zip for claude.ai; writes nothing."""
        return web.Response(body=claude_skill.archive(), content_type='application/zip',
                            headers={'Content-Disposition': f'attachment; filename="{claude_skill.NAME}.zip"'})
    async def firmware_status(request): return web.json_response(manager.firmware.status())
    async def firmware_start(request): return web.json_response(manager.firmware.start(await request.json()))
    async def firmware_create(request):
        # Profile, missing wifi secrets and (with a USB port) the build and flash in one request.
        return web.json_response(manager.firmware.install(await request.json()))
    async def shutdown(app):
        for task in (manager.updates.task, manager.firmware.task):
            if task and not task.done():
                task.cancel()
                with contextlib.suppress(asyncio.CancelledError): await task
    app.on_cleanup.append(shutdown)
    app.router.add_get('/api/screens/{inbox}/inspect', inspector)
    app.router.add_post('/api/screens/{inbox}/update', update_screen)
    app.router.add_post('/api/updates/run', update_all)
    app.router.add_put('/api/updates', update_settings)
    app.router.add_get('/api/firmware', firmware_status)
    app.router.add_post('/api/firmware/jobs', firmware_start)
    app.router.add_post('/api/firmware/profiles', firmware_create)
    app.router.add_get('/', index)
    app.router.add_get('/api/inventory', inventory)
    app.router.add_post('/api/header-preview', header_preview)
    app.router.add_post('/api/claude-skill', install_claude_skill)
    app.router.add_get('/api/claude-skill.zip', download_claude_skill)
    app.router.add_get('/api/events', events)
    app.router.add_put('/api/screens/{inbox}', save)
    app.router.add_put('/api/screens/{inbox}/settings', change_settings)
    app.router.add_static('/static/', static)
    return app

async def main():
    development = os.environ.get('SCREEN_DEV') == '1'
    token = os.environ.get('SUPERVISOR_TOKEN', '')
    if development and os.environ.get('HA_TOKEN_FILE'):
        token = Path(os.environ['HA_TOKEN_FILE']).read_text().strip()
    if not token:
        raise SystemExit('No Home Assistant access. Start the app via Supervisor.')
    async with ClientSession(timeout=ClientTimeout(total=20)) as session:
        ha = HomeAssistant(session, os.environ.get('HA_API', 'http://supervisor/core/api'), token)
        manager = Manager(ha, Path(os.environ.get('SCREEN_DATA', '/data')) / 'screens.json')
        runner = web.AppRunner(create_app(manager, development), access_log=None)
        await runner.setup()
        await web.TCPSite(runner, '127.0.0.1' if development else '0.0.0.0', 8099).start()
        try:
            await asyncio.gather(ha.run(), manager.run(), manager.history_loop(), manager.updates.run(),
                                 manager.alert_loop(), manager.tile_loop(), manager.card_history_loop())
        finally:
            await runner.cleanup()

if __name__ == '__main__':
    logging.basicConfig(level=logging.INFO, format='%(levelname)s %(message)s')
    asyncio.run(main())
