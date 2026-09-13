"""HA Ingress app. HA writes: text.set_value on discovered inboxes and one persistent notification when a nightly update stops."""
import asyncio
import contextlib
import json
import logging
import os
from pathlib import Path
import secrets
import time
from datetime import datetime, timedelta, timezone
import math
from firmware import Firmware
import tile_icons
from updates import Updater

from aiohttp import ClientSession, ClientTimeout, WSMsgType, web
from core import BUILTIN, TILE_BACKGROUNDS, discover, extras, installation_yaml, min_firmware, packets, state_message, validate_layout, validate_settings
from zoneinfo import ZoneInfo

LOG = logging.getLogger('screen_manager')

REGISTRY_EVENTS = ('entity_registry_updated', 'device_registry_updated', 'area_registry_updated')
SCREEN_ENTITY_NAMES = {'Tegelinstellingen', 'Schermfirmware', 'Guition schermtype', 'Apparaatnaam', 'IP-adres'}
KEEPALIVE_SECONDS = 120

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
        self.registry_changed = asyncio.Event()
        self.setting_events = []
        self.time_zone = None

    async def request(self, kind, **data):
        if self.ws is None or self.ws.closed:
            raise ConnectionError('Home Assistant is niet verbonden.')
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
                        future.set_exception(ConnectionError('Home Assistant heeft de opdracht geweigerd.'))
            elif data.get('type') == 'event':
                event = data.get('event', {})
                body = event.get('data', {})
                if event.get('event_type') == 'esphome.screen_setting':
                    self.setting_events.append(body)
                    self.changed.set()
                elif event.get('event_type') == 'state_changed':
                    eid = body.get('entity_id')
                    if body.get('new_state'):
                        self.states[eid] = body['new_state']
                    else:
                        self.states.pop(eid, None)
                    if self.relevant is None or eid in self.relevant:
                        self.changed.set()
                elif event.get('event_type') in REGISTRY_EVENTS:
                    self.registry_changed.set()
        raise ConnectionError('Home Assistant-verbinding verbroken.')

    def describe_close(self, connected):
        """Why and after how long the websocket ended; helps explain unexpected reconnects in the log."""
        if connected is None:
            return ''
        ws = self.ws
        reason = ws.exception() if ws is not None else None
        return ' na %d s, close-code %s%s' % (time.monotonic() - connected, ws.close_code if ws is not None else None,
                                            f', {type(reason).__name__}' if reason else '')

    async def registries(self):
        self.registry, self.devices, self.areas = await asyncio.gather(
            self.request('config/entity_registry/list'), self.request('config/device_registry/list'), self.request('config/area_registry/list'))

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
                        raise ConnectionError('Home Assistant-authenticatie mislukt.')
                    reader = asyncio.create_task(self.read())
                    await self.request('subscribe_events', event_type='state_changed')
                    await self.request('subscribe_events', event_type='esphome.screen_setting')
                    for event_type in REGISTRY_EVENTS:
                        await self.request('subscribe_events', event_type=event_type)
                    self.states = {s['entity_id']: s for s in await self.request('get_states')}
                    await self.registries()
                    try:
                        self.time_zone = ZoneInfo((await self.request('get_config')).get('time_zone') or 'UTC')
                    except Exception:
                        self.time_zone = timezone.utc
                    self.online = True
                    self.changed.set()
                    connected = time.monotonic()
                    LOG.info('Home Assistant verbonden')
                    # Refresh the registry when HA reports a change (debounced), with a slow
                    # fallback; a full fetch is about 1 MB of JSON and used to run every 30 s.
                    fetched = time.monotonic()
                    while True:
                        waiter = asyncio.ensure_future(self.registry_changed.wait())
                        try:
                            done, _ = await asyncio.wait([reader, waiter], timeout=30, return_when=asyncio.FIRST_COMPLETED)
                        finally:
                            waiter.cancel()
                        if reader in done:
                            await reader
                        if self.registry_changed.is_set():
                            await asyncio.sleep(1)
                            self.registry_changed.clear()
                        elif time.monotonic() - fetched < self.registry_interval:
                            continue
                        await self.registries()
                        fetched = time.monotonic()
                        self.changed.set()
            except (ConnectionError, TimeoutError, OSError, ValueError) as error:
                LOG.warning('Home Assistant tijdelijk niet beschikbaar (%s)%s', type(error).__name__, self.describe_close(connected))
            except Exception as error:
                LOG.warning('Verbinding opnieuw starten (%s)%s', type(error).__name__, self.describe_close(connected))
            finally:
                if self.online:
                    LOG.info('Home Assistant-verbinding gesloten%s', self.describe_close(connected))
                self.online = False
                self.ws = None
                if reader:
                    reader.cancel()
                    with contextlib.suppress(asyncio.CancelledError, Exception):
                        await reader
                for future in self.pending.values():
                    if not future.done():
                        future.set_exception(ConnectionError('Verbinding verbroken.'))
            await asyncio.sleep(5)

    async def send(self, inbox, message):
        for packet in packets(message):
            await self.request('call_service', domain='text', service='set_value',
                               service_data={'entity_id': inbox, 'value': packet})

    async def forecast(self, entity):
        # Forecasts left the weather attributes in HA 2024.4; ask the service instead.
        result = await self.request('call_service', domain='weather', service='get_forecasts',
                                    service_data={'type': 'daily'}, target={'entity_id': entity}, return_response=True)
        forecast = ((result or {}).get('response') or {}).get(entity, {}).get('forecast', [])
        return forecast if isinstance(forecast, list) else []

    async def history(self, entity, hours):
        start = (datetime.now(timezone.utc)-timedelta(hours=hours)).isoformat()
        async with self.session.get(self.base+'/history/period/'+start,
                params={'filter_entity_id':entity,'minimal_response':'','no_attributes':''},
                headers={'Authorization':'Bearer '+self.token}) as response:
            response.raise_for_status()
            raw = bytearray()
            async for chunk in response.content.iter_chunked(65536):
                raw.extend(chunk)
                if len(raw)>2*1024*1024: raise ValueError('Geschiedenis te groot.')
            rows=json.loads(raw)
        samples=[]; events=[]
        begin=datetime.fromisoformat(start).timestamp(); span=hours*3600
        for row in rows[0] if rows else []:
            try:
                timestamp=datetime.fromisoformat(row.get('last_changed',row.get('last_updated','')).replace('Z','+00:00')).timestamp()
            except (ValueError,TypeError): continue
            try:
                value=float(row['state']); value=round(value,3) if math.isfinite(value) else None
            except (ValueError,TypeError,KeyError): value=None
            events.append((timestamp,value))
        events.sort(key=lambda pair:pair[0]);position=0;value=None
        for bucket in range(24):
            boundary=begin+(bucket+1)*span/24
            while position<len(events) and events[position][0]<=boundary:
                value=events[position][1];position+=1
            samples.append(value)
        return samples

