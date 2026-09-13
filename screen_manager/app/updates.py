"""Firmware updates for paired screens: on demand, all at once, or nightly. One screen at a time."""
import asyncio
from datetime import datetime, timezone
import json
import logging
import os
from pathlib import Path
import re
import time
from core import FIRMWARE_VERSION

LOG = logging.getLogger('screen_manager')
NIGHT_HOURS = range(3, 6)

def parse_version(text):
    try:
        parts = tuple(int(part) for part in str(text).split('.'))
    except ValueError:
        return None
    return parts if len(parts) == 3 else None

TARGET = parse_version(FIRMWARE_VERSION)

class Updater:
    verify_timeout = 240
    settle_seconds = 60
    pause_seconds = 120
    poll_seconds = 5

    def __init__(self, manager, path):
        self.manager, self.path = manager, Path(path)
        self.auto, self.hosts, self.results, self.last_round = False, {}, {}, None
        self.task, self.current, self.queue, self.phase = None, None, [], None
        if self.path.exists():
            raw = json.loads(self.path.read_text())
            if raw.get('version') != 1:
                raise ValueError('Onbekende opslagversie voor updates; gegevens blijven ongewijzigd.')
            self.auto = raw.get('auto') is True
            self.hosts = {k: v for k, v in raw.get('hosts', {}).items() if isinstance(v, str)}
            self.results = {k: v for k, v in raw.get('results', {}).items() if isinstance(v, dict)}
            self.last_round = raw.get('last_round') if isinstance(raw.get('last_round'), str) else None

    def save(self):
        self.path.parent.mkdir(parents=True, exist_ok=True)
        temp = self.path.with_suffix('.tmp')
        with open(temp, 'w', encoding='utf8') as handle:
            os.chmod(temp, 0o600)
            json.dump({'version': 1, 'auto': self.auto, 'hosts': self.hosts, 'results': self.results,
                       'last_round': self.last_round}, handle, ensure_ascii=False)
            handle.flush()
            os.fsync(handle.fileno())
        temp.replace(self.path)

    def busy(self):
        return bool(self.task and not self.task.done())

    def screen(self, inbox):
        return next((s for s in self.manager.inventory()[0] if s['id'] == inbox), None)

    def resolve(self, screen):
        """Profile file and OTA address for a screen; None when the add-on cannot tell."""
        profiles = self.manager.firmware.profile_names()
        profile = next((f for f, p in profiles.items() if screen.get('node') and p['node'] == screen['node']), None)
        if profile is None and screen.get('device'):
            # Screens on firmware before 0.2.17 do not report their node name yet.
            matches = [f for f, p in profiles.items() if p['friendly'] == screen['device']]
            profile = matches[0] if len(matches) == 1 else None
        return profile, screen.get('ip') or self.hosts.get(screen['id'])

    def state_for(self, screen):
        version = parse_version(screen.get('firmware'))
        profile, host = self.resolve(screen)
        if screen['id'] == self.current:
            state = 'running'
        elif screen['id'] in self.queue:
            state = 'queued'
        else:
            state = 'idle'
        return {'available': bool(version and TARGET and version < TARGET), 'target': FIRMWARE_VERSION,
                'profile': profile, 'host': host, 'state': state, 'phase': self.phase if state == 'running' else None,
                'result': self.results.get(screen['id'])}

    def summary(self):
        return {'auto': self.auto, 'target': FIRMWARE_VERSION, 'busy': self.current,
                'pending': len(self.pending()), 'last_round': self.last_round}

    def pending(self):
        return [s['id'] for s in self.manager.inventory()[0]
                if s['online'] and self.state_for(s)['available'] and all(self.resolve(s))]

    def set_auto(self, enabled):
        if not isinstance(enabled, bool):
            raise ValueError('Kies aan of uit voor automatisch bijwerken.')
        self.auto = enabled
        self.save()

    def start(self, inbox, host=None):
        if self.busy():
            raise ValueError('Er loopt al een update. Wacht tot die klaar is.')
        screen = self.screen(inbox)
        if not screen:
            raise ValueError('Onbekend scherm. Vernieuw het overzicht.')
        if not screen['online']:
            raise ValueError('Dit scherm is offline; bijwerken kan zodra het terug is.')
        if not self.state_for(screen)['available']:
            raise ValueError('Dit scherm heeft al de nieuwste firmware.')
        if host is not None:
            if not isinstance(host, str) or not re.fullmatch(r'[a-zA-Z0-9][a-zA-Z0-9.-]{0,252}', host.strip()):
                raise ValueError('Vul het IP-adres van dit scherm in.')
            self.hosts[inbox] = host.strip()
            self.save()
        profile, host = self.resolve(screen)
        if not profile:
            raise ValueError('Geen ESPHome-profiel gevonden voor dit scherm. Controleer de naam in de ESPHome-map.')
        if not host:
            raise ValueError('IP-adres onbekend. Vul het eenmalig in; nieuwe firmware meldt het daarna zelf.')
        self.launch([inbox])
        return self.state_for(screen)

    def start_all(self):
        if self.busy():
            raise ValueError('Er loopt al een update. Wacht tot die klaar is.')
        pending = self.pending()
        if not pending:
            raise ValueError('Alle bereikbare schermen zijn al bijgewerkt.')
        self.launch(pending)
        return pending

    def launch(self, inboxes, automatic=False):
        # Mark the first screen busy right away so the page shows progress before the task runs.
        self.current, self.queue, self.phase = inboxes[0], inboxes[1:], 'install'
        self.task = asyncio.create_task(self.run_round(inboxes, automatic))

    def record(self, inbox, state, message):
        self.results[inbox] = {'time': time.time(), 'state': state, 'message': message, 'version': FIRMWARE_VERSION}
        self.save()

    async def run_round(self, inboxes, automatic=False):
        """Update screens one by one; a failure ends the round so a bad build never reaches the next screen."""
        try:
            for index, inbox in enumerate(inboxes):
                self.current, self.queue = inbox, inboxes[index+1:]
                outcome = await self.update_one(inbox)
                if outcome == 'failed':
                    if automatic:
                        await self.notify(f"Automatisch bijwerken is gestopt bij {self.name(inbox)}: "
                                          f"{self.results[inbox]['message']} De overige schermen zijn niet aangeraakt.")
                    break
                if self.queue and outcome == 'success':
                    await asyncio.sleep(self.pause_seconds)
        finally:
            self.current, self.queue, self.phase = None, [], None

    def name(self, inbox):
        screen = self.screen(inbox)
        return screen['name'] if screen else inbox

    async def update_one(self, inbox):
        screen = self.screen(inbox)
        if not screen or not screen['online']:
            self.record(inbox, 'skipped', 'Scherm was offline; volgende keer opnieuw.')
            return 'skipped'
        profile, host = self.resolve(screen)
        if not profile or not host:
            self.record(inbox, 'skipped', 'Profiel of IP-adres onbekend; werk dit scherm handmatig bij.')
            return 'skipped'
        self.phase = 'install'
        try:
            self.manager.firmware.start({'file': profile, 'action': 'install', 'target': host})
            await self.manager.firmware.task
        except ValueError as error:
            self.record(inbox, 'failed', str(error))
            return 'failed'
        if self.manager.firmware.job.get('state') != 'success':
            self.record(inbox, 'failed', 'Bouwen of installeren mislukt; bekijk het log onder Firmware & USB.')
            return 'failed'
        self.phase = 'verify'
        if not await self.wait_for_target(inbox):
            self.record(inbox, 'failed', f'Scherm meldde zich niet met firmware {FIRMWARE_VERSION}; controleer het scherm.')
            return 'failed'
        self.phase = 'settle'
        await asyncio.sleep(self.settle_seconds)
        current = self.screen(inbox)
        if not current or not current['online']:
            self.record(inbox, 'failed', 'Scherm viel weg na de update; controleer het scherm.')
            return 'failed'
        self.record(inbox, 'success', f'Bijgewerkt naar firmware {FIRMWARE_VERSION}.')
        return 'success'

    async def wait_for_target(self, inbox):
        deadline = time.monotonic() + self.verify_timeout
        while time.monotonic() < deadline:
            screen = self.screen(inbox)
            version = parse_version(screen.get('firmware')) if screen else None
            if screen and screen['online'] and version and version >= TARGET:
                return True
            await asyncio.sleep(self.poll_seconds)
        return False

    async def notify(self, message):
        try:
            await self.manager.ha.request('call_service', domain='persistent_notification', service='create',
                                          service_data={'notification_id': 'esp_screens_update', 'title': 'ESP Screens', 'message': message})
        except Exception as error:
            LOG.warning('Melding naar Home Assistant mislukt (%s)', type(error).__name__)

    def due(self, now):
        return self.auto and now.hour in NIGHT_HOURS and self.last_round != now.date().isoformat()

    async def run(self):
        while True:
            await asyncio.sleep(60)
            try:
                if self.busy() or not self.manager.ha.online:
                    continue
                now = datetime.now(getattr(self.manager.ha, 'time_zone', None) or timezone.utc)
                if not self.due(now):
                    continue
                self.last_round = now.date().isoformat()
                self.save()
                pending = self.pending()
                if pending:
                    LOG.info('Nachtelijke firmwareronde: %d scherm(en)', len(pending))
                    self.launch(pending, automatic=True)
            except Exception as error:
                LOG.warning('Nachtelijke updatecontrole overgeslagen (%s)', type(error).__name__)
