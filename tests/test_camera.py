"""Camera images on a Guition (app 0.2.66, firmware 0.2.57): the app fetches, sizes and serves the image on its own
port; the screen asks with esphome.screen_camera and loads the link with ESPHome's online_image."""
import asyncio
import importlib.util
import io
from pathlib import Path
import re
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import profiles  # noqa: E402
sys.path.insert(0, str(ROOT / 'screen_manager/app'))
import camera_feed  # noqa: E402
from core import (ALERT_FIELDS, BROADCAST_SHOW, CAMERA_MIN_FIRMWARE, alert_camera, alert_reference, entity_id,  # noqa: E402
                  header_entity, min_firmware, validate_layout, extras)

HAS_PIL = importlib.util.find_spec('PIL') is not None
HAS_AIOHTTP = importlib.util.find_spec('aiohttp') is not None
if HAS_AIOHTTP:
    from server import HomeAssistant, Manager

PROFILE = profiles.text('guition-4848s040.yaml')
TILES = (ROOT / 'components/smart_display/runtime_tiles.h').read_text()


def picture(fmt, size, mode='RGB'):
    from PIL import Image
    out = io.BytesIO()
    Image.new(mode, size, (200, 30, 90) if mode == 'RGB' else None).save(out, fmt)
    return out.getvalue()


