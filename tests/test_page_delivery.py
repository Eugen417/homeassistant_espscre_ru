import asyncio
from copy import deepcopy
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "screen_manager/app"))
from core import Grid
from layout_migrations import migrate_legacy
from page_delivery import Sender, DeliveryError, Refused, Superseded, configuration


class Screen:
    """A wire-level test peer; physical/host-render tests cover the real parser."""
    def __init__(self):
        self.messages = []
        self.session = 0
        self.revision = "0" * 16
        self.active = False
        self.initial = []
        self.pages = []
        self.sequence = 0
        self.failure = None
        self.delayed = None

    async def send(self, message):
        self.messages.append(deepcopy(message))
        await asyncio.sleep(0)
        op = message["op"]
        if self.failure == op:
            self.failure = None
            raise TimeoutError("simulated lost connection")
        if self.delayed:
            answer, self.delayed = self.delayed, None
            return answer
        if op == "hello":
            self.session += 1
            self.sequence = 0
            return {"protocol": 2, "session": f"{self.session:016x}", "request": message["request"],
                    "status": f"Session:{self.session:016x}"}
        assert message["session"] == f"{self.session:016x}"
        assert message["seq"] > self.sequence
        self.sequence = message["seq"]
        if op == "begin":
            if not self.active or self.revision != message["rev"]:
                self.active = False
                self.initial, self.pages = [], []
            self.begin = message
            self.revision = message["rev"]
        assert message["rev"] == self.revision
        if op == "tile":
            assert not self.active
            self.initial.append(message)
        elif op == "page":
            assert not self.active
            self.pages.append(message)
        elif op == "commit":
            assert len(self.initial) == self.begin["tiles"] and len(self.pages) == self.begin["pages"]
            self.active = True
        elif op == "state":
            assert self.active and "o" not in message and "slot" not in message
        return {"protocol": 2, "session": message["session"], "seq": self.sequence, "rev": self.revision,
                "applied": self.active, "status": "Synced" if self.active else "Loading tiles"}


