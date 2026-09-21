import tempfile
import unittest
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
import test_portal
from core import turns_of, validate_settings

class RotationTests(unittest.IsolatedAsyncioTestCase):
    def test_the_turns_a_screen_takes_follow_its_glass(self):
        # A half turn keeps the canvas and the grid, so any glass takes it; a quarter turn only a square one.
        self.assertEqual(turns_of({'width': 480, 'height': 480}), (0, 90, 180, 270))
        self.assertEqual(turns_of({'width': 800, 'height': 480}), (0, 180))
        self.assertEqual(turns_of({'width': 320, 'height': 240}), (0, 180))

    async def test_a_wide_screen_turns_upside_down_but_not_a_quarter(self):
        with tempfile.TemporaryDirectory() as tmp:
            m = test_portal.ManagerTests().setup_manager(Path(tmp) / 'screens.json')
            # An 800 x 480 screen on firmware that turns (0.2.80), telling its own shape.
            m.ha.registry.append({'entity_id': 'sensor.fw', 'device_id': m.ha.registry[0].get('device_id'), 'platform': 'esphome', 'original_name': 'Screen firmware'})
            m.ha.states['sensor.fw'] = {'state': '0.2.80'}
            m.ha.registry.append({'entity_id': 'sensor.shape', 'device_id': m.ha.registry[0].get('device_id'), 'platform': 'esphome', 'original_name': 'Screen layout'})
            m.ha.states['sensor.shape'] = {'state': '800x480 3x3 217dpi standard'}
            m._screens_key = None
            screen = m.screen('text.screen')
            self.assertEqual(m.turns(screen), (0, 180))
            self.assertEqual(m.settings_view(screen)['rotations'], [0, 180])
            self.assertIn('rotation', m.settings_view(screen)['keys'])
            layout = {'title': 'Home', 'tiles': [{'entity': 'light.a'}], 'settings': {'rotation': 180}}
            m.save('text.screen', layout)
            self.assertEqual(m.layouts['text.screen']['settings']['rotation'], 180)
            with self.assertRaisesRegex(ValueError, 'square'):
                m.save('text.screen', {**layout, 'settings': {'rotation': 90}})
            # Firmware from before 0.2.80 on a board that is not a Guition turns not at all.
            m.ha.states['sensor.fw'] = {'state': '0.2.78'}
            m._screens_key = None
            with self.assertRaisesRegex(ValueError, '0.2.80'):
                m.save('text.screen', {**layout, 'settings': {'rotation': 180}})

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