class Rules(unittest.TestCase):
    def test_camera_and_image_tiles_need_the_camera_firmware(self):
        self.assertTrue(entity_id('camera.front_door') and entity_id('image.doorbell'))
        self.assertEqual(camera_feed.MIN_FIRMWARE, CAMERA_MIN_FIRMWARE)
        layout = validate_layout({'title': 'Hall', 'tiles': [{'entity': 'camera.front_door', 'name': ''}]})
        self.assertEqual(min_firmware(layout), (0, 2, 57))
        # Automatic and "open control" both open the image full screen; view only keeps the tile still.
        validate_layout({'title': 'Hall', 'tiles': [{'entity': 'camera.front_door', 'name': '', 'options': {'tap': 'detail'}}]})
        validate_layout({'title': 'Hall', 'tiles': [{'entity': 'image.doorbell', 'name': '', 'options': {'tap': 'none'}}]})
        # A top bar item shows a state; a camera has nothing worth reading there.
        self.assertFalse(header_entity('camera.front_door'))

    def test_an_image_tile_knows_when_its_picture_changed(self):
        states = {'image.doorbell': {'state': '2026-09-17T12:00:00+00:00', 'attributes': {}}}
        self.assertEqual(extras({'entity': 'image.doorbell'}, states), {'last': 1789646400})

    def test_which_screens_draw_images(self):
        self.assertTrue(camera_feed.can_show({'board': 'guition', 'firmware': '0.2.57'}))
        self.assertTrue(camera_feed.can_show({'board': 'guition', 'firmware': '0.3.0'}))
        for screen in ({'board': 'guition', 'firmware': '0.2.56'}, {'board': 'unknown', 'firmware': '0.2.57'},
                       {'board': 'guition', 'firmware': 'unknown'}, None, {}):
            self.assertFalse(camera_feed.can_show(screen), screen)
        self.assertTrue(camera_feed.supported('camera.max') and camera_feed.supported('image.max_motion'))
        for value in ('light.kitchen', 'camera', 'Camera.Max', None, 42, 'camera.' + 'x' * 120):
            self.assertFalse(camera_feed.supported(value), value)

    def test_the_alert_camera_field(self):
        self.assertEqual(alert_camera({'camera': 'camera.front_door'}), ('camera.front_door', True))
        self.assertEqual(alert_camera({'camera': ' image.doorbell '}), ('image.doorbell', True))
        self.assertEqual(alert_camera({}), ('', True))
        self.assertEqual(alert_camera({'camera': ''}), ('', True))
        for value in ('light.hall', True, ['camera.max'], 'camera'):
            self.assertEqual(alert_camera({'camera': value}), ('', False), value)
        # Not an argument of the show_alert action: older firmware would refuse the whole call.
        self.assertNotIn('camera', [name for name, *_ in ALERT_FIELDS])
        self.assertEqual(alert_reference()['camera']['name'], 'camera')

    def test_the_boxes_are_the_profile_sizes(self):
        subs = dict(re.findall(r'^  (CAMERA_\w+): "(\d+)"', PROFILE, re.M))
        self.assertEqual(camera_feed.BOXES['guition'], {'full': (int(subs['CAMERA_FULL_W']), int(subs['CAMERA_FULL_H'])),
                                                        'thumb': (int(subs['CAMERA_THUMB_W']), int(subs['CAMERA_THUMB_H']))})

    def test_the_firmware_and_the_app_speak_the_same_words(self):
        self.assertIn('request.service = esphome::StringRef("esphome.screen_camera");', TILES)
        self.assertIn("event_type='esphome.screen_camera'", (ROOT / 'screen_manager/app/server.py').read_text())
        self.assertIn('if (op == "camera") {', TILES)
        self.assertIn('"camera", "image"})', (ROOT / 'components/smart_display/runtime_model.h').read_text())
        # The profile loads both images and binds them; the CYD has none, so it never opens a camera.
        for needle in ('online_image:\n  - id: camera_image', '  - id: alert_image', 'runtime_tiles::camera_loaded(false, cached);',
                       'runtime_tiles::camera_loaded(true, cached);', 'runtime_tiles::camera_tick();', 'runtime_tiles::alert_prepare();',
                       'runtime_tiles::alert_clear();', 'runtime_tiles::camera_close();', 'id: alert_image_frame'):
            self.assertIn(needle, PROFILE, needle)
        # A link only replaces the URL when it is new: set_url() forgets the ETag that makes an unchanged picture a 304.
        self.assertEqual(PROFILE.count('if (url != current) {'), 2)
        # A busy camera port leaves the rest of the app running.
        self.assertIn("except OSError as error:\n            # Everything else still works; only camera images stay away.", (ROOT / 'screen_manager/app/server.py').read_text())
        cyd = profiles.text('home-like-2432s028.yaml')
        self.assertNotIn('online_image', cyd)
        self.assertNotIn('camera_full.load', cyd)
        # The add-on's port is published, and the Docker route passes it on with the host network.
        config = (ROOT / 'screen_manager/config.yaml').read_text()
        self.assertIn(f'{camera_feed.PORT}/tcp: {camera_feed.PORT}', config)

    def test_the_first_picture_waits_for_no_tick(self):
        # Firmware 0.2.73+: the camera asks when it opens and loads the link when it comes, not on the next 250 ms tick;
        # the spinner turns until the first picture or a note is there.
        opened = TILES.split('inline void camera_open(const std::string &entity, const std::string &name) {', 1)[1].split('\n}\n', 1)[0]
        self.assertIn('camera_spinner = spinner_create(camera_root,', opened)
        self.assertIn('if (awake() && fresh() && camera.should_ask(now)) {', opened)
        answer = TILES.split('inline void camera_answer(const std::string &view, const std::string &entity, const std::string &url) {', 1)[1].split('\n}\n', 1)[0]
        self.assertIn('if (awake() && camera.should_load(now)) camera_load(now);', answer)
        # Never under a finger, from the answer or from the tick.
        load = TILES.split('inline void camera_load(uint32_t now) {', 1)[1].split('\n}\n', 1)[0]
        self.assertIn('lv_indev_get_state(input) == LV_INDEV_STATE_PRESSED) return;', load)
        note = TILES.split('inline void camera_note_text(const char *text) {', 1)[1].split('\n}\n', 1)[0]
        self.assertIn('lv_obj_delete(camera_spinner);', note)
        self.assertNotIn('Loading image', TILES)


