"""The screensaver (app 0.4.48, firmware 0.29.0): what a screen shows in standby instead of its dimmed tiles.

- Each screen keeps its own choice, by its Home Assistant device, in screensavers.json beside the layouts: a media
  player, a camera and the order of the steps, with the clock as a step of its own. The layouts' storage is untouched.
- The app picks the first step that is on and available: a player that plays with a cover, a camera Home Assistant has,
  the clock always. A board without pictures has the clock alone.
- A screen whose hello lists `screensaver` hears what to show in one small message, when it changes and once in every
  session; it may then load that player's cover and that camera, which are on none of its tiles.
"""
from manager_fixtures import with_screen_grid, seed_layout
import asyncio
import importlib.util
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'screen_manager/app'))
import screen_saver  # noqa: E402
from core import SCREENSAVER_MIN_FIRMWARE, media_extras, short, validate_layout  # noqa: E402

HAS_AIOHTTP = importlib.util.find_spec('aiohttp') is not None
if HAS_AIOHTTP:
    from server import Manager

PLAYER, CAMERA = 'media_player.living_room', 'camera.front_door'
PLAYING = {'state': 'playing', 'attributes': {'friendly_name': 'Living room', 'media_title': 'Song', 'media_artist': 'Band',
                                              'media_duration': 200, 'entity_picture': '/api/media_player_proxy/x?cache=1'}}
DOOR = {'state': 'idle', 'attributes': {'friendly_name': 'Front door'}}
CHOICE = {'show': True, 'media': PLAYER, 'camera': CAMERA, 'order': ['media', 'camera', 'clock'], 'off': [], 'weather': 'auto'}


