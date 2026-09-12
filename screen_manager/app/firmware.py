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
from core import installation_yaml

class Firmware:
    def __init__(self, root, data):
        self.root, self.data = Path(root).resolve(), Path(data)
        self.job = None
        self.task = None
        self.logs = deque(maxlen=300)
        self.process = None

    def profiles(self):
        if not self.root.exists(): return []
        return [{'file': p.name} for p in sorted(self.root.glob('*.yaml'))
                if p.name != 'secrets.yaml' and p.is_file() and not p.is_symlink()]

    def ports(self):
        return sorted(set(glob.glob('/dev/serial/by-id/*') or glob.glob('/dev/ttyUSB*') + glob.glob('/dev/ttyACM*')))

    def profile(self, name):
        if not isinstance(name,str) or not re.fullmatch(r'[a-zA-Z0-9_-]+\.yaml', name):
            raise ValueError('Kies een bestaand YAML-profiel.')
        p = self.root / name
        if p.is_symlink() or p.resolve().parent != self.root or not p.is_file():
            raise ValueError('Profiel bestaat niet in de ESPHome-map.')
        return p

    def status(self):
        return {'available': bool(shutil.which('esphome')), 'profiles': self.profiles(),
                'ports': self.ports(), 'job': self.job, 'logs': list(self.logs)}

    def create(self, data):
        content = installation_yaml(data)
        self.root.mkdir(parents=True,exist_ok=True)
        # Existing wifi secrets remain untouched. New owners can supply them once.
        secret_path = self.root / 'secrets.yaml'
        if not secret_path.exists():
            ssid, password = data.get('wifi_ssid'), data.get('wifi_password')
            if not isinstance(ssid,str) or not ssid.strip() or not isinstance(password,str):
                raise ValueError('Vul wifi in voor deze eerste installatie.')
            with secret_path.open('x') as f:
                os.chmod(secret_path,0o600)
                yaml.safe_dump({'wifi_ssid':ssid,'wifi_password':password}, f)
        profile = self.root / (data['name']+'.yaml')
        try:
            with profile.open('x') as f:
                os.chmod(profile,0o600);f.write(content)
        except FileExistsError:
            raise ValueError('Deze naam bestaat al. Gebruik het bestaande profiel voor updates.')
        return {'file':profile.name, 'yaml':content}

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
                self.logs.append('ESPHome: '+stage)
                self.process=await asyncio.create_subprocess_exec(*cmd,cwd=self.root,env=env,stdout=asyncio.subprocess.PIPE,stderr=asyncio.subprocess.STDOUT,limit=1024*1024,start_new_session=True)
                async with asyncio.timeout(7200):
                    async for line in self.process.stdout:
                        self.logs.append(self.redact(line.decode(errors='replace').rstrip()))
                    code=await self.process.wait()
                if code: raise RuntimeError('ESPHome '+stage+' mislukt; bekijk het log.')
            self.job['state']='success'; self.logs.append('Geslaagd: '+action)
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
