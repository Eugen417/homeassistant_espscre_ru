"""The page loads its own script and styles by content hash (app 0.2.58, Vite build since 0.2.74).

After updating to 0.2.57, Safari showed an empty Screen settings panel: it kept a cached script from before
the update. Vite names every built file after a hash of its content and index.html asks for those names, so a
new version is always fetched and an old one is never combined with a new page.
"""
import importlib.util
import re
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
STATIC = ROOT / 'screen_manager/app/static'
HAS_AIOHTTP = importlib.util.find_spec('aiohttp') is not None


class Build(unittest.TestCase):
    def test_the_build_is_committed_and_names_its_files_by_hash(self):
        page = (STATIC / 'index.html').read_text()
        script = re.search(r'<script type="module" crossorigin src="\./(assets/index-[\w-]+\.js)"></script>', page)
        styles = re.search(r'<link rel="stylesheet" crossorigin href="\./(assets/index-[\w-]+\.css)">', page)
        self.assertTrue(script and styles, 'index.html is the Vite build of web/ (cd web && npm run build)')
        for name in (script[1], styles[1]):
            self.assertTrue((STATIC / name).exists(), name)
        self.assertFalse((STATIC / 'app.js').exists(), 'the old script is gone')
        self.assertEqual(len(list((STATIC / 'assets').glob('index-*.js'))), 1, 'one build at a time: npm run build clears assets/')


@unittest.skipUnless(HAS_AIOHTTP, 'Run using .venv-portal/bin/python for server tests')
class Assets(unittest.IsolatedAsyncioTestCase):
    async def test_the_server_serves_the_page_and_its_hashed_assets(self):
        import sys
        sys.path.insert(0, str(ROOT / 'screen_manager/app'))
        sys.path.insert(0, str(ROOT / 'tests'))
        import test_scaling
        from aiohttp.test_utils import TestClient, TestServer
        from server import Manager, create_app
        page = (STATIC / 'index.html').read_text()
        script = re.search(r'src="\./(assets/index-[\w-]+\.js)"', page)[1]
        with tempfile.TemporaryDirectory() as tmp:
            manager = Manager(test_scaling.fake_ha(), Path(tmp) / 'screens.json')
            async with TestClient(TestServer(create_app(manager, True))) as client:
                response = await client.get('/')
                self.assertEqual(response.status, 200)
                self.assertEqual(await response.text(), page)
                self.assertEqual(response.headers['Cache-Control'], 'no-store')
                self.assertIn('text/html', response.headers['Content-Type'])
                asset = await client.get(f'/{script}')
                self.assertEqual((asset.status, await asset.read()), (200, (STATIC / script).read_bytes()))
                self.assertEqual((await client.get('/static/app.js')).status, 404)


if __name__ == '__main__':
    unittest.main()
