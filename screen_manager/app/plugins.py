"""Plugins in the app (docs/PLUGINS.md): which exist, which each screen runs, the file that builds them into a screen's
firmware, and the data their tiles show.

Where plugins come from:
- **the index**, github.com/MaxGramser/tessera-plugins `index.json`: every plugin with its manifest, its texts and its
  README at the commit of its release. Read when the Plugins page opens and at most every six hours, kept in
  /data/plugins/ for when the internet is away.
- **a folder** in Home Assistant's config, `tessera-plugins/<id>/` beside the `esphome/` folder: a plugin someone is
  making. Every build takes what the folder holds now; the app shows it as a test and offers no updates.

The app never runs a plugin's code. It reads the manifest (plugin_manifest.py), writes the screen's plugins file so
ESPHome builds the plugin into that screen, and carries out the plugin's fetches with its own rules (plugin_fetch.py).
"""
import asyncio
import json
import logging
import time
from copy import deepcopy
from pathlib import Path

import yaml

import core
import plugin_fetch
import plugin_manifest as pm
import tile_icons
from i18n import t
from plugin_store import PluginSecrets, PluginStore

LOG = logging.getLogger('plugins')

INDEX_URL = 'https://raw.githubusercontent.com/MaxGramser/tessera-plugins/main/index.json'
INDEX_TTL = 6 * 3600
INDEX_MAX = 2 * 1024 * 1024
INDEX_FORMAT = 1
TESSERA_OWNER = 'https://github.com/MaxGramser/'
FOLDER = 'tessera-plugins'
PLUGIN_ICON = 'F0A66'         # puzzle-outline, for a plugin whose icon is not in Tessera's set
X_BUDGET = 2600               # bytes of a tile's data on the wire: the message carries more and stays under 4 KB
LOOP_SECONDS = 15


def api_text():
    return f'{pm.PLUGIN_API[0]}.{pm.PLUGIN_API[1]}'


def glyph(name):
    return tile_icons.GLYPHS.get(name) or PLUGIN_ICON


def trimmed(data):
    """A tile's data within X_BUDGET: the last items go first."""
    if not isinstance(data, dict):
        return data
    data = dict(data)
    while len(json.dumps(data, separators=(',', ':'), ensure_ascii=False).encode()) > X_BUDGET:
        items = data.get('items')
        if not isinstance(items, list) or not items:
            return {'wait': 'too_large'}
        data['items'] = items[:-1]
    return data


class Entry:
    """One plugin the app knows, from the index, a snapshot of an installed release, or a folder."""

    def __init__(self, raw, translations, readme, source, label, repo=None, path='.', ref=None, folder=None, status='ok'):
        self.raw, self.translations, self.readme = raw, translations, readme
        self.source, self.label, self.repo, self.path, self.ref = source, label, repo, path or '.', ref
        self.folder, self.status = folder, status
        self.manifest = pm.check(raw, translations.get('en'))
        self.id, self.version = self.manifest['id'], self.manifest['version']
        self.texts = pm.texts(self.manifest, translations)

    def text(self, key, language='en'):
        words = self.texts.get(key) or {}
        return words.get(language) or words.get(language.split('-')[0]) or words.get('en') or key

    def tile(self, tile_id):
        return next((tile for tile in self.manifest['tiles'] if tile['id'] == tile_id), None)

    def fetch(self, fetch_id):
        return next((fetch for fetch in self.manifest['fetch'] if fetch['id'] == fetch_id), None)

    def link(self):
        """The plugin's folder on GitHub, at its commit."""
        if not self.repo:
            return ''
        base = self.repo.rstrip('/').removesuffix('.git')
        return base if self.path in ('', '.') else f'{base}/tree/{self.ref or "main"}/{self.path}'

    def snapshot(self):
        return {'raw': self.raw, 'translations': self.translations, 'readme': self.readme, 'source': self.source,
                'label': self.label, 'repo': self.repo, 'path': self.path, 'ref': self.ref}


