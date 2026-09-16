"""The page loads its own script and styles by content stamp (app 0.2.58).

After updating to 0.2.57, Safari showed an empty Screen settings panel: it kept a cached script from before
the update. The page now asks for app.js and style.css with a stamp of their content, so a new version is
always fetched and an old one is never combined with a new page.
"""
import hashlib
import importlib.util
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
STATIC = ROOT / 'screen_manager/app/static'
HAS_AIOHTTP = importlib.util.find_spec('aiohttp') is not None


@unittest.skipUnless(HAS_AIOHTTP, 'Run using .venv-portal/bin/python for server tests')
class Assets(unittest.IsolatedAsyncioTestCase):
    async def test_script_and_styles_carry_a_stamp_of_their_content(self):
        import sys
        sys.path.insert(0, str(ROOT / 'screen_manager/app'))
        sys.path.insert(0, str(ROOT / 'tests'))
        import test_scaling
        from aiohttp.test_utils import TestClient, TestServer
        from server import Manager, create_app
        stamp = hashlib.sha1((STATIC / 'app.js').read_bytes() + (STATIC / 'style.css').read_bytes()).hexdigest()[:12]
        with tempfile.TemporaryDirectory() as tmp:
            manager = Manager(test_scaling.fake_ha(), Path(tmp) / 'screens.json')
            async with TestClient(TestServer(create_app(manager, True))) as client:
                response = await client.get('/')
                page = await response.text()
                self.assertEqual(response.status, 200)
                self.assertIn(f'<script src="static/app.js?v={stamp}" defer></script>', page)
                self.assertIn(f'<link rel="stylesheet" href="static/style.css?v={stamp}" />', page)
                self.assertEqual(response.headers['Cache-Control'], 'no-store')
                self.assertIn('text/html', response.headers['Content-Type'])
                script = await client.get(f'/static/app.js?v={stamp}')
                self.assertEqual((script.status, await script.read()), (200, (STATIC / 'app.js').read_bytes()))


if __name__ == '__main__':
    unittest.main()
