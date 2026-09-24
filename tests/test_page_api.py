"""HTTP contracts for canonical drafts, imports, editor storage and upgrades."""
from copy import deepcopy
import asyncio
import json
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import AsyncMock, patch

from aiohttp.test_utils import TestClient, TestServer
import test_portal
from layout_migrations import migrate_legacy
from page_layout import LayoutError
from core import Grid
from server import create_app, status_text


class PageApiTests(unittest.IsolatedAsyncioTestCase):
    async def test_page_delivery_status_follows_editor_language(self):
        for status, translated in [('Saved, waiting for screen', 'Opgeslagen; wacht op synchronisatie'),
                                   ('Applying', 'Indeling toepassen'), ('Applied', 'Indeling toegepast')]:
            self.manager.status['text.screen'] = status
            response = await self.client.get('/api/inventory', headers={'X-ESP-Screens-Language': 'nl'})
            screen = next(screen for screen in (await response.json())['screens'] if screen['id'] == 'text.screen')
            self.assertEqual(screen['delivery'], translated)

    def test_protocol_session_is_presented_as_ready(self):
        self.assertEqual(str(status_text('Session:' + 'a' * 16)),
                         str(status_text('Ready for tile configuration')))

    async def test_new_screen_capability_is_learned_without_saving_a_layout(self):
        self.manager.store.forget('text.screen')
        notified = asyncio.Event()

        async def answer(inbox, message, action=None, respond=False):
            self.assertEqual(message['op'], 'hello')
            return {'protocol': 2, 'request': message['request'], 'session': 'a' * 16, 'status': 'Session:' + 'a' * 16}

        with patch.object(self.manager, 'answers', return_value=True), \
                patch.object(self.manager.ha, 'send', new=AsyncMock(side_effect=answer)) as send, \
                patch.object(self.manager, 'notify', side_effect=notified.set):
            self.manager.ha.changed.set()
            task = asyncio.create_task(self.manager.run())
            try:
                await asyncio.wait_for(notified.wait(), 3)
                inventory = await (await self.client.get('/api/inventory')).json()
                screen = next(screen for screen in inventory['screens'] if screen['id'] == 'text.screen')
                self.assertEqual(screen['page_capability'], 'ready')
                self.assertIsNone(self.record())
                self.assertEqual(send.await_count, 1)
                await self.manager.probe_pages('text.screen', self.manager.screen('text.screen'))
                self.assertEqual(send.await_count, 1, 'ordinary refreshes must preserve the delivery session')
            finally:
                task.cancel()
                with self.assertRaises(asyncio.CancelledError): await task

    async def test_grid_adaptation_requires_the_exact_reviewed_source_and_actual_target(self):
        before = self.record()
        request = {'format': 'pages-v2', 'revision': before['revision'], 'layout': before['layout']}
        with patch.object(self.manager, 'verified_grid', return_value=Grid(1, 4)), patch.object(self.manager.store, 'grid_for', return_value=Grid(1, 4)):
            response = await self.client.put('/api/screens/text.screen', headers=self.headers, json=request)
            self.assertEqual(response.status, 400)
            self.assertEqual(self.record(), before)
            request['adaptation'] = {'from': before['sourceGrid'], 'to': {'columns': 10, 'rows': 10}}
            response = await self.client.put('/api/screens/text.screen', headers=self.headers, json=request)
            self.assertEqual(response.status, 400)
            self.assertEqual(self.record(), before)
            request['adaptation']['to'] = {'columns': 1, 'rows': 4}
            response = await self.client.put('/api/screens/text.screen', headers=self.headers, json=request)
            self.assertEqual(response.status, 200, await response.text())
            self.assertEqual(self.record()['sourceGrid'], {'columns': 1, 'rows': 4})
            self.assertEqual(self.record()['layout'], before['layout'])
            self.assertNotEqual(self.record()['revision'], before['revision'])

    async def test_graph_preview_reads_recorder_only_and_rejects_unknown_entities(self):
        self.manager.ha.states['sensor.graph'] = {'state': '0', 'attributes': {'unit_of_measurement': '°C'}}
        history = {'start': 10, 'end': 20, 'values': [0, None, 2], 'unit': '°C'}
        with patch.object(self.manager, 'card_history', new=AsyncMock(return_value=history)) as fetch:
            response = await self.client.get('/api/history-preview?entity=sensor.graph&hours=24')
            self.assertEqual(await response.json(), {'history': history})
            fetch.assert_awaited_once_with('sensor.graph', 24, 'line')
            for query in ['entity=sensor.missing', 'entity=light.a', 'entity=sensor.graph&hours=999', 'entity=sensor.graph&hours=no']:
                self.assertEqual(await (await self.client.get('/api/history-preview?' + query)).json(), {'history': None})
            self.assertEqual(fetch.await_count, 1)
            self.assertFalse(self.manager.ha.changed.is_set())

    async def test_value_changes_rebuild_only_the_dependent_page_bars(self):
        from copy import deepcopy
        from page_layout import new_id
        record = self.record()
        document = deepcopy(record['layout'])
        for page in document['pages']:
            page['topbar']['trailing'] = [{'id': new_id(), 'type': 'entity', 'entity': 'light.a'}]
        record = self.manager.store.save('text.screen', document, record['revision'])
        sender = self.manager.page_sender('text.screen', self.manager.screen('text.screen'))
        sender.confirmed = 'confirmed'
        with patch.object(sender, 'synchronize', new=AsyncMock(return_value='confirmed')), \
                patch.object(self.manager, 'header_message', wraps=self.manager.header_message) as bars, \
                patch.object(self.manager, 'tile_message', wraps=self.manager.tile_message) as tiles:
            await self.manager.sync_pages('text.screen', record, self.manager.screen('text.screen'))
            self.assertEqual(bars.call_count, 2)
            self.assertEqual(tiles.call_count, 1)
            bars.reset_mock(); tiles.reset_mock()
            await self.manager.sync_pages('text.screen', record, self.manager.screen('text.screen'), dirty={'light.unrelated'})
            bars.assert_not_called(); tiles.assert_not_called()
            await self.manager.sync_pages('text.screen', record, self.manager.screen('text.screen'), dirty={'light.a'})
            self.assertEqual(bars.call_count, 2)
            self.assertEqual(tiles.call_count, 1)

    async def asyncSetUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.path = Path(self.temp.name) / 'screens.json'
        self.manager = test_portal.ManagerTests.setup_manager(self, self.path)
        self.manager.save('text.screen', {'title': 'Test', 'tiles': [{'entity': 'light.a', 'name': '', 'slot': 0}], 'pages': 2})
        self.client = TestClient(TestServer(create_app(self.manager, True)))
        await self.client.start_server()
        self.addAsyncCleanup(self.client.close)
        inventory = await (await self.client.get('/api/inventory')).json()
        self.headers = {'X-Screen-CSRF': inventory['csrf']}
        self.manager.ha.changed.clear()

    def record(self):
        return self.manager.store.get('text.screen')

    async def post_import(self, document, grid=None):
        return await self.client.post('/api/screens/text.screen/import', headers=self.headers,
                                      json={'document': document, 'sourceGrid': grid or {'columns': 2, 'rows': 3}})

    async def test_import_validates_a_draft_without_saving_or_sending(self):
        before = self.path.read_bytes()
        source = self.record()
        source['layout']['title'] = 'Imported draft'
        result = await self.post_import({'esp_screens_layout': 2, 'sourceGrid': source['sourceGrid'], 'layout': source['layout'],
                                         'editor': {'positions': {source['layout']['homePageId']: {'x': 2, 'y': 3}}}})
        self.assertEqual(result.status, 200)
        draft = await result.json()
        self.assertEqual(draft['revision'], '')
        self.assertEqual(draft['layout']['title'], 'Imported draft')
        self.assertEqual(self.path.read_bytes(), before)
        self.assertEqual(self.manager.ha.messages, [])
        self.assertFalse(self.manager.ha.changed.is_set())

    async def test_import_rejects_unknown_versions_keys_footprints_and_dangling_routes(self):
        record = self.record()
        valid = {'esp_screens_layout': 2, 'sourceGrid': record['sourceGrid'], 'layout': record['layout']}
        rectangle, dangling = deepcopy(valid), deepcopy(valid)
        rectangle['layout']['pages'][0]['tiles'][0]['placement'].update(columns=1, rows=3)
        dangling['layout']['pages'][0]['tiles'][0]['content'] = {'kind': 'navigation', 'target': {'kind': 'page', 'pageId': 'f' * 16}}
        before = self.path.read_bytes()
        for source in [{**valid, 'api_key': 'never-import'}, {**valid, 'esp_screens_layout': 99}, rectangle, dangling]:
            with self.subTest(source=list(source)):
                self.assertEqual((await self.post_import(source)).status, 400)
                self.assertEqual(self.path.read_bytes(), before)

    async def test_old_export_runs_the_isolated_importer_with_an_explicit_grid(self):
        old = {'esp_screens_layout': 1, 'title': 'Old export', 'tiles': [{'entity': 'light.a', 'slot': 5}]}
        with patch('server.migrate_legacy', wraps=migrate_legacy) as migration:
            result = await self.post_import(old, {'columns': 3, 'rows': 3})
            self.assertEqual(result.status, 200)
            self.assertEqual(migration.call_args.args[1], Grid(3, 3))
        draft = await result.json()
        self.assertEqual(draft['layout']['pages'][0]['tiles'][0]['placement'], {'row': 1, 'column': 2, 'columns': 1, 'rows': 1})
        self.assertEqual(self.record()['layout']['title'], 'Test')

    async def test_workspace_save_requires_both_revisions_and_never_schedules_delivery(self):
        record = self.record()
        workspace = record.get('workspace', {'revision': '', 'positions': {}})
        workspace['positions'] = {record['layout']['homePageId']: {'x': 4, 'y': 2}}
        payload = {'revision': record['revision'], 'workspace': workspace}
        result = await self.client.put('/api/screens/text.screen/workspace', headers=self.headers, json=payload)
        self.assertEqual(result.status, 200)
        saved = await result.json()
        self.assertNotEqual(saved['revision'], workspace['revision'])
        self.assertEqual(self.record()['layout'], record['layout'])
        self.assertEqual(self.record()['revision'], record['revision'])
        self.assertFalse(self.manager.ha.changed.is_set())
        self.assertEqual(self.manager.ha.messages, [])
        self.assertEqual((await self.client.put('/api/screens/text.screen/workspace', headers=self.headers, json=payload)).status, 409)

    async def test_an_unavailable_existing_header_survives_an_unrelated_edit(self):
        record = self.record()
        for page in record['layout']['pages']:
            page['topbar']['trailing'] = [{'id': page['id'] + '_header', 'type': 'entity', 'entity': 'sensor.retired', 'content': 'state'}]
        saved = self.manager.store.save('text.screen', record['layout'], record['revision'])
        saved['layout']['title'] = 'Changed title'
        result = self.manager.save_pages('text.screen', {'format': 'pages-v2', 'revision': saved['revision'], 'layout': saved['layout']})
        self.assertEqual(result['layout']['pages'][0]['topbar']['trailing'][0]['entity'], 'sensor.retired')
        result['layout']['pages'][0]['topbar']['trailing'][0]['entity'] = 'sensor.new_unknown'
        self.manager.page_senders['text.screen'] = SimpleNamespace(protocol=2)
        with self.assertRaises(LayoutError):
            self.manager.save_pages('text.screen', {'format': 'pages-v2', 'revision': result['revision'], 'layout': result['layout']})

    async def test_scoped_auxiliary_reply_is_dropped_when_its_session_has_disappeared(self):
        self.assertFalse(await self.manager.send_auxiliary('text.screen', {'v': 1, 'op': 'history'}, None,
                                                           {'session': 'a' * 16, 'rev': 'b' * 16, 'view': 4}))
        self.assertEqual(self.manager.ha.messages, [])

    async def test_updating_a_pending_or_changed_grid_configuration_is_refused(self):
        self.manager.preflight_update('text.screen')
        with patch.object(self.manager, 'verified_grid', return_value=Grid(3, 3)):
            with self.assertRaisesRegex(LayoutError, 'grid'): self.manager.preflight_update('text.screen')
        pending = {'text.screen': {'format': 'legacy-v1', 'payload': '{"tiles":[]}', 'migrationError': 'Unknown field'}}
        with patch.object(self.manager.store, 'retry_migrations', return_value=pending):
            with self.assertRaisesRegex(LayoutError, 'migration'): self.manager.preflight_update('text.screen')

    async def test_preflight_refusal_leaves_the_saved_revision_and_delivery_untouched(self):
        from page_delivery import Refused
        record = self.record(); before = self.path.read_bytes()
        record['layout']['title'] = 'Oversized configuration'
        with patch('server.page_delivery.prepare', side_effect=Refused('Message too large')):
            with self.assertRaisesRegex(LayoutError, 'Message too large'):
                self.manager.save_pages('text.screen', {'format': 'pages-v2', 'revision': record['revision'], 'layout': record['layout']})
        self.assertEqual(self.path.read_bytes(), before)
        self.assertFalse(self.manager.ha.changed.is_set())
        self.assertEqual(self.manager.ha.messages, [])