class DeliveryTests(unittest.IsolatedAsyncioTestCase):
    def setUp(self):
        self.record = migrate_legacy({"title": "Test", "pages": 2, "tiles": [
            {"entity": "light.test", "name": "Desk", "slot": 0, "options": {"size": "wide"}},
            {"entity": "screen.page_1", "name": "Home", "slot": 6}]}, Grid(2, 3))
        self.values = [
            {"v": 1, "op": "state", "i": 0, "entity": "light.test", "name": "Desk", "state": "on",
             "a": {"brightness": 200}, "o": {"size": "wide", "tap": "toggle", "inline": "brightness"}},
            {"v": 1, "op": "state", "i": 1, "entity": "screen.page_1", "name": "Home", "state": "on", "a": {}}]
        self.bars = [[{"k": "clock"}], [{"k": "text", "t": "21 °C"}]]
        self.region = {"clock_24h": True, "numbers": "english", "group_min": 1,
                       "percent_space": False, "keepalive": 120}
        self.screen = Screen()
        self.sender = Sender(self.screen.send)

    async def sync(self, alive=lambda: True):
        return await self.sender.synchronize("text.test_inbox", self.record, self.region, self.values, self.bars, alive)

    async def test_complete_configuration_precedes_activation(self):
        await self.sync()
        ops = [m["op"] for m in self.screen.messages]
        self.assertEqual(ops[:7], ["hello", "begin", "page", "page", "tile", "tile", "commit"])
        self.assertEqual(self.screen.initial[0]["o"]["size"], "wide")
        self.assertEqual(self.screen.initial[0]["o"]["tap"], "toggle")
        self.assertEqual(self.sender.phase, "applied")
        self.assertEqual(self.sender.confirmed, configuration(self.record, self.region))

    async def test_live_values_do_not_replace_configuration(self):
        await self.sync()
        self.screen.messages.clear()
        self.values[0]["a"]["brightness"] = 123
        self.bars[1][0]["t"] = "22 °C"
        await self.sync()
        self.assertEqual([m["op"] for m in self.screen.messages], ["state", "bar"])
        self.assertNotIn("o", self.screen.messages[0])
        self.assertEqual(self.screen.messages[1]["p"], 1)
        self.screen.messages.clear()
        await self.sync()
        self.assertEqual(self.screen.messages, [])

    async def test_future_protocol_error_never_selects_legacy_delivery(self):
        async def future(_):
            return {"status": "Error: protocol version", "protocol": 99}
        sender = Sender(future)
        with self.assertRaises(DeliveryError):
            await sender.probe()
        self.assertIsNone(sender.protocol)
        self.assertIsNone(sender.session)

    async def test_same_revision_recovers_every_interruption_without_edit(self):
        for op in ["hello", "begin", "page", "tile", "commit", "state", "bar"]:
            with self.subTest(op=op):
                self.screen = Screen()
                self.sender = Sender(self.screen.send)
                self.screen.failure = op
                with self.assertRaises(TimeoutError): await self.sync()
                self.assertIsNone(self.sender.confirmed)
                await self.sync()
                self.assertTrue(self.screen.active)
                self.assertEqual(self.sender.phase, "applied")

    async def test_manager_restart_resumes_active_layout_without_initialization(self):
        await self.sync()
        self.screen.messages.clear()
        self.sender = Sender(self.screen.send)
        await self.sync()
        ops = [m["op"] for m in self.screen.messages]
        self.assertEqual(ops[:2], ["hello", "begin"])
        self.assertNotIn("tile", ops)
        self.assertNotIn("page", ops)
        self.assertNotIn("commit", ops)

    async def test_device_reboot_reinitializes_same_revision(self):
        await self.sync()
        self.screen.active = False
        with self.assertRaises(DeliveryError): await self.sender.ping()
        self.screen.messages.clear()
        await self.sync()
        self.assertIn("commit", [m["op"] for m in self.screen.messages])

    async def test_save_during_delivery_cannot_acknowledge_newer_save(self):
        def current():
            return not any(m["op"] == "tile" for m in self.screen.messages)
        with self.assertRaises(Superseded): await self.sync(current)
        self.assertFalse(self.screen.active)
        self.assertIsNone(self.sender.confirmed)
        self.record["layout"]["title"] = "New saved title"
        await self.sync()
        self.assertEqual(self.screen.begin["title"], "New saved title")

    async def test_stale_ack_is_not_an_applied_configuration(self):
        await self.sync()
        self.screen.delayed = {"protocol": 2, "session": "a" * 16, "seq": 99,
                               "rev": self.sender.confirmed, "applied": True, "status": "Synced"}
        with self.assertRaises(DeliveryError): await self.sender.ping()
        self.assertIsNone(self.sender.confirmed)

    async def test_old_firmware_requires_actual_response_not_cached_version(self):
        async def old(_): return {"status": "Error: protocol version", "rev": "123"}
        self.assertEqual(await Sender(old).probe(), 1)
        async def stale(_): return {"status": "Synced", "rev": "123", "protocol": 2}
        with self.assertRaises(DeliveryError): await Sender(stale).probe()

    async def test_messages_preflight_before_any_replacement(self):
        self.values[0]["o"]["act"] = {"s": "light.turn_on", "d": [["data", "x" * 5000]]}
        with self.assertRaises(Refused): await self.sync()
        self.assertEqual(self.screen.messages, [])

    async def test_workspace_revision_and_settings_do_not_reload_firmware(self):
        await self.sync()
        before = self.sender.confirmed
        self.record.update(revision="new-save-revision", workspace={"positions": {}}, settings={"brightness": 20})
        await self.sync()
        self.assertEqual(self.sender.confirmed, before)

    async def test_auxiliary_replies_are_serialized_and_bound_to_request(self):
        await self.sync()
        old_session, old_revision = self.sender.session, self.sender.confirmed
        message = {"v": 1, "op": "history", "entity": "light.test", "hours": 24}
        self.screen.messages.clear()
        self.assertTrue(await self.sender.auxiliary(message, session=old_session, revision=old_revision))
        self.record["layout"]["title"] = "Changed"
        await self.sync()
        count = len(self.screen.messages)
        self.assertFalse(await self.sender.auxiliary(message, session=old_session, revision=old_revision))
        self.assertEqual(len(self.screen.messages), count)


if __name__ == "__main__": unittest.main()
