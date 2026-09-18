import asyncio
import base64
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'screen_manager/app'))
from core import SETTINGS_BESIDE_BLOCK, discover, installation_yaml, packets, state_message, validate_layout, validate_settings

class ProtocolTests(unittest.TestCase):
    def test_unicode_chunks_and_limits(self):
        message = {'v':1,'op':'layout','title':'Living room 🌈', 'entities':['light.lamp']*10}
        parts = packets(message)
        self.assertTrue(all(len(part)<=255 for part in parts))
        self.assertEqual(json.loads(base64.b64decode(''.join(p.split('|')[3] for p in parts))),message)
        self.assertTrue(parts[-1].split('|')[2]=='1')
        with self.assertRaises(ValueError): packets({'huge':'x'*4096})

    def test_layout_rejects_duplicates_unsupported_and_overflow(self):
        for tiles in [[{'entity':'lock.front'}],[{'entity':'light.a'}]*2,[{'entity':f'light.a{i}'} for i in range(49)],[{'entity':'light.a;restart'}]]:
            with self.assertRaises(ValueError): validate_layout({'title':'Home','tiles':tiles})
        self.assertEqual(validate_layout({'title':'Home','tiles':[]})['tiles'],[])

    def test_settings_defaults_ranges_and_old_layout(self):
        self.assertNotIn('settings', validate_layout({'title':'Home','tiles':[]}))
        defaults = validate_settings({})
        self.assertEqual(defaults['standby_seconds'], 600)
        self.assertEqual(defaults['night_start'], 22*60)
        for patch in [{'standby_seconds':True}, {'brightness':0}, {'night_start':1440},
                      {'show_clock':1}, {'standby_seconds':600.5}, {'new_unknown':1},
                      {'brightness':10,'standby_brightness':20}]:
            with self.assertRaises(ValueError): validate_settings(patch)
        for bad in [None, [], 'wrong']:
            with self.assertRaises(ValueError): validate_settings(bad)
        settings = validate_settings({'brightness':40,'standby_brightness':0,'night_brightness':0})
        self.assertEqual(settings['brightness'],40)
        message={'v':1,'op':'layout','title':'Home','entities':[], 'settings':settings}
        self.assertTrue(all(len(p)<=255 for p in packets(message)))

    def test_message_has_only_bounded_display_attributes(self):
        state={'state':'on','attributes':{'friendly_name':'é'*90,'access_token':'private','hs_color':[float('nan'),300], 'brightness':float('inf')}}
        msg=state_message(0,{'entity':'light.a','name':''},{'light.a':state})
        self.assertNotIn('access_token',msg['a']); self.assertNotIn('brightness',msg['a'])
        self.assertLessEqual(len(msg['name'].encode()),80)
        packets(msg)

    def test_discovery_only_enabled_esphome_inboxes(self):
        registry=[{'entity_id':'text.screen','platform':'esphome','original_name':'Tile settings'},
                  {'entity_id':'text.wrong','platform':'template','original_name':'Tile settings'},
                  {'entity_id':'text.disabled','platform':'esphome','original_name':'Tile settings','disabled_by':'user'}]
        screens,_=discover(registry,{'text.screen':{'state':'Synced'}},[],[])
        self.assertEqual([s['id'] for s in screens],['text.screen'])

    def test_unique_credentials_and_remote_profiles(self):
        data={'board':'cyd','name':'living-room','friendly_name':'Living room'}
        first,second=installation_yaml(data),installation_yaml(data)
        self.assertNotEqual(first,second)
        self.assertIn('DEVICE_NAME: "living-room"',first)
        self.assertIn('packages/cyd.yaml',first)
        self.assertIn('!secret wifi_password',first)
        import re
        self.assertEqual(len(base64.b64decode(re.search(r'key: "([^"]+)"',first)[1])),32)
        for name in ['../bad','Bad Name','a\napi:']:
            with self.assertRaises(ValueError): installation_yaml({**data,'name':name})

HAS_AIOHTTP=importlib.util.find_spec('aiohttp') is not None
if HAS_AIOHTTP:
    from server import Manager, create_app
    from aiohttp.test_utils import TestClient, TestServer

