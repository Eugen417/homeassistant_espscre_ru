"""Serialised ESPHome CLI jobs. YAML/secrets stay local; no shell or arbitrary commands."""
import asyncio
from collections import deque
import glob
import json
import os
from pathlib import Path
import re
import shutil
import signal
import time
import yaml
from core import REPO, installation_yaml

# libyaml parses a 300 KB profile roughly ten times faster than the pure-Python loader.
class LenientLoader(yaml.CSafeLoader if getattr(yaml, '__with_libyaml__', False) else yaml.SafeLoader):
    """Reads profile metadata without resolving !secret or !include."""
LenientLoader.add_multi_constructor('!', lambda loader, suffix, node: None)

def profile_meta(text):
    """{'node', 'friendly'} from profile YAML, or None when it has no esphome block."""
    data = yaml.load(text, Loader=LenientLoader)
    block = data.get('esphome') if isinstance(data, dict) else None
    if not isinstance(block, dict):
        return None
    substitutions = data.get('substitutions') if isinstance(data.get('substitutions'), dict) else {}
    def resolve(value):
        if isinstance(value, str) and value.startswith('${') and value.endswith('}'):
            value = substitutions.get(value[2:-1])
        return value if isinstance(value, str) else None
    # 'screen': the profile pulls this project's board package, so it is one of ours (not any ESPHome device).
    packages = data.get('packages') if isinstance(data.get('packages'), dict) else {}
    ours = any(isinstance(entry, dict) and REPO in str(entry.get('url', '')) for entry in packages.values())
    api = data.get('api') if isinstance(data.get('api'), dict) else {}
    encryption = api.get('encryption') if isinstance(api.get('encryption'), dict) else {}
    key = encryption.get('key')
    return {'node': resolve(block.get('name')), 'friendly': resolve(block.get('friendly_name')),
            'screen': ours, 'api_key': key if isinstance(key, str) else None}