class Manager:
    def __init__(self, ha, path):
        self.ha, self.path = ha, Path(path)
        self.layouts, self.sent, self.status, self.last = {}, {}, {}, {}
        self.histories = {}
        self.forecasts = {}
        self.listeners = set()  # asyncio.Event per open /api/events stream
        self.firmware = Firmware(os.environ.get("ESPHOME_CONFIG", "/homeassistant/esphome"), self.path.parent)
        self.updates = Updater(self, self.path.parent / 'updates.json')
        if self.path.exists():
            raw = json.loads(self.path.read_text())
            # Versioned persistent data. Never silently overwrite an unknown schema.
            if raw.get('version') != 1 or not isinstance(raw.get('screens'), dict):
                raise ValueError('Onbekende opslagversie; gegevens blijven ongewijzigd.')
            self.layouts = {key: validate_layout(value) for key, value in raw['screens'].items()}

    def inventory(self):
        return discover(self.ha.registry, self.ha.states, self.ha.devices, self.ha.areas)

    def firmware_version(self, inbox, screen=None):
        if screen is None:
            screen=next((s for s in self.inventory()[0] if s['id']==inbox), {})
        try:
            version=tuple(int(part) for part in screen.get('firmware','').split('.'))
            return version if len(version)==3 else None
        except (ValueError, TypeError):
            return None

    def supports_twenty(self, inbox):
        version=self.firmware_version(inbox)
        return bool(version) and version >= (0,2,7)

    def needs_firmware(self, inbox, layout, screen=None):
        """Version string the screen must run first, or None when the layout can be sent."""
        needed=min_firmware(layout)
        if needed and not ((self.firmware_version(inbox, screen) or (0,0,0)) >= needed):
            return '.'.join(str(part) for part in needed)
        return None

    def save(self, inbox, data):
        screens, entities = self.inventory()
        screen=next((s for s in screens if s['id']==inbox), None)
        if screen is None:
            raise ValueError('Dit is geen gekoppeld ESP-scherm. Vernieuw het overzicht.')
        layout = validate_layout(data)
        needed = self.needs_firmware(inbox, layout, screen)
        if needed:
            raise ValueError(f"Installeer eerst schermfirmware {needed} of nieuwer voor deze tegels.")
        # A still-open older UI may save tiles without the new optional settings.
        if 'settings' not in layout and 'settings' in self.layouts.get(inbox, {}):
            layout['settings'] = self.layouts[inbox]['settings'].copy()
        if 'settings' in layout and 'swipe_pages' not in data.get('settings',{}):
            layout['settings']['swipe_pages']=self.layouts.get(inbox,{}).get('settings',{}).get('swipe_pages',False)
        if 'settings' in layout and 'rotation' not in data.get('settings',{}):
            layout['settings']['rotation']=self.layouts.get(inbox,{}).get('settings',{}).get('rotation',0)
        if layout.get('settings',{}).get('rotation',0) and screen.get('board')!='guition':
            raise ValueError('Rotatie vereist een Guition met firmware 0.2.9 of nieuwer.')
        old_tiles = {t['entity']:t for t in self.layouts.get(inbox,{}).get('tiles',[])}
        for tile in layout['tiles']:
            old_options=old_tiles.get(tile['entity'],{}).get('options',{})
            for key in ('background', 'icon'):
                if 'options' in tile and key not in tile['options'] and key in old_options:
                    tile['options'][key]=old_options[key]
            if 'options' not in tile and 'options' in old_tiles.get(tile['entity'],{}):
                tile['options'] = old_tiles[tile['entity']]['options'].copy()
        known = {e['id'] for e in entities} | set(BUILTIN)
        if any(t['entity'] not in known for t in layout['tiles']):
            raise ValueError('Een gekozen entiteit bestaat niet meer. Zoek de nieuwe entiteit op.')
        updated = {**self.layouts, inbox: layout}
        self.path.parent.mkdir(parents=True, exist_ok=True)
        temp = self.path.with_suffix('.tmp')
        with open(temp, 'w', encoding='utf8') as handle:
            os.chmod(temp, 0o600)
            json.dump({'version': 1, 'screens': updated}, handle, ensure_ascii=False)
            handle.flush()
            os.fsync(handle.fileno())
        temp.replace(self.path)
        self.layouts = updated
        self.sent.pop(inbox, None)
        self.status[inbox] = 'Opgeslagen; wacht op synchronisatie'
        self.ha.changed.set()
        self.notify()

    def notify(self):
        for listener in self.listeners:
            listener.set()

    def watched_entities(self):
        """Entities whose state changes matter: tiles on any layout plus the screens' own diagnostics."""
        watched = {tile['entity'] for layout in self.layouts.values() for tile in layout['tiles']}
        for item in getattr(self.ha, 'registry', []):
            if item.get('platform') == 'esphome' and item.get('original_name') in SCREEN_ENTITY_NAMES:
                watched.add(item['entity_id'])
        return watched

    async def cached(self, store, key, ttl, fetch):
        entry=store.get(key)
        if not entry or time.monotonic()-entry[0]>ttl:
            try: value=await fetch()
            except Exception: value=[]
            entry=(time.monotonic(),value);store[key]=entry
        return entry[1]

    async def sync_one(self, inbox, layout, force=False, screen=None):
        if screen is None:
            screen=next((s for s in self.inventory()[0] if s['id']==inbox), {})
        needed = self.needs_firmware(inbox, layout, screen)
        if needed:
            self.status[inbox]=f"Indeling bewaard; firmware {needed}+ nodig voor deze tegels"
            return
        messages = [{'v': 1, 'op': 'layout', 'inbox': inbox, 'title': layout['title'], 'entities': [t['entity'] for t in layout['tiles']]}]
        if 'settings' in layout:
            messages[0]['settings'] = {k:v for k,v in layout['settings'].items() if k not in ('swipe_pages','rotation')}
            messages[0]['swipe_pages'] = layout['settings'].get('swipe_pages',False)
            if screen.get('board')=='guition':
                messages[0]['rotation'] = layout['settings'].get('rotation',0)
        for i,tile in enumerate(layout['tiles']):
            forecast=None
            if tile['entity'].startswith('weather.') and hasattr(self.ha,'forecast'):
                forecast=await self.cached(self.forecasts, tile['entity'], 1800, lambda: self.ha.forecast(tile['entity']))
            message=state_message(i,tile,self.ha.states,extras(tile,self.ha.states,forecast,getattr(self.ha,'time_zone',None)))
            if tile['entity'].startswith('sensor.') and hasattr(self.ha,'history'):
                hours=tile.get('options',{}).get('history_hours',24)
                key=(tile['entity'],hours)
                message['history']={'hours':hours,'values':await self.cached(self.histories, key, 300, lambda: self.ha.history(*key))}
            messages.append(message)
        previous = self.sent.get(inbox, [])
        force = force or not previous or messages[0] != previous[0]
        for i, message in enumerate(messages):
            # Save during transmission aborts the old batch, then starts a full new layout.
            if self.layouts.get(inbox) != layout:
                self.sent.pop(inbox, None)
                return
            if force or i >= len(previous) or message != previous[i]:
                await self.ha.send(inbox, message)
        self.sent[inbox] = messages
        self.last[inbox] = time.monotonic()
        self.status[inbox] = 'Verzonden naar Home Assistant'

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
                self.notify()
                continue
            for event in getattr(self.ha,'setting_events',[])[:]:
                try:
                    inbox,key,value=event.get('inbox'),event.get('key'),event.get('value')
                    if inbox in self.layouts:
                        layout=dict(self.layouts[inbox]); settings=validate_settings(layout.get('settings',{}))
                        if key not in settings: continue
                        settings[key] = value=='1' if type(settings[key]) is bool else int(value)
                        if key=='brightness':
                            settings['standby_brightness']=min(settings['standby_brightness'],settings[key])
                            settings['night_brightness']=min(settings['night_brightness'],settings[key])
                        layout['settings']=validate_settings(settings);self.save(inbox,layout)
                except (ValueError,TypeError): pass
            if hasattr(self.ha,'setting_events'): self.ha.setting_events.clear()
            screens = self.inventory()[0]
            self.ha.relevant = self.watched_entities()
            for screen in screens:
                inbox = screen['id']
                if not screen['online']:
                    self.sent.pop(inbox, None)
                    self.status[inbox] = 'Scherm offline; wijzigingen bewaard'
                    continue
                if inbox not in self.layouts:
                    continue
                try:
                    force = time.monotonic() - self.last.get(inbox, 0) >= KEEPALIVE_SECONDS
                    # last records full keepalive, not unrelated HA state events.
                    before = self.last.get(inbox, 0)
                    await self.sync_one(inbox, self.layouts[inbox], force, screen)
                    if not force:
                        self.last[inbox] = before
                except Exception as error:
                    self.sent.pop(inbox, None)
                    self.status[inbox] = 'Verzenden mislukt; automatisch opnieuw proberen'
                    LOG.warning('Schermsynchronisatie opnieuw proberen (%s)', type(error).__name__)
                    await asyncio.sleep(1)
            self.notify()

