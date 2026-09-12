"""Guition-specific geometry, hardware isolation and capacitive touch checks."""
from pathlib import Path
import re
import sys
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import verify_gt911
import new_device
SOURCE=(ROOT/'guition-4848s040.yaml').read_text()
VALUES=dict(re.findall(r'^  (\w+): "([^"]*)"',SOURCE,re.M))

class GuitionTests(unittest.TestCase):
    def test_hardware_is_s3_rgb_with_capacitive_touch(self):
        self.assertIn('model: GUITION-4848S040',SOURCE)
        self.assertIn('platform: gt911',SOURCE)
        self.assertIn('mode: octal',SOURCE)
        self.assertNotIn('platform: xpt2046',SOURCE)
        self.assertNotIn('set_raw_correction',SOURCE)
        self.assertEqual(VALUES['TOUCH_MIRROR_X'],'true')
        self.assertEqual(VALUES['TOUCH_MIRROR_Y'],'true')
        self.assertIn('pclk_frequency: 16MHz',SOURCE)
        self.assertIn('execute_from_psram: true',SOURCE)
        self.assertNotIn('id: output_red',SOURCE)
        self.assertEqual(VALUES['DISPLAY_W'],'480')
        self.assertEqual(VALUES['DISPLAY_H'],'480')

    def test_cards_fit_with_gutters_and_reserved_navigation(self):
        v=lambda k:int(VALUES[k])
        rectangles=[]
        for r in range(1,4):
            for c in range(1,3):
                x,y=v(f'GRID_COL{c}_X'),v('SCROLL_Y')+v(f'GRID_ROW{r}_Y')
                w,h=v('TILE_W'),v('TILE_H')
                self.assertGreaterEqual(x,16)
                self.assertLessEqual(x+w,464)
                self.assertLessEqual(y+h,420)
                self.assertGreaterEqual(w*h,3*147*52)
                for xx,yy,ww,hh in rectangles:
                    self.assertFalse(x<xx+ww and x+w>xx and y<yy+hh and y+h>yy)
                rectangles.append((x,y,w,h))
        self.assertEqual(len(rectangles),6)
        self.assertIn('height: 44',SOURCE.split('id: page_next',1)[1][:200])

    def test_all_tiles_guard_events_and_clip_long_titles(self):
        for i in range(1,11):
            self.assertEqual(SOURCE.count(f'cyd::touch_guard.accept(millis(), {i})'),2)
            self.assertRegex(SOURCE,rf'id: t{i}_title\n\s+width: 130\n\s+long_mode: DOT')
        self.assertEqual(VALUES['AUTO_DIM_TIMEOUT'],'600')

    def test_gt911_verification_accepts_pixels_and_rejects_wrong_orientation(self):
        data=dict(screen=[480,480],rotation=0,points=[dict(name=name,samples=[list(xy)]*3) for name,xy in verify_gt911.TARGETS])
        self.assertEqual(len(verify_gt911.verify(data)),5)
        data['rotation']=180
        with self.assertRaises(ValueError):verify_gt911.verify(data)

    def test_gt911_rotations(self):
        self.assertEqual(verify_gt911.screen_point(20,30,90),(30,459))
        self.assertEqual(verify_gt911.screen_point(20,30,270),(449,20))

    def test_initializer_does_not_copy_resistive_calibration(self):
        with tempfile.TemporaryDirectory() as tmp:
            new_device.create(tmp,'wallbox-test','Wallbox Test','guition')
            profile=(Path(tmp)/'guition-device.yaml').read_text()
            self.assertIn('!include guition-4848s040.yaml',profile)
            self.assertIn('DEVICE_NAME: "wallbox-test"',profile)
            self.assertFalse((Path(tmp)/'calibration.yaml').exists())
            self.assertTrue((Path(tmp)/'secrets.yaml').exists())

if __name__=='__main__':unittest.main()
