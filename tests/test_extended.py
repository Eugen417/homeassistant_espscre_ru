import asyncio
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'screen_manager/app'))
from core import validate_layout, state_message, packets
from firmware import Firmware
from test_portal import ManagerTests

class ExtendedTests(unittest.TestCase):
 def test_options_validation_and_new_domains(self):
  for domain in ['sensor','number','input_number','select','weather','media_player']:
   layout=validate_layout({'title':'Test','tiles':[{'entity':domain+'.x','options':{'display':'watch'}}]})
   self.assertEqual(layout['tiles'][0]['options']['display'],'watch')
  for options in [{'inline':'slider'},{'tap':'toggle'},{'history_hours':True},{'history_hours':100},{'arbitrary':'run'}]:
   with self.assertRaises(ValueError):validate_layout({'title':'Test','tiles':[{'entity':'sensor.x','options':options}]})
  with self.assertRaises(ValueError):validate_layout({'title':'Test','tiles':[{'entity':'light.x','options':{'display':'watch','inline':'slider'}}]})
 def test_attributes_and_history_fit_protocol(self):
  tile={'entity':'sensor.a','name':'Temperatuur','options':{'history_hours':24,'display':'watch'}}
  msg=state_message(0,tile,{'sensor.a':{'state':'21','attributes':{'unit_of_measurement':'°C','password':'hidden'}}})
  msg['history']={'hours':24,'values':[None,21.5]*12}
  self.assertNotIn('password',msg['a']);self.assertEqual(msg['o'],tile['options']);self.assertTrue(all(len(p)<=255 for p in packets(msg)))
 def test_firmware_profiles_no_overwrite_or_traversal(self):
  with tempfile.TemporaryDirectory() as tmp:
   f=Firmware(Path(tmp)/'config',Path(tmp)/'data');data={'board':'cyd','name':'test','friendly_name':'Test','wifi_ssid':'local-wifi','wifi_password':'private-wifi'}
   result=f.create(data);before=f.profile('test.yaml').read_text();secrets=(f.root/'secrets.yaml').read_text()
   with self.assertRaises(ValueError):f.create(data)
   self.assertEqual(before,f.profile('test.yaml').read_text());self.assertEqual(secrets,(f.root/'secrets.yaml').read_text())
   for value in ['../secrets.yaml','/etc/passwd','test.yaml;echo']:
    with self.assertRaises(ValueError):f.profile(value)
   (f.root/'linked.yaml').symlink_to(f.profile('test.yaml'))
   with self.assertRaises(ValueError):f.profile('linked.yaml')
   self.assertEqual(f.profiles(),[{'file':'test.yaml'}])

class FirmwareJobs(unittest.IsolatedAsyncioTestCase):
 async def test_cli_arguments_redaction_and_busy(self):
  with tempfile.TemporaryDirectory() as tmp:
   f=Firmware(tmp,tmp);f.create({'board':'guition','name':'test','friendly_name':'Test','wifi_ssid':'homewifi','wifi_password':'secretwifi'})
   async def waiting(*args):await asyncio.sleep(20)
   with patch('firmware.shutil.which',return_value='/bin/esphome'),patch.object(f,'run',side_effect=waiting):
    with self.assertRaises(ValueError):f.start({'file':'test.yaml','action':'install','target':'1.2.3.4;evil'})
    with self.assertRaises(ValueError):f.start({'file':'test.yaml','action':'install','target':'/dev/unknown'})
    f.start({'file':'test.yaml','action':'validate'})
    self.assertNotIn('secretwifi',f.redact('Using secretwifi on homewifi'))
    with self.assertRaises(ValueError):f.start({'file':'test.yaml','action':'build'})
    f.task.cancel()
    try:await f.task
    except asyncio.CancelledError:pass
 async def test_old_browser_preserves_new_tile_options(self):
  with tempfile.TemporaryDirectory() as tmp:
   m=ManagerTests().setup_manager(Path(tmp)/'screens.json')
   m.save('text.screen',{'title':'Home','tiles':[{'entity':'light.a','name':'Lamp','options':{'inline':'slider'}}]})
   m.save('text.screen',{'title':'Nieuw','tiles':[{'entity':'light.a','name':'Lamp'}]})
   self.assertEqual(m.layouts['text.screen']['tiles'][0]['options'],{'inline':'slider'})