def create_app(manager, development=False):
    csrf = secrets.token_urlsafe(32)
    @web.middleware
    async def guard(request, handler):
        allowed = {'127.0.0.1', '::1'} if development else {'172.30.32.2'}
        if request.remote not in allowed:
            raise web.HTTPForbidden(text='Open deze pagina via Home Assistant.')
        if request.method not in {'GET', 'HEAD'} and request.headers.get('X-Screen-CSRF') != csrf:
            raise web.HTTPForbidden(text='Vernieuw deze pagina en probeer opnieuw.')
        try:
            response = await handler(request)
        except ValueError as error:
            return web.json_response({'error': str(error)}, status=400)
        except (TypeError, KeyError):
            return web.json_response({'error': 'Ongeldige invoer. Controleer naam, bord en gekozen tegels.'}, status=400)
        if response.prepared:
            return response  # streamed (SSE) responses set their headers before prepare()
        response.headers['Cache-Control'] = 'no-store'
        response.headers['X-Content-Type-Options'] = 'nosniff'
        response.headers['Content-Security-Policy'] = "default-src 'self'; script-src 'self'; style-src 'self'; img-src 'self' data:; connect-src 'self'; base-uri 'none'; form-action 'self'"
        return response

    app = web.Application(middlewares=[guard], client_max_size=16*1024)
    static = Path(__file__).parent / 'static'

    async def index(request):
        return web.FileResponse(static / 'index.html')
    def light_payload():
        """Screens and update status: everything that changes while the page is open."""
        screens, entities = manager.inventory()
        profiles = manager.firmware.profile_names()
        for screen in screens:
            screen['layout'] = manager.layouts.get(screen['id'], {'title': 'Thuis', 'tiles': []})
            screen['delivery'] = manager.status.get(screen['id'], 'Kies je eerste tegels')
            screen['update'] = manager.updates.state_for(screen, profiles)
        return {'csrf': csrf, 'connected': manager.ha.online, 'screens': screens,
                'updates': manager.updates.summary(screens, profiles)}, entities
    async def inventory(request):
        payload, entities = light_payload()
        if request.query.get('light') != '1':
            # The page polls the light form; entities, backgrounds and icons (~100 KB) only on demand.
            payload['entities'] = entities
            payload['backgrounds'] = TILE_BACKGROUNDS
            payload['icons'] = tile_icons.editor()
            payload['builtin'] = [{'id': key, 'name': name, 'device': 'Ingebouwd op het scherm', 'area': '', 'state': 'ok'} for key, name in BUILTIN.items()]
        return web.json_response(payload)
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
                body = json.dumps(light_payload()[0], ensure_ascii=False)
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
    async def update_screen(request):
        data = await request.json() if request.can_read_body else {}
        return web.json_response(manager.updates.start(request.match_info['inbox'], data.get('host')))
    async def update_all(request):
        return web.json_response({'started': manager.updates.start_all()})
    async def update_settings(request):
        manager.updates.set_auto((await request.json()).get('auto'))
        return web.json_response(manager.updates.summary())
    async def download(request):
        data = await request.json()
        content = installation_yaml(data)
        return web.Response(text=content, content_type='text/yaml', headers={'Content-Disposition': f'attachment; filename="{data["name"]}.yaml"'})
    async def inspector(request):
        inbox=request.match_info['inbox']
        if inbox not in {s['id'] for s in manager.inventory()[0]}: raise ValueError('Onbekend scherm.')
        layout=manager.layouts.get(inbox,{'tiles':[]})
        return web.json_response({'screen':next(s for s in manager.inventory()[0] if s['id']==inbox),
            'delivery':manager.status.get(inbox), 'layout':layout,
            'tiles':[{'entity':t['entity'],'state':manager.ha.states.get(t['entity'],{}).get('state'),
                      'attributes':state_message(i,t,manager.ha.states)['a'],
                      'options':t.get('options',{})} for i,t in enumerate(layout['tiles'])]})
    async def firmware_status(request): return web.json_response(manager.firmware.status())
    async def firmware_start(request): return web.json_response(manager.firmware.start(await request.json()))
    async def firmware_create(request): return web.json_response(manager.firmware.create(await request.json()))
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
    app.router.add_get('/api/events', events)
    app.router.add_put('/api/screens/{inbox}', save)
    app.router.add_post('/api/install', download)
    app.router.add_static('/static/', static)
    return app

async def main():
    development = os.environ.get('SCREEN_DEV') == '1'
    token = os.environ.get('SUPERVISOR_TOKEN', '')
    if development and os.environ.get('HA_TOKEN_FILE'):
        token = Path(os.environ['HA_TOKEN_FILE']).read_text().strip()
    if not token:
        raise SystemExit('Geen Home Assistant-toegang. Start de app via Supervisor.')
    async with ClientSession(timeout=ClientTimeout(total=20)) as session:
        ha = HomeAssistant(session, os.environ.get('HA_API', 'http://supervisor/core/api'), token)
        manager = Manager(ha, Path(os.environ.get('SCREEN_DATA', '/data')) / 'screens.json')
        runner = web.AppRunner(create_app(manager, development), access_log=None)
        await runner.setup()
        await web.TCPSite(runner, '127.0.0.1' if development else '0.0.0.0', 8099).start()
        try:
            await asyncio.gather(ha.run(), manager.run(), manager.updates.run())
        finally:
            await runner.cleanup()

if __name__ == '__main__':
    logging.basicConfig(level=logging.INFO, format='%(levelname)s %(message)s')
    asyncio.run(main())