@unittest.skipUnless(HAS_AIOHTTP, 'Run using .venv-portal/bin/python for server tests')
class ManagerTests(unittest.IsolatedAsyncioTestCase):
    def setup_manager(self,path):
        class HA:
            online=True
            registry=[{'entity_id':'text.screen','platform':'esphome','original_name':'Tile settings'}]
            devices=[];areas=[]
            states={'text.screen':{'state':'Ready'},'light.a':{'state':'on','attributes':{'friendly_name':'Lamp'}}}
            changed=asyncio.Event()
            def __init__(self): self.messages=[]
            async def send(self,inbox,message,action=None): self.messages.append((inbox,message))
        return Manager(HA(),path)

    async def test_update_restart_preserves_layout(self):
        with tempfile.TemporaryDirectory() as temp:
            path=Path(temp)/'screens.json';m=self.setup_manager(path)
            layout={'title':'My house','tiles':[{'entity':'light.a','name':'Custom name'}]}
            m.save('text.screen',layout)
            fresh=self.setup_manager(path)
            # Stored layouts always carry a grid position; a save without one packs in order.
            self.assertEqual(fresh.layouts['text.screen'],{**layout,'tiles':[{**layout['tiles'][0],'slot':0}]})
            layout=fresh.layouts['text.screen']
            await fresh.sync_one('text.screen',layout)
            self.assertEqual(len(fresh.ha.messages),2)
            await fresh.sync_one('text.screen',layout)
            self.assertEqual(len(fresh.ha.messages),2,'unchanged states should not resend')
            fresh.ha.states['light.a']['state']='off'
            await fresh.sync_one('text.screen',layout)
            self.assertEqual(fresh.ha.messages[-1][1]['state'],'off')
            self.assertEqual(len(fresh.ha.messages),3)
            original=path.read_text();path.write_text('{"version":99,"screens":{}}')
            with self.assertRaises(ValueError): self.setup_manager(path)
            self.assertIn('99',path.read_text(),'unknown schema must never be overwritten')

    async def test_settings_survive_update_and_old_client_save(self):
        with tempfile.TemporaryDirectory() as temp:
            path=Path(temp)/'screens.json'; m=self.setup_manager(path)
            legacy={'title':'Home','tiles':[{'entity':'light.a','name':'My lamp'}]}
            path.write_text(json.dumps({'version':1,'screens':{'text.screen':legacy}}))
            m=self.setup_manager(path)
            stored={**legacy,'tiles':[{**legacy['tiles'][0],'slot':0}]}
            self.assertEqual(m.layouts['text.screen'],stored)
            settings=validate_settings({'brightness':55,'standby_seconds':1200})
            m.save('text.screen',{**legacy,'settings':settings})
            fresh=self.setup_manager(path)
            self.assertEqual(fresh.layouts['text.screen']['settings'],settings)
            fresh.save('text.screen',{**legacy,'title':'Different title'})
            saved=fresh.layouts['text.screen']
            self.assertEqual(saved['settings'],settings)
            self.assertEqual(saved['tiles'],stored['tiles'])
            before=path.read_bytes()
            with self.assertRaises(ValueError): fresh.save('text.screen',{**saved,'settings':{'brightness':0}})
            self.assertEqual(path.read_bytes(),before)
            await fresh.sync_one('text.screen',saved)
            self.assertEqual(fresh.ha.messages[0][1]['settings'],{k:v for k,v in settings.items() if k not in SETTINGS_BESIDE_BLOCK})
            await fresh.sync_one('text.screen',saved)
            self.assertEqual(len(fresh.ha.messages),2)
            await fresh.sync_one('text.screen',saved,True)
            self.assertEqual(fresh.ha.messages[2][1]['settings'],{k:v for k,v in settings.items() if k not in SETTINGS_BESIDE_BLOCK})

    async def test_reorder_aborts_old_batch(self):
        with tempfile.TemporaryDirectory() as temp:
            m=self.setup_manager(Path(temp)/'screens.json');layout={'title':'Home','tiles':[{'entity':'light.a','name':''}]}
            m.save('text.screen',layout)
            original=m.ha.send
            async def interrupt(inbox,message,action=None):
                await original(inbox,message)
                m.save(inbox,{'title':'Empty','tiles':[]})
            m.ha.send=interrupt
            await m.sync_one('text.screen',m.layouts['text.screen'])
            self.assertEqual(len(m.ha.messages),1)
            self.assertNotIn('text.screen',m.sent)

    async def test_http_csrf_validation_download_and_no_token(self):
        with tempfile.TemporaryDirectory() as temp:
            m=self.setup_manager(Path(temp)/'screens.json')
            async with TestClient(TestServer(create_app(m,True))) as client:
                response=await client.get('/api/inventory');inventory=await response.json()
                self.assertNotIn('token',inventory)
                response=await client.put('/api/screens/text.screen',json={'title':'Test','tiles':[]})
                self.assertEqual(response.status,403)
                headers={'X-Screen-CSRF':inventory['csrf']}
                response=await client.put('/api/screens/text.other',headers=headers,json={'title':'Test','tiles':[]})
                self.assertEqual(response.status,400)
                response=await client.put('/api/screens/text.screen',headers=headers,json={'title':'Test','tiles':[]})
                self.assertEqual(response.status,200)
                from firmware import Firmware
                m.firmware=Firmware(Path(temp)/'esphome',Path(temp))
                profile={'board':'guition','name':'screen-new','friendly_name':'New','wifi_ssid':'net','wifi_password':'pw'}
                # A refused USB target leaves no profile behind.
                response=await client.post('/api/firmware/profiles',headers=headers,json={**profile,'target':'/dev/ttyUSB9'})
                self.assertEqual(response.status,400)
                self.assertFalse((Path(temp)/'esphome'/'screen-new.yaml').exists())
                response=await client.post('/api/firmware/profiles',headers=headers,json=profile)
                self.assertEqual(response.status,200)
                self.assertEqual(response.headers['Cache-Control'],'no-store')
                result=await response.json()
                self.assertEqual(result['file'],'screen-new.yaml')
                self.assertEqual(len(base64.b64decode(result['api_key'])),32)
                self.assertNotIn('job',result)
                self.assertIn('packages/guition.yaml',(Path(temp)/'esphome'/'screen-new.yaml').read_text())
                self.assertNotIn('pw',json.dumps((await (await client.get('/api/firmware')).json())['wifi']))
                # Until Home Assistant lists the screen, the page shows the profile as "not yet in HA" with its key.
                pending=(await (await client.get('/api/inventory?light=1')).json())['pending']
                self.assertEqual([(p['file'],p['friendly'],p['installed'],p['api_key']==result['api_key']) for p in pending],[('screen-new.yaml','New',False,True)])

if __name__=='__main__':unittest.main()
