"""HA Ingress app. The only HA write is text.set_value on discovered inboxes."""
import asyncio
import contextlib
import json
import logging
import os
from pathlib import Path
import secrets
import time

from aiohttp import ClientSession, ClientTimeout, WSMsgType, web
from core import discover, installation_yaml, packets, state_message, validate_layout

LOG = logging.getLogger('screen_manager')

class HomeAssistant:
    def __init__(self, session, base, token):
        self.session, self.base, self.token = session, base.rstrip('/'), token
        self.ws = None
        self.next_id = 0
        self.pending = {}
        self.states = {}
        self.registry, self.devices, self.areas = [], [], []
        self.online = False
        self.changed = asyncio.Event()

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
                if event.get('event_type') == 'state_changed':
                    eid = body.get('entity_id')
                    if body.get('new_state'):
                        self.states[eid] = body['new_state']
                    else:
                        self.states.pop(eid, None)
                    self.changed.set()
        raise ConnectionError('Home Assistant-verbinding verbroken.')

    async def registries(self):
        self.registry, self.devices, self.areas = await asyncio.gather(
            self.request('config/entity_registry/list'), self.request('config/device_registry/list'), self.request('config/area_registry/list'))

    async def run(self):
        url = ('ws://supervisor/core/websocket' if self.base == 'http://supervisor/core/api'
               else self.base.replace('http://', 'ws://').replace('https://', 'wss://') + '/websocket')
        while True:
            reader = None
            try:
                async with self.session.ws_connect(url, heartbeat=30, max_msg_size=16*1024*1024) as ws:
                    self.ws = ws
                    await ws.receive_json(timeout=15)
                    await ws.send_json({'type': 'auth', 'access_token': self.token})
                    if (await ws.receive_json(timeout=15)).get('type') != 'auth_ok':
                        raise ConnectionError('Home Assistant-authenticatie mislukt.')
                    reader = asyncio.create_task(self.read())
                    await self.request('subscribe_events', event_type='state_changed')
                    self.states = {s['entity_id']: s for s in await self.request('get_states')}
                    await self.registries()
                    self.online = True
                    self.changed.set()
                    LOG.info('Home Assistant verbonden')
                    # Refresh registry for newly paired screens and renamed entities.
                    while True:
                        done, _ = await asyncio.wait([reader], timeout=30)
                        if done:
                            await reader
                        await self.registries()
                        self.changed.set()
            except (ConnectionError, TimeoutError, OSError, ValueError) as error:
                LOG.warning('Home Assistant tijdelijk niet beschikbaar (%s)', type(error).__name__)
            except Exception as error:
                LOG.warning('Verbinding opnieuw starten (%s)', type(error).__name__)
            finally:
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

class Manager:
    def __init__(self, ha, path):
        self.ha, self.path = ha, Path(path)
        self.layouts, self.sent, self.status, self.last = {}, {}, {}, {}
        if self.path.exists():
            raw = json.loads(self.path.read_text())
            # Versioned persistent data. Never silently overwrite an unknown schema.
            if raw.get('version') != 1 or not isinstance(raw.get('screens'), dict):
                raise ValueError('Onbekende opslagversie; gegevens blijven ongewijzigd.')
            self.layouts = {key: validate_layout(value) for key, value in raw['screens'].items()}

    def inventory(self):
        return discover(self.ha.registry, self.ha.states, self.ha.devices, self.ha.areas)

    def save(self, inbox, data):
        if inbox not in {s['id'] for s in self.inventory()[0]}:
            raise ValueError('Dit is geen gekoppeld ESP-scherm. Vernieuw het overzicht.')
        layout = validate_layout(data)
        # A still-open older UI may save tiles without the new optional settings.
        if 'settings' not in layout and 'settings' in self.layouts.get(inbox, {}):
            layout['settings'] = self.layouts[inbox]['settings'].copy()
        known = {e['id'] for e in self.inventory()[1]}
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

    async def sync_one(self, inbox, layout, force=False):
        messages = [{'v': 1, 'op': 'layout', 'title': layout['title'], 'entities': [t['entity'] for t in layout['tiles']]}]
        if 'settings' in layout:
            messages[0]['settings'] = layout['settings']
        messages += [state_message(i, tile, self.ha.states) for i, tile in enumerate(layout['tiles'])]
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
                continue
            for screen in self.inventory()[0]:
                inbox = screen['id']
                if not screen['online']:
                    self.sent.pop(inbox, None)
                    self.status[inbox] = 'Scherm offline; wijzigingen bewaard'
                    continue
                if inbox not in self.layouts:
                    continue
                try:
                    force = time.monotonic() - self.last.get(inbox, 0) >= 25
                    # last records full keepalive, not unrelated HA state events.
                    before = self.last.get(inbox, 0)
                    await self.sync_one(inbox, self.layouts[inbox], force)
                    if not force:
                        self.last[inbox] = before
                except Exception as error:
                    self.sent.pop(inbox, None)
                    self.status[inbox] = 'Verzenden mislukt; automatisch opnieuw proberen'
                    LOG.warning('Schermsynchronisatie opnieuw proberen (%s)', type(error).__name__)
                    await asyncio.sleep(1)

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
        response.headers['Cache-Control'] = 'no-store'
        response.headers['X-Content-Type-Options'] = 'nosniff'
        response.headers['Content-Security-Policy'] = "default-src 'self'; script-src 'self'; style-src 'self'; img-src 'self' data:; connect-src 'self'; base-uri 'none'; form-action 'self'"
        return response

    app = web.Application(middlewares=[guard], client_max_size=16*1024)
    static = Path(__file__).parent / 'static'

    async def index(request):
        return web.FileResponse(static / 'index.html')
    async def inventory(request):
        screens, entities = manager.inventory()
        for screen in screens:
            screen['layout'] = manager.layouts.get(screen['id'], {'title': 'Thuis', 'tiles': []})
            screen['delivery'] = manager.status.get(screen['id'], 'Kies je eerste tegels')
        return web.json_response({'csrf': csrf, 'connected': manager.ha.online, 'screens': screens, 'entities': entities})
    async def save(request):
        manager.save(request.match_info['inbox'], await request.json())
        return web.json_response({'saved': True})
    async def download(request):
        data = await request.json()
        content = installation_yaml(data)
        return web.Response(text=content, content_type='text/yaml', headers={'Content-Disposition': f'attachment; filename="{data["name"]}.yaml"'})
    app.router.add_get('/', index)
    app.router.add_get('/api/inventory', inventory)
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
            await asyncio.gather(ha.run(), manager.run())
        finally:
            await runner.cleanup()

if __name__ == '__main__':
    logging.basicConfig(level=logging.INFO, format='%(levelname)s %(message)s')
    asyncio.run(main())