class Firmware:
    def __init__(self, root, data):
        self.root, self.data = Path(root).resolve(), Path(data)
        self.job = None
        self.task = None
        self.logs = deque(maxlen=300)
        self.process = None
        self.installed = set()  # profiles this process flashed successfully; the page nudges pairing for them
        self._names = {}  # file -> (stat signature, meta or None); parsed only when the file changes

    def profiles(self):
        if not self.root.exists(): return []
        return [{'file': p.name} for p in sorted(self.root.glob('*.yaml'))
                if p.name != 'secrets.yaml' and p.is_file() and not p.is_symlink()]

    def profile_names(self):
        """{file: {'node': esphome name, 'friendly': friendly name}} for every readable profile.

        Parsing is cached per file on inode, mtime and size, so a request only pays for changed
        profiles. The dict is rebuilt from the current directory listing, so deleted files drop out.
        """
        found, cache = {}, {}
        for entry in self.profiles():
            path = self.root / entry['file']
            try:
                st = path.stat()
                key = (st.st_ino, st.st_mtime_ns, st.st_size)
                cached = self._names.get(entry['file'])
                if cached and cached[0] == key:
                    meta = cached[1]
                else:
                    meta = profile_meta(path.read_text())
            except (OSError, yaml.YAMLError, UnicodeError):
                continue
            cache[entry['file']] = (key, meta)
            if meta:
                found[entry['file']] = meta
        self._names = cache
        return found

    def ports(self):
        return sorted(set(glob.glob('/dev/serial/by-id/*') or glob.glob('/dev/ttyUSB*') + glob.glob('/dev/ttyACM*')))

    def profile(self, name):
        if not isinstance(name,str) or not re.fullmatch(r'[a-zA-Z0-9_-]+\.yaml', name):
            raise ValueError('Kies een bestaand YAML-profiel.')
        p = self.root / name
        if p.is_symlink() or p.resolve().parent != self.root or not p.is_file():
            raise ValueError('Profiel bestaat niet in de ESPHome-map.')
        return p

    def wifi_status(self):
        # Return availability only; never send secret values to the browser.
        path = self.root / 'secrets.yaml'
        if not path.exists():
            return {'state': 'new', 'missing': ['wifi_ssid', 'wifi_password']}
        try:
            values = yaml.safe_load(path.read_text())
            if not isinstance(values, dict):
                return {'state': 'invalid'}
            missing = [key for key in ('wifi_ssid', 'wifi_password')
                       if not isinstance(values.get(key), str)]
            if isinstance(values.get('wifi_ssid'), str) and not values['wifi_ssid'].strip():
                missing.append('wifi_ssid')
            return {'state': 'missing' if missing else 'ready', 'missing': missing}
        except (OSError, yaml.YAMLError, UnicodeError):
            return {'state': 'invalid'}

    def status(self):
        return {'available': bool(shutil.which('esphome')), 'profiles': self.profiles(),
                'ports': self.ports(), 'job': self.job, 'logs': list(self.logs), 'wifi': self.wifi_status()}

    def store_wifi(self, data):
        """Put wifi_ssid/wifi_password in secrets.yaml when they are missing. Existing values and
        every other secret stay as they are; the browser never sees stored values."""
        wifi = self.wifi_status()
        if wifi['state'] == 'ready':
            return
        if wifi['state'] == 'invalid':
            raise ValueError('secrets.yaml in de ESPHome-map is geen geldige YAML. Herstel het bestand eerst; het wordt niet overschreven.')
        values = {}
        for key in wifi['missing']:
            value = data.get(key)
            if not isinstance(value, str) or (key == 'wifi_ssid' and not value.strip()):
                raise ValueError('Vul de wifi-naam en het wachtwoord in; ze worden in ESPHome secrets.yaml bewaard.')
            values[key] = value
        path = self.root / 'secrets.yaml'
        if wifi['state'] == 'new':
            with path.open('x') as f:
                os.chmod(path, 0o600)
                yaml.safe_dump(values, f, width=4096)
            return
        # An existing file with missing keys: replace only that key's own line, or append it.
        text = path.read_text()
        for key, value in values.items():
            line = yaml.safe_dump({key: value}, width=4096).strip()
            pattern = re.compile(rf'^{key}\s*:.*$', re.M)
            if pattern.search(text):
                text = pattern.sub(lambda match: line, text, count=1)
            else:
                text = text + ('' if not text or text.endswith('\n') else '\n') + line + '\n'
        check = yaml.safe_load(text)
        if not isinstance(check, dict) or any(check.get(key) != value for key, value in values.items()):
            raise ValueError('Wifi kon niet in secrets.yaml worden gezet. Vul wifi_ssid en wifi_password daar zelf in.')
        path.write_text(text)

    def create(self, data):
        """Write the device profile (unique API/OTA keys) and any missing wifi secrets."""
        content = installation_yaml(data)
        self.root.mkdir(parents=True,exist_ok=True)
        profile = self.root / (data['name']+'.yaml')
        if profile.exists() or profile.is_symlink():
            raise ValueError('Deze naam bestaat al. Gebruik het bestaande profiel voor updates.')
        self.store_wifi(data)
        try:
            with profile.open('x') as f:
                os.chmod(profile,0o600);f.write(content)
        except FileExistsError:
            raise ValueError('Deze naam bestaat al. Gebruik het bestaande profiel voor updates.')
        key = yaml.load(content, Loader=LenientLoader)['api']['encryption']['key']
        # The key is what Home Assistant asks for when pairing; the page shows it once.
        return {'file': profile.name, 'node': data['name'], 'api_key': key}

    def install(self, data):
        """Profile plus, when a USB port is chosen, the build and flash in one go.

        The port and the job slot are checked before anything is written, so a refused
        install leaves no half-made profile behind."""
        target = data.get('target') or ''
        if target:
            if not isinstance(target, str) or target not in self.ports():
                raise ValueError('Kies de aangesloten USB-poort uit de lijst.')
            if self.task and not self.task.done():
                raise ValueError('Er loopt al een build of installatie. Wacht tot die klaar is.')
            if not shutil.which('esphome'):
                raise ValueError('ESPHome CLI ontbreekt in deze installatie.')
        result = self.create(data)
        if target:
            result['job'] = self.start({'file': result['file'], 'action': 'install', 'target': target})
        return result

    def redact(self, text):
        text = re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]', '', text)
        for value in self._secret_values:
            if len(value) >= 3: text=text.replace(value,'[redacted]')
        return re.sub(r'(?i)((?:password|encryption.key|token|ssid)\s*[:=]\s*).+',r'\1[redacted]',text)[:1500]

    def start(self, data):
        if self.task and not self.task.done(): raise ValueError('Er loopt al een build of installatie.')
        profile = self.profile(data.get('file'))
        action = data.get('action')
        if action not in ('validate','build','install'): raise ValueError('Onbekende firmwareactie.')
        if not shutil.which('esphome'): raise ValueError('ESPHome CLI ontbreekt in deze installatie.')
        target = data.get('target','')
        if action == 'install':
            if target.startswith('/dev/'):
                if target not in self.ports(): raise ValueError('Kies de aangesloten USB-poort uit de lijst.')
            elif not re.fullmatch(r'[a-zA-Z0-9][a-zA-Z0-9.-]{0,252}',target):
                raise ValueError('Vul het IP-adres of de hostnaam van het bedoelde scherm in.')
        self._secret_values=set()
        # Collect scalar literals, including !secret values, without executing YAML tags.
        def collect(node):
            if isinstance(node,yaml.ScalarNode):
                if node.value: self._secret_values.add(node.value)
            elif isinstance(node,yaml.SequenceNode):
                for child in node.value: collect(child)
            elif isinstance(node,yaml.MappingNode):
                for key,value in node.value:
                    if key.value in ('password','key','ssid','token') or profile.name=='secrets.yaml':collect(value)
                    elif isinstance(value,(yaml.MappingNode,yaml.SequenceNode)):collect(value)
        collect(yaml.compose(profile.read_text()))
        secret_file=self.root/'secrets.yaml'
        if secret_file.exists():
            node=yaml.compose(secret_file.read_text())
            if isinstance(node,yaml.MappingNode):
                for _,v in node.value:collect(v)
        self.logs.clear()
        self.job={'file':profile.name,'action':action,'target':target,'state':'running','started':time.time()}
        self.task=asyncio.create_task(self.run(profile,action,target))
        return dict(self.job)

    async def run(self, profile, action, target):
        env={**os.environ, 'PLATFORMIO_CORE_DIR':str(self.data/'platformio'), 'ESPHOME_BUILD_PATH':str(self.data/'build'/profile.stem), 'NO_COLOR':'1'}
        try:
            stages = ['config'] if action=='validate' else ['compile'] + (['upload'] if action=='install' else [])
            for stage in stages:
                cmd=['esphome']+(['--quiet'] if stage=='config' else [])+[stage,str(profile)]
                if stage=='upload': cmd += ['--device',target]
                self.job['stage']=stage
                self.logs.append('ESPHome: '+stage)
                self.process=await asyncio.create_subprocess_exec(*cmd,cwd=self.root,env=env,stdout=asyncio.subprocess.PIPE,stderr=asyncio.subprocess.STDOUT,limit=1024*1024,start_new_session=True)
                async with asyncio.timeout(7200):
                    async for line in self.process.stdout:
                        self.logs.append(self.redact(line.decode(errors='replace').rstrip()))
                    code=await self.process.wait()
                if code: raise RuntimeError('ESPHome '+stage+' mislukt; bekijk het log.')
            self.job['state']='success'; self.logs.append('Geslaagd: '+action)
            if action=='install': self.installed.add(profile.name)
        except asyncio.CancelledError:
            self.job['state']='interrupted'
            raise
        except Exception as error:
            self.job['state']='failed';self.logs.append(self.redact(str(error)))
        finally:
            if self.process and self.process.returncode is None:
                os.killpg(self.process.pid,signal.SIGTERM)
                try: await asyncio.wait_for(self.process.wait(),10)
                except TimeoutError: os.killpg(self.process.pid,signal.SIGKILL);await self.process.wait()
            self.process=None
            self.job['finished']=time.time()