def read_folder(folder):
    """An Entry from a plugin's folder on disk, or raises."""
    folder = Path(folder)
    raw = yaml.safe_load((folder / 'tessera-plugin.yaml').read_text(encoding='utf-8'))
    translations = {}
    for path in sorted((folder / 'translations').glob('*.json')):
        try:
            translations[path.stem] = json.loads(path.read_text(encoding='utf-8'))
        except ValueError:
            continue
    readme = {}
    for path in sorted(folder.glob('README*.md')):
        language = path.stem.split('.', 1)[1] if '.' in path.stem else 'en'
        readme[language] = path.read_text(encoding='utf-8')[:64 * 1024]
    return Entry(raw, translations, readme, 'folder', 'test', folder=folder)


class Plugins:
    def __init__(self, manager, data_dir, config_dir, session_factory=None):
        self.manager = manager
        self.data = Path(data_dir) / 'plugins'
        self.config = Path(config_dir)
        self.store = PluginStore(Path(data_dir) / 'plugins.json')
        self.secrets = PluginSecrets(Path(data_dir) / 'plugin_secrets.json')
        self.fetcher = plugin_fetch.Fetcher(f'Tessera/{core.FIRMWARE_VERSION} (+plugins)', session_factory)
        self.index = {}             # id -> Entry, from index.json
        self.index_state = {'at': None, 'error': None, 'etag': None, 'checked': 0.0, 'blocked': []}
        self.folders = {}           # id -> Entry, from tessera-plugins/<id>/
        self.folder_errors = {}     # folder name -> what is wrong with it
        self.snapshots = {}         # (id, ref) -> Entry of an installed release
        self.jobs = {}              # inbox -> what its waiting or running build adds and removes, and its state
        self.queue = []             # the screens that wait for a build, in the order they were asked
        self.worker = None
        self.http = None            # the session that reads the index: never the one that holds Home Assistant's token
        self._load_cached_index()
        self.scan_folders()

    # ---- What exists ----

    def _index_file(self):
        return self.data / 'index.json'

    def _load_cached_index(self):
        try:
            cached = json.loads(self._index_file().read_text())
        except (OSError, ValueError):
            return
        self._take_index(cached.get('index') or {}, cached.get('etag'), cached.get('at'))

    def _take_index(self, index, etag=None, at=None):
        if not isinstance(index, dict) or index.get('format') != INDEX_FORMAT:
            self.index_state['error'] = 'format'
            return
        entries = {}
        for item in index.get('plugins') or []:
            try:
                repo = str(item.get('repo') or '')
                label = 'tessera' if item.get('label') == 'tessera' and repo.startswith(TESSERA_OWNER) else 'community'
                release = item.get('release') or {}
                entry = Entry(item['manifest'], item.get('translations') or {}, item.get('readme') or {}, 'index', label,
                              repo=repo, path=item.get('path') or '.', ref=release.get('sha'), status=item.get('status', 'ok'))
            except (pm.ManifestError, KeyError, TypeError, ValueError) as error:
                LOG.warning('Index plugin %s skipped: %s', (item or {}).get('id') if isinstance(item, dict) else '?', error)
                continue
            entries[entry.id] = entry
        self.index = entries
        self.index_state.update(at=at, etag=etag, error=None,
                                blocked=[b for b in index.get('blocked') or [] if isinstance(b, dict)])
        self._know_tiles()

    async def refresh_index(self, force=False):
        """index.json again when it is older than six hours (or `force`), with its ETag; the last one stays offline."""
        if not force and time.monotonic() - self.index_state['checked'] < INDEX_TTL and self.index_state['at']:
            return
        self.index_state['checked'] = time.monotonic()
        import aiohttp
        try:
            if self.http is None or self.http.closed:
                self.http = aiohttp.ClientSession(timeout=aiohttp.ClientTimeout(total=15),
                                                  headers={'User-Agent': self.fetcher.user_agent})
            headers = {'If-None-Match': self.index_state['etag']} if self.index_state['etag'] and not force else {}
            async with self.http.get(INDEX_URL, headers=headers) as response:
                if response.status == 304:
                    return
                response.raise_for_status()
                raw = await response.content.read(INDEX_MAX + 1)
                if len(raw) > INDEX_MAX:
                    raise ValueError('index.json is larger than 2 MB')
                index = json.loads(raw)
                etag = response.headers.get('ETag')
        except Exception as error:
            self.index_state['error'] = type(error).__name__
            LOG.info('Plugin index not read (%s)', error)
            return
        at = time.time()
        self._take_index(index, etag, at)
        try:
            self.data.mkdir(parents=True, exist_ok=True)
            self._index_file().write_text(json.dumps({'index': index, 'etag': etag, 'at': at}))
        except OSError as error:
            LOG.warning('Plugin index not kept (%s)', error)

    def folder_root(self):
        return self.config.parent / FOLDER

    def scan_folders(self):
        """The plugins someone is making, in tessera-plugins/<id>/ beside the ESPHome folder."""
        found, errors = {}, {}
        root = self.folder_root()
        if root.is_dir():
            for folder in sorted(root.iterdir()):
                if not (folder / 'tessera-plugin.yaml').is_file() or folder.is_symlink():
                    continue
                try:
                    entry = read_folder(folder)
                except (OSError, ValueError, yaml.YAMLError, pm.ManifestError) as error:
                    errors[folder.name] = str(error)[:300]
                    continue
                if entry.id != folder.name:
                    errors[folder.name] = f'the folder must be called {entry.id}, as the plugin\'s id'
                    continue
                found[entry.id] = entry
        self.folders, self.folder_errors = found, errors
        self._know_tiles()

    def _snapshot_file(self, plugin, ref):
        return self.data / 'releases' / f'{plugin}-{ref}.json'

    def snapshot_of(self, plugin, ref):
        """The manifest, texts and README of an installed release, kept when it was installed: an older version stays
        known after the index moved on."""
        key = (plugin, ref)
        if key not in self.snapshots:
            try:
                data = json.loads(self._snapshot_file(plugin, ref).read_text())
                self.snapshots[key] = Entry(data['raw'], data['translations'], data.get('readme') or {}, data['source'],
                                            data['label'], repo=data.get('repo'), path=data.get('path'), ref=data.get('ref'))
            except (OSError, ValueError, KeyError, pm.ManifestError):
                return None
        return self.snapshots[key]

    def keep_snapshot(self, entry):
        path = self._snapshot_file(entry.id, entry.ref)
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(entry.snapshot()))
        self.snapshots[(entry.id, entry.ref)] = entry

    def entry_for(self, record):
        """What an installed plugin is: its folder, or the release it is pinned to."""
        if record.get('source') == 'folder':
            return self.folders.get(record['id'])
        return self.snapshot_of(record['id'], record.get('ref')) or (
            self.index.get(record['id']) if self.index.get(record['id']) and self.index[record['id']].ref == record.get('ref') else None)

    def known(self, plugin):
        """Any manifest of this plugin: on a screen first, then a folder, then the index."""
        for records in self.store.everywhere().values():
            for record in records:
                if record['id'] == plugin:
                    entry = self.entry_for(record)
                    if entry:
                        return entry
        return self.folders.get(plugin) or self.index.get(plugin)

    def _know_tiles(self):
        """Every tile type's price for core.tile_cost (the screen's plugin_host::bytes uses the same manifest)."""
        prices = {}
        for entry in [*self.index.values(), *self.folders.values(), *self.snapshots.values()]:
            for tile in entry.manifest['tiles']:
                prices[f'plugin:{entry.id}.{tile["id"]}'] = tile['memory']
        core.PLUGIN_MEMORY.clear()
        core.PLUGIN_MEMORY.update(prices)

    def blocked(self, entry):
        for item in self.index_state['blocked']:
            if item.get('plugin') == entry.id and (entry.version in (item.get('versions') or []) or
                                                   (entry.ref and entry.ref == item.get('sha'))):
                return str(item.get('reason') or 'blocked')
        return None

    # ---- What the editor shows ----

    def editor_plugin(self, entry, language='en'):
        manifest = entry.manifest
        words = lambda key: dict(entry.texts.get(key) or {'en': key}) if key else None
        option = lambda o: {**{k: v for k, v in o.items() if k not in ('label', 'hint', 'choices')},
                            'label': words(o['label']), 'hint': words(o.get('hint')),
                            **({'choices': [{'value': c['value'], 'label': words(c['label'])} for c in o['choices']]}
                               if o.get('choices') else {})}
        gpio = any(item['kind'] == 'gpio' for item in manifest['inputs'])
        return {
            'id': entry.id, 'name': words('name'), 'summary': words('summary'), 'description': words('summary'),
            'icon': glyph(manifest['icon']), 'maintainer': manifest['maintainer'], 'tessera': entry.label == 'tessera',
            'version': entry.version, 'repo': entry.link(), 'license': manifest['license'],
            'kind': 'hardware' if gpio or manifest['boards'] != 'any' else 'behaviour', 'boards': manifest['boards'],
            'requires': {'psram': manifest['requires']['psram']}, 'flash_kb': manifest['flash_kb'],
            'permissions': {'home_assistant': manifest['permissions']['home_assistant_actions'],
                            'network': manifest['permissions']['network'],
                            'read_entities': manifest['permissions']['read_entities']},
            'privacy': manifest.get('privacy'), 'readme': entry.readme,
            'languages': pm.complete_languages(manifest, entry.translations),
            'inputs': [{'id': i['id'], 'kind': i['kind'], 'scope': i['scope'], 'label': words(i['label']),
                        'hint': words(i['hint'])} for i in manifest['inputs']],
            'parts': [{'id': p['id'], 'label': words(p['label']), 'hint': words(p['hint']) or {'en': ''},
                       'flash_kb': p['flash_kb'], 'default': p['default']} for p in manifest['parts']],
            'attributes': manifest['attributes'],
            'adds': {'tiles': [{'id': tile['id'], 'name': words(tile['name']), 'icon': glyph(tile['icon']),
                                'min': tile['min'], 'max': tile['max'], 'memory': tile['memory'],
                                'entity': tile['entity'], 'data': tile.get('data'),
                                'options': [option(o) for o in tile['options']],
                                'example': words(tile.get('example')), 'preview': bool(tile.get('preview'))}
                               for tile in manifest['tiles']]},
            'source': entry.source, 'label': entry.label, 'status': entry.status, 'blocked': self.blocked(entry),
            'fits_api': pm.api_fits(manifest['api']), 'api': manifest['api'],
        }

    def running(self):
        """What each screen's last hello said it runs: {inbox: [{id, version, tiles}]}."""
        out = {}
        for inbox, sender in getattr(self.manager, 'page_senders', {}).items():
            if getattr(sender, 'plugin_api', None):
                out[inbox] = list(getattr(sender, 'plugins', []))
        return out

    def payload(self, language='en'):
        self.scan_folders()
        listed = {}
        for entry in self.index.values():
            listed[entry.id] = entry
        for entry in self.folders.values():
            listed[entry.id] = entry  # a folder of the same id is the one someone is working on
        installed = {}
        for inbox, records in self.store.everywhere().items():
            installed[inbox] = []
            for record in records:
                entry = self.entry_for(record)
                if entry and entry.id not in listed:
                    listed[entry.id] = entry   # an installed release the index no longer lists
                installed[inbox].append({'id': record['id'], 'version': record.get('version', ''),
                                         'source': record.get('source', 'index'), 'ref': record.get('ref'),
                                         'parts': record.get('parts', []), 'values': record.get('values', {}),
                                         'state': record.get('state', 'active'), 'reason': record.get('reason')})
        secrets = {}
        for entry in listed.values():
            for item in entry.manifest['inputs']:
                if item['kind'] == 'secret':
                    secrets.setdefault(entry.id, {})[item['id']] = self.secrets.has(entry.id, item['id'])
        return {'api': api_text(), 'plugins': [self.editor_plugin(entry, language) for entry in listed.values()],
                'installed': installed, 'running': self.running(), 'secrets': secrets,
                'building': {inbox: {k: job[k] for k in ('add', 'remove', 'state') if k in job} for inbox, job in self.jobs.items()},
                'index': {'at': self.index_state['at'], 'error': self.index_state['error']},
                'folders': {'path': str(self.folder_root()), 'errors': self.folder_errors},
                'fetch': self.fetcher.status()}

    # ---- A screen's plugins file ----

    def sidecar(self, inbox):
        """The text of `<name>.plugins.yaml` for this screen's records."""
        packages, components = [], []
        for record in self.store.of(inbox):
            if record.get('state') == 'removing':
                continue
            entry = self.entry_for(record)
            if not entry:
                continue
            values = record.get('values') or {}
            variables = {item['id'].upper(): str(values.get(item['id'], '')) for item in entry.manifest['inputs']
                         if item['kind'] != 'secret' and values.get(item['id'], '') != ''}
            parts = [p['file'] for p in entry.manifest['parts'] if p['id'] in (record.get('parts') or [])]
            name = f'plugin_{entry.id}'
            if record.get('source') == 'folder':
                # Relative to the ESPHome folder, so the app, Device Builder and a shared folder find the same files.
                base = f'../{FOLDER}/{entry.id}'
                include = f'!include {base}/plugin.yaml' if not variables else \
                    '!include {file: %s/plugin.yaml, vars: %s}' % (base, json.dumps(variables))
                packages.append(f'  {name}: {include}')
                for part in parts:
                    packages.append(f'  {name}_{Path(part).stem}: !include {base}/{part}')
                components.append(f'  - source: {{type: local, path: {base}/components}}')
                continue
            folder = '' if entry.path in ('', '.') else entry.path.rstrip('/') + '/'
            packages += [f'  {name}:', f'    url: {entry.repo}', f'    ref: {entry.ref}  # {entry.id} {entry.version}',
                         '    refresh: never', '    files:', f'      - path: {folder}plugin.yaml']
            if variables:
                packages.append(f'        vars: {json.dumps(variables)}')
            packages += [f'      - path: {folder}{part}' for part in parts]
            components += ['  - source:', '      type: git', f'      url: {entry.repo}', f'      ref: {entry.ref}',
                           f'      path: {folder}components', '    refresh: never']
        lines = ['# Written by Tessera. Change plugins in Tessera, not here.']
        if packages:
            lines += ['packages:', *packages, 'external_components:', *components]
        else:
            lines.append('{}')
        return '\n'.join(lines) + '\n'

    def attach_line(self, profile_name):
        return f'packages:\n  tessera_plugins: !include {Path(profile_name).stem}.plugins.yaml'

    # ---- Adding and removing ----

    def fits(self, entry, screen):
        """None when the plugin fits this screen, else the reason (the editor's Misfit words)."""
        manifest = entry.manifest
        if not pm.api_fits(manifest['api']):
            return 'api'
        if manifest['boards'] != 'any' and core.board_of(screen) not in manifest['boards']:
            return 'board'
        if manifest['requires']['psram'] and not core.able(screen, 'pictures') and not screen.get('pictures'):
            return 'psram'
        if self.blocked(entry):
            return 'blocked'
        return None

    async def apply(self, inbox, body):
        """Add and remove plugins on one screen, write its plugins file and build it. `body`:
        {"add": [{"id", "source": "index"|"folder", "parts": [...], "values": {...}, "secrets": {...}}], "remove": [id]}"""
        if not isinstance(body, dict):
            raise ValueError(t('addon.errors.plugins.request'))
        screen = self.manager.screen(inbox)
        if screen is None:
            raise ValueError(t('addon.errors.not_paired'))
        inbox = screen['id']
        profile, host = self.manager.updates.resolve(screen)
        adds = body.get('add') or []
        removes = body.get('remove') or []
        if not isinstance(adds, list) or not isinstance(removes, list) or len(adds) > 8 or len(removes) > 8:
            raise ValueError(t('addon.errors.plugins.request'))
        firmware = self.manager.firmware
        self.scan_folders()
        changes = []
        for item in adds:
            if not isinstance(item, dict) or not isinstance(item.get('id'), str):
                raise ValueError(t('addon.errors.plugins.request'))
            source = item.get('source') or ('folder' if item['id'] in self.folders else 'index')
            entry = (self.folders if source == 'folder' else self.index).get(item['id'])
            if entry is None:
                raise ValueError(t('addon.errors.plugins.unknown', id=item['id']))
            reason = self.fits(entry, screen)
            if reason:
                words = {'api': 'addon.errors.plugins.misfit_api', 'board': 'addon.errors.plugins.misfit_board',
                         'psram': 'addon.errors.plugins.misfit_psram', 'blocked': 'addon.errors.plugins.misfit_blocked'}
                raise ValueError(t(words[reason], name=entry.text('name')))
            for required in entry.manifest['requires']['plugins']:
                if not self.store.get(inbox, required) and not any(a.get('id') == required for a in adds):
                    raise ValueError(t('addon.errors.plugins.requires', name=entry.text('name'), other=required))
            values, secrets = item.get('values') or {}, item.get('secrets') or {}
            if not isinstance(values, dict) or not isinstance(secrets, dict):
                raise ValueError(t('addon.errors.plugins.request'))
            kept = {}
            for spec in entry.manifest['inputs']:
                given = secrets.get(spec['id']) if spec['kind'] == 'secret' else values.get(spec['id'])
                if given is not None and (not isinstance(given, str) or len(given) > 256):
                    raise ValueError(t('addon.errors.plugins.request'))
                if spec['kind'] == 'secret':
                    if given:
                        self.secrets.set(entry.id, spec['id'], given, 'all' if spec['scope'] == 'all' else inbox)
                elif given:
                    kept[spec['id']] = given.strip()
            parts = [p for p in item.get('parts') or [] if p in {x['id'] for x in entry.manifest['parts']}]
            if entry.source == 'index':
                self.keep_snapshot(entry)
            changes.append({'id': entry.id, 'source': entry.source, 'repo': entry.repo, 'path': entry.path,
                            'ref': entry.ref, 'version': entry.version, 'parts': parts, 'values': kept,
                            'consent': {'permissions': pm.permission_hash(entry.manifest), 'at': int(time.time())},
                            'state': 'building' if profile else 'active'})
        for record in changes:
            self.store.put(inbox, record)
        for plugin in removes:
            if isinstance(plugin, str):
                self.store.remove(inbox, plugin)
                if not self.store.in_use(plugin):
                    self.secrets.drop_plugin(plugin)
        self._know_tiles()
        text = self.sidecar(inbox)
        if not profile:
            # A screen built from its own YAML: the app cannot build it, so the page shows the file and the line.
            return {'own_yaml': True, 'file': f'{screen.get("node") or "screen"}.plugins.yaml', 'content': text,
                    'line': self.attach_line(f'{screen.get("node") or "screen"}.yaml')}
        firmware.save_plugins(profile, text)
        if not host:
            return {'written': True, 'built': False}
        # One build at a time, in the order asked: ticking three screens on the Plugins page builds them one after the
        # other. A screen asked again while it waits builds once, with everything asked for it.
        job = self.jobs.get(inbox) if self.jobs.get(inbox, {}).get('state') == 'queued' else None
        added = [c['id'] for c in changes]
        if job:
            job['add'] = sorted(set(job['add']) | set(added))
            job['remove'] = sorted(set(job['remove']) | {r for r in removes if isinstance(r, str)})
        else:
            self.jobs[inbox] = {'add': added, 'remove': [r for r in removes if isinstance(r, str)], 'state': 'queued',
                                'profile': profile, 'host': host, 'asked': int(time.time())}
            self.queue.append(inbox)
        if self.worker is None or self.worker.done():
            self.worker = asyncio.get_running_loop().create_task(self._build_queue())
        self.manager.notify()
        return {'written': True, 'built': True, 'queued': self.queue.index(inbox) if inbox in self.queue else 0}

    async def _build_queue(self):
        """Build the screens that wait, one at a time, each when the app's one build slot is free."""
        firmware = self.manager.firmware
        while self.queue:
            while firmware.task and not firmware.task.done():
                await asyncio.sleep(2)
            inbox = self.queue.pop(0)
            job = self.jobs.get(inbox)
            if not job:
                continue
            job.update(state='building', started=int(time.time()))
            self.manager.notify()
            try:
                firmware.start({'file': job['profile'], 'action': 'install', 'target': job['host']})
                await firmware.task
                ok = (firmware.job or {}).get('state') == 'success'
                reason = None if ok else ((firmware.job or {}).get('error') or 'build')
            except Exception as error:   # refused before it started (a file that went, an address that is wrong)
                ok, reason = False, str(error)[:200]
            for plugin in job['add']:
                if self.store.get(inbox, plugin):
                    self.store.set_state(inbox, plugin, 'active' if ok else 'failed', reason)
            self.jobs.pop(inbox, None)
            self.manager.notify()
            self.manager.ha.changed.set()

    def file_for(self, inbox):
        screen = self.manager.screen(inbox)
        if screen is None:
            raise ValueError(t('addon.errors.not_paired'))
        profile, _ = self.manager.updates.resolve(screen)
        name = profile or f'{screen.get("node") or "screen"}.yaml'
        return {'file': f'{Path(name).stem}.plugins.yaml', 'content': self.sidecar(screen['id']),
                'line': self.attach_line(name), 'own_yaml': not profile}

    def forget_screen(self, inbox):
        for record in self.store.of(inbox):
            self.store.remove(inbox, record['id'])
            if not self.store.in_use(record['id']):
                self.secrets.drop_plugin(record['id'])

    def set_secret(self, plugin, input_id, value):
        entry = self.known(plugin)
        spec = entry and next((i for i in entry.manifest['inputs'] if i['id'] == input_id and i['kind'] == 'secret'), None)
        if not spec or (value is not None and (not isinstance(value, str) or len(value) > 256)):
            raise ValueError(t('addon.errors.plugins.request'))
        self.secrets.set(plugin, input_id, (value or '').strip())
        self.manager.ha.changed.set()
        return {'set': bool(value)}

    # ---- Data for tiles ----

    def values_of(self, entry, tile, options):
        """A tile's options with the manifest's defaults, plus the plugin's inputs for every screen."""
        chosen = {o['id']: o['default'] for o in tile['options'] if 'default' in o}
        for key, value in (options or {}).items():
            spec = next((o for o in tile['options'] if o['id'] == key), None)
            if spec is None:
                continue
            try:
                chosen[key] = pm.option_value(spec, value)
            except pm.ManifestError:
                continue
        return chosen

    async def tile_data(self, entry, tile, chosen, ask=True):
        """What the screen gets for one tile: the mapped answer of its fetch, with "stale" or "wait"."""
        if not tile.get('data'):
            return None
        fetch = entry.fetch(tile['data'])
        secrets = self.secrets.of(entry.id)
        values = {**{k: str(v) for k, v in chosen.items()}}
        try:
            if ask:
                _, cached = await self.fetcher.get(entry.id, entry.manifest, fetch, values, secrets)
            else:
                cached = self.fetcher.peek(entry.id, fetch, values, secrets)
        except plugin_fetch.FetchRefused as error:
            return {'wait': 'not_filled' if 'not filled' in str(error) else 'refused'}
        if not cached or cached['data'] is None:
            return {'wait': 'failed' if cached and cached['error'] else 'asking'}
        try:
            data = plugin_fetch.apply_map(fetch['map'], cached['data'], values)
        except Exception as error:   # an answer of another shape than the map expects
            LOG.warning('plugin %s: the answer did not fit its map (%s)', entry.id, error)
            return {'wait': 'failed'}
        if cached['stale']:
            data['stale'] = True
        return trimmed(data)

    async def tile_message(self, index, tile, ask=True):
        """The state message of a plugin tile (core.state_message's shape): no entity behind it, its options, its data."""
        plugin, tile_id = core.plugin_tile(tile['entity'])
        entry = self.known(plugin)
        kind = entry.tile(tile_id) if entry else None
        options = tile.get('options') or {}
        wire = {key: deepcopy(options[key]) for key in ('size', 'background', 'tap') if key in options}
        icon = options.get('icon')
        if icon in tile_icons.ICONS:
            wire['icon'] = tile_icons.ICONS[icon][0]
        elif kind:
            wire['icon'] = glyph(kind['icon'])
        chosen = self.values_of(entry, kind, options.get('plugin')) if kind else dict(options.get('plugin') or {})
        if chosen:
            wire['plugin'] = chosen
        name = tile.get('name') or (entry.text(kind['name']) if kind else tile_id)
        message = {'v': 1, 'op': 'state', 'i': index, 'entity': tile['entity'], 'name': core.short(name, 80),
                   'state': 'ok', 'a': {}, 'o': wire}
        if kind:
            data = await self.tile_data(entry, kind, chosen, ask)
            if data:
                message['x'] = data
        return message

    async def choices(self, plugin, fetch_id, values):
        """The choices of an option with `options_from`, for the editor's inspector: [{value, label}]."""
        entry = self.known(plugin)
        fetch = entry.fetch(fetch_id) if entry else None
        if not fetch or 'value' not in fetch['map']:
            raise ValueError(t('addon.errors.plugins.request'))
        clean = {k: str(v)[:64] for k, v in (values or {}).items() if isinstance(k, str)}
        try:
            _, cached = await self.fetcher.get(entry.id, entry.manifest, fetch, clean, self.secrets.of(entry.id))
        except plugin_fetch.FetchRefused:
            return {'choices': [], 'wait': 'not_filled'}
        if cached['data'] is None:
            return {'choices': [], 'wait': 'failed', 'error': cached['error']}
        return {'choices': plugin_fetch.apply_choices(fetch['map'], cached['data'], clean)}

    async def preview(self, plugin, tile_id, options):
        """What the editor draws for a plugin tile (it cannot run the plugin's C++): the manifest's `preview` filled in
        from the tile's data, for its first items: [{badge, title, value, at}], `at` a moment the editor counts down to."""
        entry = self.known(plugin)
        kind = entry.tile(tile_id) if entry else None
        if not kind or not kind.get('preview'):
            raise ValueError(t('addon.errors.plugins.request'))
        chosen = self.values_of(entry, kind, options)
        data = await self.tile_data(entry, kind, chosen)
        if not data or 'wait' in data:
            return {'items': [], 'wait': (data or {}).get('wait', 'asking')}
        rows = data.get('items') if isinstance(data.get('items'), list) else [data]
        spec, out = kind['preview'], []
        fill = lambda text, row: pm.PLACEHOLDER.sub(lambda m: '' if row.get(m.group(1)) is None else str(row.get(m.group(1))), text).strip()
        for row in rows[:6]:
            item = {key: fill(spec[key], row) for key in ('badge', 'title', 'value') if key in spec}
            if 'countdown' in spec:
                item['at'] = row.get(spec['countdown'])
            out.append(item)
        return {'items': out, **({'stale': True} if data.get('stale') else {})}

    def plugin_tiles(self):
        """Every plugin tile on a screen's pages: [(inbox, tile)]."""
        out = []
        for inbox, layout in (getattr(self.manager, 'layouts', None) or {}).items():
            for tile in (layout or {}).get('tiles', []):
                if core.plugin_tile(tile.get('entity')):
                    out.append((inbox, tile))
        return out

    async def refresh_data(self):
        """One round of the fetch loop: ask what is due for every plugin tile on a screen, and send the tiles whose
        answer changed."""
        changed = set()
        for _, tile in self.plugin_tiles():
            plugin, tile_id = core.plugin_tile(tile['entity'])
            entry = self.known(plugin)
            kind = entry.tile(tile_id) if entry else None
            if not kind or not kind.get('data'):
                continue
            chosen = self.values_of(entry, kind, (tile.get('options') or {}).get('plugin'))
            fetch = entry.fetch(kind['data'])
            try:
                before = (self.fetcher.peek(entry.id, fetch, {k: str(v) for k, v in chosen.items()},
                                            self.secrets.of(entry.id)) or {}).get('changed')
                _, cached = await self.fetcher.get(entry.id, entry.manifest, fetch, {k: str(v) for k, v in chosen.items()},
                                                   self.secrets.of(entry.id))
            except plugin_fetch.FetchRefused:
                continue
            if cached['changed'] != before:
                changed.add(tile['entity'])
        self.fetcher.forget_unused()
        if changed:
            self.manager.ha.dirty.update(changed)
            self.manager.ha.changed.set()

    async def loop(self):
        while True:
            try:
                if self.plugin_tiles():
                    await self.refresh_data()
            except Exception as error:   # one plugin's trouble never stops the app
                LOG.warning('Plugin data round failed: %s', error)
            await asyncio.sleep(LOOP_SECONDS)
