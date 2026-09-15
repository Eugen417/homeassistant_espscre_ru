import tempfile
import unittest
from pathlib import Path
import test_portal
from core import validate_settings

class RotationTests(unittest.IsolatedAsyncioTestCase):
    def test_only_quarter_turns(self):
        for angle in (0,90,180,270):
            self.assertEqual(validate_settings({'rotation':angle})['rotation'],angle)
        for angle in (-90,45,360,True,90.0,'90'):
            with self.assertRaises(ValueError):validate_settings({'rotation':angle})

    async def test_legacy_wire_and_saved_rotation_survive_old_browser(self):
        with tempfile.TemporaryDirectory() as tmp:
            m=test_portal.ManagerTests().setup_manager(Path(tmp)/'screens.json')
            layout={'title':'Home','tiles':[{'entity':'light.a'}],'settings':{'rotation':90}}
            with self.assertRaises(ValueError):m.save('text.screen',layout)
            m.ha.registry[0]['device_id']='guition'
            m.ha.registry.append({'entity_id':'sensor.board','device_id':'guition','platform':'esphome','original_name':'Guition screen type'})
            # Capability stays discoverable with the panel offline or renamed.
            m.ha.states['sensor.board']={'state':'unavailable'}
            m.save('text.screen',layout)
            await m.sync_one('text.screen',m.layouts['text.screen'])
            wire=m.ha.messages[0][1]
            self.assertEqual(wire['rotation'],90)
            self.assertEqual(len(wire['settings']),11)
            self.assertNotIn('rotation',wire['settings'])
            m.save('text.screen',{'title':'Different','tiles':layout['tiles'],'settings':{'brightness':80}})
            self.assertEqual(m.layouts['text.screen']['settings']['rotation'],90)
            fresh=test_portal.ManagerTests().setup_manager(m.path)
            self.assertEqual(fresh.layouts['text.screen']['settings']['rotation'],90)
