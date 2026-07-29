#!/usr/bin/env python3
import importlib.util
import os
import sys
import time
import unittest
from pathlib import Path
from unittest.mock import MagicMock, patch

AI_DIR = Path(__file__).resolve().parents[2] / "src" / "ai"
if str(AI_DIR) not in sys.path:
    sys.path.insert(0, str(AI_DIR))

SPAWNER_PY = AI_DIR / "spawner.py"
SPEC = importlib.util.spec_from_file_location("zappy_ai_spawner", SPAWNER_PY)
spawner = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(spawner)


class SpawnerTestCase(unittest.TestCase):
    def setUp(self):
        spawner.START_COMMAND = []
        spawner.CHILD_AI = None
        spawner.PLAYER_NUMBER = 1
        spawner.MAIN_PROCESS_ID = None
        spawner.CHILD_STOPPED_AT = None


class TestMainBotIsRunning(SpawnerTestCase):
    def test_no_main_process_id_assumes_running(self):
        spawner.MAIN_PROCESS_ID = None
        self.assertTrue(spawner.main_bot_is_running())

    def test_missing_proc_entry_means_not_running(self):
        spawner.MAIN_PROCESS_ID = 999999999
        self.assertFalse(spawner.main_bot_is_running())

    def test_own_process_is_running(self):
        spawner.MAIN_PROCESS_ID = os.getpid()
        self.assertTrue(spawner.main_bot_is_running())

    def test_zombie_process_is_not_running(self):
        fake_path = MagicMock()
        fake_path.read_text.return_value = "1234 (name) Z 1"
        spawner.MAIN_PROCESS_ID = 1234
        with patch.object(spawner, "Path", return_value=fake_path):
            self.assertFalse(spawner.main_bot_is_running())


class TestCanCreateBot(SpawnerTestCase):
    def test_below_team_goal_returns_true(self):
        spawner.PLAYER_NUMBER = 1
        self.assertTrue(spawner.can_create_bot())

    def test_above_team_goal_returns_false(self):
        spawner.PLAYER_NUMBER = spawner.TEAM_GOAL + 1
        self.assertFalse(spawner.can_create_bot())

    def test_at_team_goal_and_main_bot_running_returns_false(self):
        spawner.PLAYER_NUMBER = spawner.TEAM_GOAL
        with patch.object(spawner, "main_bot_is_running", return_value=True):
            self.assertFalse(spawner.can_create_bot())

    def test_at_team_goal_and_main_bot_dead_returns_true(self):
        spawner.PLAYER_NUMBER = spawner.TEAM_GOAL
        with patch.object(spawner, "main_bot_is_running", return_value=False):
            self.assertTrue(spawner.can_create_bot())


class TestChildIsRunning(SpawnerTestCase):
    def test_no_child_returns_false(self):
        spawner.CHILD_AI = None
        self.assertFalse(spawner.child_is_running())

    def test_running_child_returns_true(self):
        child = MagicMock()
        child.poll.return_value = None
        spawner.CHILD_AI = child
        self.assertTrue(spawner.child_is_running())

    def test_finished_child_clears_state_and_records_stop_time(self):
        child = MagicMock()
        child.poll.return_value = 0
        spawner.CHILD_AI = child
        spawner.CHILD_STOPPED_AT = None
        self.assertFalse(spawner.child_is_running())
        self.assertIsNone(spawner.CHILD_AI)
        self.assertIsNotNone(spawner.CHILD_STOPPED_AT)


class TestCanStartChild(SpawnerTestCase):
    def test_never_stopped_can_start(self):
        spawner.CHILD_STOPPED_AT = None
        self.assertTrue(spawner.can_start_child())

    def test_recently_stopped_must_wait(self):
        spawner.CHILD_STOPPED_AT = time.monotonic()
        self.assertFalse(spawner.can_start_child())

    def test_after_retry_wait_can_start(self):
        spawner.CHILD_STOPPED_AT = time.monotonic() - spawner.RETRY_WAIT - 1
        self.assertTrue(spawner.can_start_child())


class TestStartNewAi(SpawnerTestCase):
    def test_no_start_command_returns_false(self):
        spawner.START_COMMAND = []
        self.assertFalse(spawner.start_new_ai())

    def test_spawns_subprocess_and_clears_stop_time(self):
        spawner.START_COMMAND = ["true"]
        spawner.CHILD_STOPPED_AT = time.monotonic() - spawner.RETRY_WAIT - 1
        with patch.object(spawner.subprocess, "Popen") as popen:
            popen.return_value = MagicMock()
            self.assertTrue(spawner.start_new_ai())
            popen.assert_called_once_with(["true"], close_fds=True)
        self.assertIsNone(spawner.CHILD_STOPPED_AT)

    def test_spawn_failure_records_stop_time(self):
        spawner.START_COMMAND = ["true"]
        with patch.object(spawner.subprocess, "Popen", side_effect=OSError):
            self.assertFalse(spawner.start_new_ai())
        self.assertIsNone(spawner.CHILD_AI)
        self.assertIsNotNone(spawner.CHILD_STOPPED_AT)

    def test_already_running_child_prevents_new_spawn(self):
        child = MagicMock()
        child.poll.return_value = None
        spawner.CHILD_AI = child
        spawner.START_COMMAND = ["true"]
        with patch.object(spawner.subprocess, "Popen") as popen:
            self.assertFalse(spawner.start_new_ai())
            popen.assert_not_called()


if __name__ == "__main__":
    unittest.main()