class Choice(unittest.TestCase):
    def test_a_choice_is_checked_and_filled_in(self):
        self.assertEqual(screen_saver.validate({}), screen_saver.DEFAULT)
        self.assertEqual(screen_saver.validate({'show': True})['order'], ['media', 'camera', 'clock'])
        for wrong in ({'show': 'yes'}, {'media': 'camera.front'}, {'camera': 'media_player.x'}, {'media': 'Media Player'},
                      {'order': ['media', 'clock']}, {'order': ['media', 'media', 'clock']}, {'off': ['tv']}, {'colour': 1}, None):
            with self.assertRaises(ValueError, msg=wrong):
                screen_saver.validate(wrong)
        # An image entity is a camera too (a doorbell's last snapshot), as on an alert.
        self.assertEqual(screen_saver.validate({'camera': 'image.doorbell'})['camera'], 'image.doorbell')
        # What is off keeps the order's order.
        self.assertEqual(screen_saver.validate({'order': ['clock', 'camera', 'media'], 'off': ['media', 'clock']})['off'], ['clock', 'media'])

    def test_the_first_step_that_is_available(self):
        states = {PLAYER: PLAYING, CAMERA: DOOR}
        self.assertEqual(screen_saver.pick(CHOICE, states, True), 'media')
        paused = {**states, PLAYER: {**PLAYING, 'state': 'paused'}}
        self.assertEqual(screen_saver.pick(CHOICE, paused, True), 'camera')
        bare = {**states, PLAYER: {'state': 'playing', 'attributes': {'media_title': 'Radio'}}}
        self.assertEqual(screen_saver.pick(CHOICE, bare, True), 'camera', 'a player without a cover is not the screensaver')
        gone = {**paused, CAMERA: {'state': 'unavailable'}}
        self.assertEqual(screen_saver.pick(CHOICE, gone, True), 'clock')
        self.assertEqual(screen_saver.pick({**CHOICE, 'off': ['clock']}, gone, True), '', 'nothing available: dark as before')
        self.assertEqual(screen_saver.pick({**CHOICE, 'order': ['clock', 'media', 'camera']}, states, True), 'clock')
        self.assertEqual(screen_saver.pick({**CHOICE, 'show': False}, states, True), '')
        # A board without pictures: the clock alone.
        self.assertEqual(screen_saver.pick(CHOICE, states, False), 'clock')

    def test_the_message_carries_what_the_screen_draws(self):
        states = {PLAYER: PLAYING, CAMERA: DOOR}
        media = screen_saver.message(CHOICE, states, True, short, media_extras, lambda e, a: '102030,102030')
        self.assertEqual((media['op'], media['k'], media['e'], media['n'], media['t']), ('saver', 'media', PLAYER, 'Living room', 'Song'))
        self.assertEqual((media['x']['artist'], media['x']['g']), ('Band', '102030,102030'))
        self.assertIn('pic', media['x'])
        # Where the track is shows nowhere: a new position is no new message.
        self.assertEqual(set(media['x']), {'artist', 'pic', 'g'})
        camera = screen_saver.message({**CHOICE, 'off': ['media']}, states, True, short, media_extras)
        self.assertEqual(camera, {'op': 'saver', 'k': 'camera', 'e': CAMERA, 'n': 'Front door'})
        self.assertEqual(screen_saver.message(CHOICE, states, False, short, media_extras), {'op': 'saver', 'k': 'clock'})
        self.assertEqual(screen_saver.message({**CHOICE, 'show': False}, states, True, short, media_extras), {'op': 'saver', 'k': ''})

    def test_the_clock_shows_the_outside_temperature(self):
        """App 0.4.52: under the clock the outside temperature, whole degrees in Home Assistant's own unit, from its
        first weather entity unless the owner chose one, or none."""
        home = {'state': 'cloudy', 'attributes': {'temperature': 21.6, 'temperature_unit': '°C', 'friendly_name': 'Home'}}
        north = {'state': 'sunny', 'attributes': {'temperature': 70.4, 'temperature_unit': '°F'}}
        states = {'weather.home': home, 'weather.north': north, 'weather.broken': {'state': 'unavailable', 'attributes': {}}}
        clock = {**CHOICE, 'off': ['media', 'camera']}
        self.assertEqual(screen_saver.message(clock, states, True, short, media_extras), {'op': 'saver', 'k': 'clock', 'w': '22°'})
        self.assertEqual(screen_saver.message({**clock, 'weather': 'weather.north'}, states, True, short, media_extras)['w'], '70°')
        self.assertNotIn('w', screen_saver.message({**clock, 'weather': ''}, states, True, short, media_extras))
        self.assertNotIn('w', screen_saver.message({**clock, 'weather': 'weather.broken'}, states, True, short, media_extras))
        # The forecast Home Assistant made for its home comes first, whatever its id sorts as.
        self.assertEqual(screen_saver.weather_entity(clock, {**states, 'weather.forecast_home': home}), 'weather.forecast_home')
        # Without a weather entity in Home Assistant there is no temperature, and minus zero is zero.
        self.assertEqual(screen_saver.message(clock, {}, True, short, media_extras), {'op': 'saver', 'k': 'clock'})
        self.assertEqual(screen_saver.temperature(clock, {'weather.cold': {'state': 'snowy', 'attributes': {'temperature': -0.4}}}), '0°')
        self.assertEqual(screen_saver.temperature(clock, {'weather.cold': {'state': 'snowy', 'attributes': {'temperature': -3.6}}}), '-4°')
        # A picture step carries no temperature, and the app follows the weather entity so a new value goes out.
        self.assertNotIn('w', screen_saver.message(CHOICE, {**states, PLAYER: PLAYING}, True, short, media_extras))
        self.assertIn('weather.home', screen_saver.entities(clock, states))
        self.assertNotIn('weather.home', screen_saver.entities({**clock, 'show': False}, states))
        self.assertEqual(screen_saver.validate({'weather': 'weather.home'})['weather'], 'weather.home')
        for wrong in ('sensor.outside', 'Weather', 7):
            with self.assertRaises(ValueError):
                screen_saver.validate({'weather': wrong})
        # A choice kept before 0.4.52 reads as the automatic temperature.
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'screensavers.json'
            path.write_text('{"version": 1, "screens": {"d1": {"show": true, "media": "", "camera": "", "order": ["media", "camera", "clock"], "off": []}}}')
            self.assertEqual(screen_saver.ScreenSavers(path).get('d1')['weather'], 'auto')

    def test_kept_per_device_beside_the_layouts(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'screensavers.json'
            savers = screen_saver.ScreenSavers(path)
            self.assertEqual(savers.get('d1'), screen_saver.DEFAULT)
            savers.set('d1', CHOICE)
            self.assertEqual(screen_saver.ScreenSavers(path).get('d1'), CHOICE)
            self.assertEqual(json.loads(path.read_text())['version'], 1)
            savers.get('d1')['order'].reverse()
            self.assertEqual(savers.get('d1')['order'], CHOICE['order'], 'a copy, never the kept choice')
            savers.set('d1', {})
            self.assertNotIn('d1', screen_saver.ScreenSavers(path).choices, 'the default is not written down')
            path.write_text('{"version": 1, "screens": {"d2": {"show": "x"}}}')
            self.assertEqual(screen_saver.ScreenSavers(path).get('d2'), screen_saver.DEFAULT)


HAS_PIL = importlib.util.find_spec('PIL') is not None


@unittest.skipUnless(HAS_PIL, 'Run using .venv-portal/bin/python for picture tests')
class Picture(unittest.TestCase):
    """camera_feed.encode_saver: one picture of the screen's whole box, a little darker everywhere."""

    def picture(self, size, colour=(200, 200, 200)):
        from PIL import Image
        out = io.BytesIO()
        Image.new('RGB', size, colour).save(out, 'JPEG')
        return out.getvalue()

    def open(self, bmp):
        from PIL import Image
        return Image.open(io.BytesIO(bmp)).convert('RGB')

    def test_a_camera_fills_the_glass_darkened(self):
        import camera_feed
        image = self.open(camera_feed.encode_saver(self.picture((1920, 1080)), (480, 480), 'camera'))
        self.assertEqual(image.size, (480, 480))
        # Cut to the glass, not fitted into it: no black bars, and every pixel a little darker, the same everywhere.
        for point in ((2, 2), (240, 240), (477, 477)):
            self.assertTrue(120 <= image.getpixel(point)[0] <= 160, image.getpixel(point))

    def test_a_cover_fills_square_glass_and_stands_beside_its_colour_on_long_glass(self):
        import camera_feed
        cover = self.picture((640, 640), (240, 240, 240))
        square = self.open(camera_feed.encode_saver(cover, (480, 480), 'media', 0x204060))
        self.assertGreater(square.getpixel((470, 240))[0], 140)
        wide = self.open(camera_feed.encode_saver(cover, (1024, 600), 'media', 0x204060))
        self.assertEqual(wide.size, (1024, 600))
        self.assertGreater(wide.getpixel((300, 300))[0], 140, 'the cover at the left, the full height')
        right = wide.getpixel((900, 300))
        self.assertTrue(abs(right[2] - 0x60 * 0.7) < 12 and right[0] < 40, right)
        tall = self.open(camera_feed.encode_saver(cover, (600, 1024), 'media', 0x204060))
        self.assertGreater(tall.getpixel((300, 300))[0], 140, 'the cover at the top, the full width')
        self.assertLess(tall.getpixel((300, 900))[0], 40)

    def test_the_firmware_lays_its_words_by_the_same_rule(self):
        import camera_feed
        header = (ROOT / 'components/smart_display/saver_view.h').read_text()
        self.assertEqual(camera_feed.SAVER_SQUARE, (5, 4))
        self.assertIn('width * 4 <= height * 5 && height * 4 <= width * 5', header)
        for box, shape in (((480, 480), 'fill'), ((500, 400), 'fill'), ((501, 400), 'side'), ((400, 501), 'top'), ((1024, 600), 'side')):
            self.assertEqual(camera_feed.saver_shape(box), shape, box)


@unittest.skipUnless(HAS_AIOHTTP, 'Run using .venv-portal/bin/python for server tests')
class TheApp(unittest.IsolatedAsyncioTestCase):
    def ha(self, firmware='0.29.0'):
        class HA:
            online = True

            def __init__(self):
                self.registry = [{'entity_id': 'text.d1_tiles', 'platform': 'esphome', 'original_name': 'Tile settings', 'device_id': 'd1'},
                                 {'entity_id': 'sensor.d1_node', 'platform': 'esphome', 'original_name': 'Device name', 'device_id': 'd1'},
                                 {'entity_id': 'sensor.d1_fw', 'platform': 'esphome', 'original_name': 'Screen firmware', 'device_id': 'd1'},
                                 {'entity_id': 'select.d1_type', 'platform': 'esphome', 'original_name': 'Guition screen type', 'device_id': 'd1'}]
                self.devices, self.areas = [{'id': 'd1', 'name': 'Hall'}], []
                self.states = {'text.d1_tiles': {'state': 'Synced'}, 'sensor.d1_node': {'state': 'hall'}, 'sensor.d1_fw': {'state': firmware},
                               PLAYER: PLAYING, CAMERA: DOOR}
                self.changed, self.dirty, self.log = asyncio.Event(), set(), []

            async def send(self, inbox, message, action=None, respond=False):
                self.log.append(('send', inbox, dict(message)))
        return HA()

    async def test_the_screen_hears_once_per_change_and_per_session(self):
        with tempfile.TemporaryDirectory() as tmp:
            ha = self.ha()
            m = Manager(with_screen_grid(ha), Path(tmp) / 'screens.json')
            seed_layout(m, 'text.d1_tiles', validate_layout({'title': 'Hall', 'tiles': [{'entity': 'light.hall', 'name': 'Hall'}]}))
            sent = []

            class Sender:
                protocol, session, confirmed, features = 2, 'S1', 'R1', {screen_saver.FEATURE}

                async def auxiliary(self, message, *, session, revision):
                    sent.append((dict(message), session, revision))
                    return True
            m.page_senders['text.d1_tiles'] = sender = Sender()
            screen = m.screen('text.d1_tiles')
            m.savers.set('d1', CHOICE)
            # The player and the camera are followed, and the screen may load them though no tile shows them.
            self.assertTrue({PLAYER, CAMERA} <= m.watched_entities())
            self.assertTrue(m.camera_allowed('text.d1_tiles', CAMERA) and m.camera_allowed('text.d1_tiles', PLAYER))
            self.assertFalse(m.camera_allowed('text.d1_tiles', 'camera.garden'))
            await m.sync_saver('text.d1_tiles', screen)
            await m.sync_saver('text.d1_tiles', screen)
            self.assertEqual([(s[0]['k'], s[1], s[2]) for s in sent], [('media', 'S1', 'R1')])
            ha.states[PLAYER] = {**PLAYING, 'state': 'paused'}
            await m.sync_saver('text.d1_tiles', screen)
            self.assertEqual(sent[-1][0], {'op': 'saver', 'k': 'camera', 'e': CAMERA, 'n': 'Front door'})
            sender.session = 'S2'
            await m.sync_saver('text.d1_tiles', screen)
            self.assertEqual((len(sent), sent[-1][1]), (3, 'S2'), 'a new session hears it again')
            # A screen whose hello does not list it hears nothing.
            sender.features = set()
            m.savers.set('d1', {})
            await m.sync_saver('text.d1_tiles', screen)
            self.assertEqual(len(sent), 3)

    async def test_the_editor_sees_the_choice_and_what_the_screen_can(self):
        with tempfile.TemporaryDirectory() as tmp:
            m = Manager(with_screen_grid(self.ha('0.28.0')), Path(tmp) / 'screens.json')
            screen = m.screen('text.d1_tiles')
            self.assertLess(m.firmware_version('text.d1_tiles', screen), SCREENSAVER_MIN_FIRMWARE)
            m.savers.set('d1', CHOICE)
            self.assertEqual(m.saver_entities('text.d1_tiles'), {PLAYER, CAMERA})
            self.assertEqual(m.saver_message(screen)['k'], 'media')


if __name__ == '__main__':
    unittest.main()
