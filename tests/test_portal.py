import asyncio
import base64
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'screen_manager/app'))
from core import discover, installation_yaml, packets, state_message, validate_layout

class ProtocolTests(unittest.TestCase):
    def test_unicode_chunks_and_limits(self):
        message = {'v':1,'op':'layout','title':'Woonkamer 🌈', 'entities':['light.lamp']*10}
        parts = packets(message)
        self.assertTrue(all(len(part)<=255 for part in parts))
        self.assertEqual(json.loads(base64.b64decode(''.join(p.split('|')[3] for p in parts))),message)
        self.assertTrue(parts[-1].split('|')[2]=='1')
        with self.assertRaises(ValueError): packets({'huge':'x'*4096})

    def test_layout_rejects_duplicates_unsupported_and_overflow(self):
        for tiles in [[{'entity':'lock.front'}],[{'entity':'light.a'}]*2,[{'entity':f'light.a{i}'} for i in range(11)],[{'entity':'light.a;restart'}]]:
            with self.assertRaises(ValueError): validate_layout({'title':'Thuis','tiles':tiles})
        self.assertEqual(validate_layout({'title':'Thuis','tiles':[]})['tiles'],[])

    def test_message_has_only_bounded_display_attributes(self):
        state={'state':'on','attributes':{'friendly_name':'é'*90,'access_token':'private','hs_color':[float('nan'),300], 'brightness':float('inf')}}
        msg=state_message(0,{'entity':'light.a','name':''},{'light.a':state})
        self.assertNotIn('access_token',msg['a']); self.assertNotIn('brightness',msg['a'])
        self.assertLessEqual(len(msg['name'].encode()),80)
        packets(msg)

    def test_discovery_only_enabled_esphome_inboxes(self):
        registry=[{'entity_id':'text.screen','platform':'esphome','original_name':'Tegelinstellingen'},
                  {'entity_id':'text.wrong','platform':'template','original_name':'Tegelinstellingen'},
                  {'entity_id':'text.disabled','platform':'esphome','original_name':'Tegelinstellingen','disabled_by':'user'}]
        screens,_=discover(registry,{'text.screen':{'state':'Gesynchroniseerd'}},[],[])
        self.assertEqual([s['id'] for s in screens],['text.screen'])

    def test_unique_credentials_and_remote_profiles(self):
        data={'board':'cyd','name':'woonkamer','friendly_name':'Woonkamer'}
        first,second=installation_yaml(data),installation_yaml(data)
        self.assertNotEqual(first,second)
        self.assertIn('DEVICE_NAME: "woonkamer"',first)
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
            registry=[{'entity_id':'text.screen','platform':'esphome','original_name':'Tegelinstellingen'}]
            devices=[];areas=[]
            states={'text.screen':{'state':'Ready'},'light.a':{'state':'on','attributes':{'friendly_name':'Lamp'}}}
            changed=asyncio.Event()
            def __init__(self): self.messages=[]
            async def send(self,inbox,message): self.messages.append((inbox,message))
        return Manager(HA(),path)

    async def test_update_restart_preserves_layout(self):
        with tempfile.TemporaryDirectory() as temp:
            path=Path(temp)/'screens.json';m=self.setup_manager(path)
            layout={'title':'Mijn huis','tiles':[{'entity':'light.a','name':'Eigen naam'}]}
            m.save('text.screen',layout)
            fresh=self.setup_manager(path)
            self.assertEqual(fresh.layouts['text.screen'],layout)
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

    async def test_reorder_aborts_old_batch(self):
        with tempfile.TemporaryDirectory() as temp:
            m=self.setup_manager(Path(temp)/'screens.json');layout={'title':'Thuis','tiles':[{'entity':'light.a','name':''}]}
            m.save('text.screen',layout)
            original=m.ha.send
            async def interrupt(inbox,message):
                await original(inbox,message)
                m.save(inbox,{'title':'Leeg','tiles':[]})
            m.ha.send=interrupt
            await m.sync_one('text.screen',layout)
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
                response=await client.post('/api/install',headers=headers,json={'board':'guition','name':'screen-new','friendly_name':'Nieuw'})
                self.assertEqual(response.status,200)
                self.assertEqual(response.headers['Cache-Control'],'no-store')
                self.assertIn('packages/guition.yaml',await response.text())

if __name__=='__main__':unittest.main()