@unittest.skipUnless(HAS_PIL, 'Pillow comes with ESPHome in the add-on image')
class Encoding(unittest.TestCase):
    def test_every_snapshot_becomes_a_bmp_that_fits(self):
        from PIL import Image
        cases = [(picture('JPEG', (1920, 1080)), (480, 480), (480, 270)),
                 (picture('JPEG', (1920, 1080)), (392, 220), (391, 220)),
                 (picture('PNG', (400, 100), 'RGBA'), (480, 480), (480, 120)),
                 (picture('GIF', (550, 512), 'P'), (392, 220), (236, 220)),
                 (picture('JPEG', (1080, 1440)), (480, 480), (360, 480)),
                 (picture('PNG', (100, 50)), (480, 480), (480, 240))]
        for raw, box, size in cases:
            bmp = camera_feed.encode(raw, box)
            with Image.open(io.BytesIO(bmp)) as image:
                self.assertEqual((image.format, image.size, image.mode), ('BMP', size, 'RGB'))
            # What ESPHome's BMP decoder reads: 24 bits per pixel, no compression, rows padded to four bytes.
            self.assertEqual((bmp[:2], int.from_bytes(bmp[28:30], 'little'), int.from_bytes(bmp[30:34], 'little')), (b'BM', 24, 0))
            self.assertEqual(len(bmp), int.from_bytes(bmp[10:14], 'little') + size[1] * ((size[0] * 3 + 3) // 4 * 4))
        with self.assertRaises(Exception):
            camera_feed.encode(b'not an image', (480, 480))

    def test_fit_keeps_proportions(self):
        self.assertEqual(camera_feed.fit((1920, 1080), (480, 480)), (480, 270))
        self.assertEqual(camera_feed.fit((1, 5000), (224, 126)), (1, 126))
        self.assertEqual(camera_feed.fit((5000, 1), (224, 126)), (224, 1))


class Clock:
    def __init__(self): self.now = 1000.0
    def __call__(self): return self.now


@unittest.skipUnless(HAS_PIL, 'Pillow comes with ESPHome in the add-on image')
class Feed(unittest.IsolatedAsyncioTestCase):
    def feed(self, answers):
        self.fetched, self.gate = [], None
        clock = Clock()

        async def fetch(entity):
            self.fetched.append(entity)
            answer = answers[min(len(self.fetched), len(answers)) - 1]
            if self.gate is not None:
                # A camera that takes its time: the fetch stays on its way until the test opens the gate.
                await self.gate.wait()
            if isinstance(answer, Exception):
                raise answer
            return answer
        return camera_feed.CameraFeed(fetch, clock=clock), clock

    async def settle(self):
        """Let the fetch a load started run before counting fetches: whether it already ran when serve() returns
        depends on whether serve() had to wait for anything itself."""
        for _ in range(5):
            await asyncio.sleep(0)

    async def test_links_serve_the_last_frame_and_answer_unchanged_images_with_304(self):
        feed, clock = self.feed([picture('JPEG', (1920, 1080))])
        token = feed.link('camera.max', (480, 480))
        status, bmp, etag = await feed.serve(token)
        self.assertEqual((status, bmp[:2]), (200, b'BM'))
        self.assertRegex(etag, r'^"[0-9a-f]{16}-480x480"$')
        await self.settle()
        # The camera answered the same picture again: the screen keeps what it has.
        self.assertEqual(await feed.serve(token, etag), (304, None, etag))
        self.assertEqual((await feed.serve('unknown'))[0], 404)
        # A link nobody used for LINK_SECONDS is gone; the screen asks for a new one.
        clock.now += camera_feed.LINK_SECONDS + 1
        self.assertEqual((await feed.serve(token))[0], 404)

    async def test_each_load_fetches_the_next_picture_and_nothing_is_fetched_between_loads(self):
        first, second, third = picture('JPEG', (640, 360)), picture('PNG', (640, 360)), picture('BMP', (640, 360))
        feed, clock = self.feed([first, second, third])
        token = feed.link('camera.max', (480, 480))
        _, one, tag_one = await feed.serve(token)
        await self.settle()
        self.assertEqual(self.fetched, ['camera.max'] * 2, 'the first picture, and the next one started at once')
        await asyncio.sleep(0.05)
        self.assertEqual(len(self.fetched), 2, 'nobody loads, nothing is fetched')
        clock.now += 4
        _, two, tag_two = await feed.serve(token, tag_one)
        await self.settle()
        self.assertNotEqual(tag_two, tag_one, 'the picture fetched after the last load')
        self.assertEqual(len(self.fetched), 3)
        # Two screens loading while the next picture is on its way share that one fetch.
        self.gate = asyncio.Event()
        await feed.serve(token, tag_two)
        await feed.serve(token, tag_two)
        await self.settle()
        self.assertEqual(len(self.fetched), 4)
        self.gate.set()
        await self.settle()

    async def test_a_camera_that_fails_is_asked_again_after_a_pause(self):
        feed, clock = self.feed([ConnectionError('500'), ConnectionError('500'), picture('JPEG', (640, 360))])
        token = feed.link('camera.woonkamer_live', (480, 480))
        with self.assertLogs(camera_feed.LOG, 'INFO'):
            self.assertEqual((await feed.serve(token))[0], 503)
            self.assertEqual((await feed.serve(token))[0], 503)
            await self.settle()
            self.assertEqual(len(self.fetched), 1, 'no second try within the pause')
            clock.now += 5
            self.assertEqual((await feed.serve(token))[0], 503)
            await self.settle()
            self.assertEqual(len(self.fetched), 2)
            clock.now += 10
            self.assertEqual((await feed.serve(token))[0], 200)
            await self.settle()
        self.assertEqual(len(self.fetched), 4, 'the good picture, then the next one')

    async def test_an_alert_gets_a_snapshot_of_its_own_moment(self):
        first, second = picture('JPEG', (640, 360)), picture('PNG', (640, 360))
        feed, clock = self.feed([first, second])
        tag_first, _ = await feed.frame('camera.front_door', (392, 220), fresh=False)
        await self.settle()
        self.assertEqual(len(self.fetched), 1)
        clock.now += 20
        # The bell rings 20 s later: the kept snapshot is not the moment, a new fetch is.
        tag_alert, _ = await feed.frame('camera.front_door', (392, 220), fresh=False, now=True)
        await self.settle()
        self.assertEqual(len(self.fetched), 2)
        self.assertNotEqual(tag_alert, tag_first)

    async def test_an_alert_whose_camera_fails_gets_no_old_picture(self):
        feed, clock = self.feed([picture('JPEG', (640, 360)), ConnectionError('offline')])
        await feed.frame('camera.front_door', (392, 220), fresh=False)
        clock.now += 20
        with self.assertLogs(camera_feed.LOG, 'INFO'):
            self.assertIsNone(await feed.frame('camera.front_door', (392, 220), fresh=False, now=True))

    async def test_a_camera_nobody_loads_is_forgotten(self):
        feed, clock = self.feed([picture('JPEG', (64, 36))])
        await feed.frame('camera.max', (480, 480), fresh=False)
        self.assertIn('camera.max', feed.watches)
        clock.now += camera_feed.WATCH_SECONDS + 1
        feed.watch('camera.garden')
        self.assertNotIn('camera.max', feed.watches)

    async def test_a_still_is_one_fixed_image(self):
        feed, _ = self.feed([RuntimeError('never asked')])
        bmp = camera_feed.encode(picture('JPEG', (640, 360)), (392, 220))
        token = feed.link('camera.max', (392, 220), bmp)
        status, body, etag = await feed.serve(token)
        self.assertEqual((status, body), (200, bmp))
        self.assertEqual((await feed.serve(token, etag))[0], 304)
        self.assertEqual(self.fetched, [])

    async def test_the_number_of_links_is_bounded(self):
        feed, clock = self.feed([picture('JPEG', (64, 36))])
        tokens = []
        for _ in range(camera_feed.MAX_LINKS + 10):
            clock.now += 0.01
            tokens.append(feed.link('camera.max', (480, 480)))
        self.assertLessEqual(len(feed.links), camera_feed.MAX_LINKS)
        self.assertNotIn(tokens[0], feed.links)
        self.assertIn(tokens[-1], feed.links)
        self.assertTrue(all(re.fullmatch(r'[A-Za-z0-9_-]{24}', token) for token in tokens))

    @unittest.skipUnless(HAS_AIOHTTP, 'Run using .venv-portal/bin/python for server tests')
    async def test_the_http_port(self):
        from aiohttp.test_utils import TestClient, TestServer
        feed, _ = self.feed([picture('JPEG', (1920, 1080))])
        token = feed.link('camera.max', (480, 480))
        async with TestClient(TestServer(camera_feed.web_app(feed))) as client:
            response = await client.get(f'/camera/{token}.bmp')
            body = await response.read()
            self.assertEqual((response.status, response.content_type, int(response.headers['Content-Length'])), (200, 'image/bmp', len(body)))
            etag = response.headers['ETag']
            self.assertEqual((await client.get(f'/camera/{token}.bmp', headers={'If-None-Match': etag})).status, 304)
            self.assertEqual((await client.get('/camera/abcdefghijklmnopqrstuvwx.bmp')).status, 404)
            self.assertEqual((await client.get('/api/inventory')).status, 404)
            self.assertEqual((await client.post(f'/camera/{token}.bmp')).status, 405)


def fake_ha(image=None):
    """Home Assistant with two current screens (a Guition and a CYD) and an old Guition."""
    class HA:
        online = True

        def __init__(self):
            self.registry, self.devices, self.states = [], [], {}
            for device, node, firmware, guition in (('d1', 'hall', '0.2.57', True), ('d2', 'desk', '0.2.57', False),
                                                    ('d3', 'attic', '0.2.56', True)):
                self.registry += [{'entity_id': f'text.{device}_tiles', 'platform': 'esphome', 'original_name': 'Tile settings', 'device_id': device},
                                  {'entity_id': f'sensor.{device}_node', 'platform': 'esphome', 'original_name': 'Device name', 'device_id': device},
                                  {'entity_id': f'sensor.{device}_fw', 'platform': 'esphome', 'original_name': 'Screen firmware', 'device_id': device}]
                if guition:
                    self.registry.append({'entity_id': f'select.{device}_type', 'platform': 'esphome', 'original_name': 'Guition screen type', 'device_id': device})
                self.devices.append({'id': device, 'name': node.title()})
                self.states.update({f'text.{device}_tiles': {'state': 'Synced'}, f'sensor.{device}_node': {'state': node},
                                    f'sensor.{device}_fw': {'state': firmware}})
            self.areas = []
            self.changed = asyncio.Event()
            self.log = []
            self.responses = set()

        async def call(self, action, data):
            self.log.append(('call', action, data))

        async def send(self, inbox, message, action=None, respond=False):
            self.log.append(('send', inbox, dict(message)))

        async def request(self, kind, **data):
            if kind == 'network':
                return {'adapters': [{'name': 'end0', 'default': True, 'ipv4': [{'address': '192.168.1.57'}]}]}
            raise ConnectionError(kind)

        async def camera_image(self, entity):
            self.log.append(('fetch', entity))
            if image is None:
                raise ConnectionError('500')
            return image
    return HA()


@unittest.skipUnless(HAS_AIOHTTP and HAS_PIL, 'Run using .venv-portal/bin/python for server tests')
class App(unittest.IsolatedAsyncioTestCase):
    def setUp(self):
        camera_feed.base_url.__defaults__[0].clear()

    async def test_an_alert_with_a_camera(self):
        with tempfile.TemporaryDirectory() as tmp:
            ha = fake_ha(picture('JPEG', (1920, 1080)))
            m = Manager(ha, Path(tmp) / 'screens.json')
            await m.broadcast(BROADCAST_SHOW, {'title': 'Someone is at the door', 'camera': 'camera.front_door'})
            sends = [entry for entry in ha.log if entry[0] == 'send']
            calls = [entry for entry in ha.log if entry[0] == 'call']
            # Only the current Guition hears of the image: first announced, then the alert, then the link.
            self.assertEqual([target for _, target, _ in sends], ['text.d1_tiles', 'text.d1_tiles'])
            self.assertEqual(sends[0][2], {'v': 1, 'op': 'camera', 't': 'alert', 'e': 'camera.front_door', 'u': ''})
            self.assertLess(ha.log.index(sends[0]), ha.log.index(calls[0]))
            self.assertEqual(len(calls), 3, 'every current screen gets the alert itself')
            self.assertNotIn('camera', calls[0][2])
            url = sends[1][2]['u']
            self.assertRegex(url, r'^http://192\.168\.1\.57:8098/camera/[A-Za-z0-9_-]{24}\.bmp$')
            token = url.rsplit('/', 1)[1][:-4]
            self.assertEqual(m.camera.links[token].box, (392, 220))
            self.assertEqual((await m.camera.serve(token))[0], 200)
            # The alert's camera may be opened full screen without a tile.
            self.assertTrue(m.camera_allowed('text.d1_tiles', 'camera.front_door'))

    async def test_an_alert_whose_camera_has_no_image_still_goes_out(self):
        with tempfile.TemporaryDirectory() as tmp:
            ha = fake_ha(None)
            m = Manager(ha, Path(tmp) / 'screens.json')
            with self.assertLogs('screen_manager', 'INFO') as logs:
                await m.broadcast(BROADCAST_SHOW, {'title': 'Door', 'camera': 'camera.front_door'})
            sends = [entry for entry in ha.log if entry[0] == 'send']
            self.assertEqual([message['u'] for _, _, message in sends], ['', ''])
            self.assertEqual(len([entry for entry in ha.log if entry[0] == 'call']), 3)
            self.assertTrue(any('(no image)' in line for line in logs.output), logs.output)

    async def test_an_unusable_camera_is_named_and_left_out(self):
        with tempfile.TemporaryDirectory() as tmp:
            ha = fake_ha(picture('JPEG', (64, 36)))
            m = Manager(ha, Path(tmp) / 'screens.json')
            with self.assertLogs('screen_manager', 'INFO') as logs:
                await m.broadcast(BROADCAST_SHOW, {'title': 'Door', 'camera': 'light.hall'})
            self.assertEqual([entry[0] for entry in ha.log], ['call'] * 3)
            self.assertTrue(any('unusable camera left empty' in line for line in logs.output), logs.output)

    async def test_a_screen_asks_for_a_camera_it_may_show(self):
        with tempfile.TemporaryDirectory() as tmp:
            ha = fake_ha(picture('JPEG', (1920, 1080)))
            m = Manager(ha, Path(tmp) / 'screens.json')
            m.layouts['text.d1_tiles'] = validate_layout({'title': 'Hall', 'tiles': [{'entity': 'camera.max', 'name': ''}]})
            with self.assertLogs('screen_manager', 'INFO'):
                await m.answer_camera({'inbox': 'text.d1_tiles', 'entity': 'camera.max'})
            (_, inbox, message), = [entry for entry in ha.log if entry[0] == 'send']
            self.assertEqual((inbox, message['op'], message['t'], message['e']), ('text.d1_tiles', 'camera', 'full', 'camera.max'))
            token = message['u'].rsplit('/', 1)[1][:-4]
            self.assertEqual(m.camera.links[token].box, (480, 480))
            ha.log.clear()
            # Not on its layout, a CYD, an old Guition, a light: no link at all.
            for request in ({'inbox': 'text.d1_tiles', 'entity': 'camera.garden'}, {'inbox': 'text.d2_tiles', 'entity': 'camera.max'},
                            {'inbox': 'text.d3_tiles', 'entity': 'camera.max'}, {'inbox': 'text.d1_tiles', 'entity': 'light.hall'},
                            {'inbox': 'text.unknown', 'entity': 'camera.max'}, 'nonsense'):
                await m.answer_camera(request)
            self.assertEqual([entry for entry in ha.log if entry[0] == 'send'], [])

    async def test_camera_tiles_only_on_a_guition(self):
        with tempfile.TemporaryDirectory() as tmp:
            ha = fake_ha()
            m = Manager(ha, Path(tmp) / 'screens.json')
            m.inventory = lambda: (m.screens(), [{'id': 'camera.max'}])
            m.write_layouts = lambda layouts: None
            m.notify = lambda: None
            with self.assertRaisesRegex(ValueError, 'Guition'):
                m.save('text.d2_tiles', {'title': 'Desk', 'tiles': [{'entity': 'camera.max', 'name': ''}]})
            # A CYD on old firmware hears the same, not "update first": an update doesn't make room for images.
            ha.states['sensor.d2_fw']['state'] = '0.2.54'
            m._screens_key = None
            with self.assertRaisesRegex(ValueError, 'Guition'):
                m.save('text.d2_tiles', {'title': 'Desk', 'tiles': [{'entity': 'camera.max', 'name': ''}]})
            with self.assertRaisesRegex(ValueError, '0.2.57'):
                m.save('text.d3_tiles', {'title': 'Attic', 'tiles': [{'entity': 'camera.max', 'name': ''}]})
            m.save('text.d1_tiles', {'title': 'Hall', 'tiles': [{'entity': 'camera.max', 'name': ''}]})
            self.assertEqual(m.layouts['text.d1_tiles']['tiles'][0]['entity'], 'camera.max')

    async def test_home_assistant_queues_camera_requests(self):
        from aiohttp import WSMsgType

        class Message:
            type = WSMsgType.TEXT
            def __init__(self, data): self.data = data
            def json(self): return self.data

        class Socket:
            def __aiter__(self): return self._iterate()
            async def _iterate(self):
                yield Message({'type': 'event', 'event': {'event_type': 'esphome.screen_camera', 'data': {'inbox': 'text.d1_tiles', 'entity': 'camera.max'}}})
        ha = HomeAssistant(None, 'http://ha/api', 'token')
        ha.ws = Socket()
        with self.assertRaises(ConnectionError):
            await ha.read()
        self.assertEqual(ha.camera_requests.get_nowait(), {'inbox': 'text.d1_tiles', 'entity': 'camera.max'})


if __name__ == '__main__':
    unittest.main()
