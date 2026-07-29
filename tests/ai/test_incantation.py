#!/usr/bin/env python3
import importlib.util
import sys
import unittest
from pathlib import Path
from unittest.mock import MagicMock, patch

AI_DIR = Path(__file__).resolve().parents[2] / "src" / "ai"
if str(AI_DIR) not in sys.path:
    sys.path.insert(0, str(AI_DIR))

INCANTATION_PY = AI_DIR / "incantation.py"
SPEC = importlib.util.spec_from_file_location("zappy_ai_incantation", INCANTATION_PY)
incantation = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(incantation)


class IncantationTestCase(unittest.TestCase):
    def setUp(self):
        incantation.PLAYER_LEVEL = 1
        incantation.TEAM_NAME = ""
        incantation.TEAM_PLAYERS.clear()
        incantation.LAST_INCANTATION_CALL = None
        incantation.LAST_FOLLOWED_CALL = None
        incantation.PLAYER_ID = "self-id"
        incantation.INCANTATION_CALLS.clear()
        incantation.READY_PLAYERS.clear()
        incantation.TEAM_STATUS_TURNS = 0
        incantation.HELP_MESSAGE_SENT = False
        incantation.READY_MESSAGE_SENT = False
        incantation.COORDINATION_TURN = 0


class TestRememberIncantationCall(IncantationTestCase):
    def test_malformed_message_without_comma_is_ignored(self):
        incantation.remember_incantation_call("message 3")
        self.assertIsNone(incantation.LAST_INCANTATION_CALL)

    def test_invalid_header_is_ignored(self):
        incantation.remember_incantation_call("notmessage 3,incantation 1")
        self.assertIsNone(incantation.LAST_INCANTATION_CALL)

    def test_non_digit_direction_is_ignored(self):
        incantation.remember_incantation_call("message x,incantation 1")
        self.assertIsNone(incantation.LAST_INCANTATION_CALL)

    def test_direction_out_of_range_is_ignored(self):
        incantation.remember_incantation_call("message 9,incantation 1")
        self.assertIsNone(incantation.LAST_INCANTATION_CALL)

    def test_team_status_message_updates_team_players(self):
        incantation.TEAM_NAME = "Team"
        incantation.remember_incantation_call("message 3,team_status|Team|2|peer-1")
        self.assertEqual(incantation.TEAM_PLAYERS["peer-1"], (2, 0))

    def test_incantation_request_for_other_team_is_ignored(self):
        incantation.TEAM_NAME = "Team"
        incantation.remember_incantation_call("message 3,incantation_request|Other|1|peer-1")
        self.assertEqual(incantation.INCANTATION_CALLS, {})

    def test_incantation_request_for_own_team_is_saved(self):
        incantation.TEAM_NAME = "Team"
        incantation.remember_incantation_call("message 3,incantation_request|Team|1|peer-1")
        self.assertEqual(incantation.INCANTATION_CALLS["peer-1"], (3, 1, 0))

    def test_incantation_ready_updates_ready_players(self):
        incantation.TEAM_NAME = "Team"
        incantation.remember_incantation_call("message 0,incantation_ready|Team|1|self-id|peer-2")
        self.assertIn("peer-2", incantation.READY_PLAYERS)

    def test_incantation_ready_wrong_caller_is_ignored(self):
        incantation.TEAM_NAME = "Team"
        incantation.remember_incantation_call("message 0,incantation_ready|Team|1|someone-else|peer-2")
        self.assertNotIn("peer-2", incantation.READY_PLAYERS)

    def test_legacy_compact_request_used_when_no_team(self):
        incantation.remember_incantation_call("message 3,incantation_request_1_peer-1")
        self.assertEqual(incantation.INCANTATION_CALLS["peer-1"], (3, 1, 0))

    def test_legacy_compact_ready_used_when_no_team(self):
        incantation.remember_incantation_call("message 0,incantation_ready_1_self-id_peer-2")
        self.assertIn("peer-2", incantation.READY_PLAYERS)

    def test_legacy_plain_incantation_matching_level_sets_last_call(self):
        incantation.remember_incantation_call("message 5,incantation 1")
        self.assertEqual(incantation.LAST_INCANTATION_CALL, (5, 1))

    def test_legacy_plain_incantation_wrong_level_ignored(self):
        incantation.remember_incantation_call("message 5,incantation 2")
        self.assertIsNone(incantation.LAST_INCANTATION_CALL)

    def test_legacy_request_fallback_when_no_team(self):
        incantation.remember_incantation_call("message 2,incantation request 1 peer-3")
        self.assertEqual(incantation.INCANTATION_CALLS["peer-3"], (2, 1, 0))

    def test_legacy_ready_fallback_when_no_team(self):
        incantation.remember_incantation_call("message 0,incantation ready 1 self-id peer-4")
        self.assertIn("peer-4", incantation.READY_PLAYERS)


class TestSaveIncantationCall(IncantationTestCase):
    def test_non_digit_level_is_ignored(self):
        incantation.save_incantation_call(2, "abc", "peer")
        self.assertEqual(incantation.INCANTATION_CALLS, {})

    def test_level_mismatch_is_ignored(self):
        incantation.save_incantation_call(2, "3", "peer")
        self.assertEqual(incantation.INCANTATION_CALLS, {})

    def test_matching_level_is_stored(self):
        incantation.save_incantation_call(2, "1", "peer")
        self.assertEqual(incantation.INCANTATION_CALLS["peer"], (2, 1, 0))


class TestSaveReadyPlayer(IncantationTestCase):
    def test_wrong_caller_id_is_ignored(self):
        incantation.save_ready_player(0, "1", "someone-else", "peer")
        self.assertEqual(incantation.READY_PLAYERS, {})

    def test_self_as_ready_player_is_ignored(self):
        incantation.save_ready_player(0, "1", "self-id", "self-id")
        self.assertEqual(incantation.READY_PLAYERS, {})

    def test_nonzero_direction_is_ignored(self):
        incantation.save_ready_player(3, "1", "self-id", "peer")
        self.assertEqual(incantation.READY_PLAYERS, {})

    def test_valid_ready_player_is_stored(self):
        incantation.save_ready_player(0, "1", "self-id", "peer")
        self.assertIn("peer", incantation.READY_PLAYERS)


class TestSaveTeamStatus(IncantationTestCase):
    def test_wrong_team_is_ignored(self):
        incantation.TEAM_NAME = "Team"
        incantation.save_team_status("Other", "2", "peer")
        self.assertEqual(incantation.TEAM_PLAYERS, {})

    def test_self_status_is_ignored(self):
        incantation.TEAM_NAME = "Team"
        incantation.save_team_status("Team", "2", "self-id")
        self.assertEqual(incantation.TEAM_PLAYERS, {})

    def test_out_of_range_level_is_ignored(self):
        incantation.TEAM_NAME = "Team"
        incantation.save_team_status("Team", "9", "peer")
        self.assertEqual(incantation.TEAM_PLAYERS, {})

    def test_valid_status_is_stored(self):
        incantation.TEAM_NAME = "Team"
        incantation.save_team_status("Team", "3", "peer")
        self.assertEqual(incantation.TEAM_PLAYERS["peer"], (3, 0))


class TestCountTeamPlayersAtLevel(IncantationTestCase):
    def test_counts_self_plus_matching_peers(self):
        incantation.TEAM_PLAYERS["peer-1"] = (2, 0)
        incantation.TEAM_PLAYERS["peer-2"] = (3, 0)
        self.assertEqual(incantation.count_team_players_at_level(2), 2)

    def test_expired_peers_are_dropped(self):
        incantation.TEAM_PLAYERS["peer-1"] = (2, 0)
        incantation.COORDINATION_TURN = incantation.TEAM_STATUS_LIFETIME_TURNS + 1
        self.assertEqual(incantation.count_team_players_at_level(2), 1)
        self.assertNotIn("peer-1", incantation.TEAM_PLAYERS)


class TestIsIncantationLeader(IncantationTestCase):
    def test_alone_is_leader(self):
        self.assertTrue(incantation.is_incantation_leader())

    def test_lower_id_peer_at_same_level_loses_leadership(self):
        incantation.TEAM_PLAYERS["aaa-peer"] = (1, 0)
        self.assertFalse(incantation.is_incantation_leader())

    def test_higher_id_peer_at_same_level_keeps_leadership(self):
        incantation.TEAM_PLAYERS["zzz-peer"] = (1, 0)
        self.assertTrue(incantation.is_incantation_leader())

    def test_peer_at_different_level_is_ignored(self):
        incantation.TEAM_PLAYERS["aaa-peer"] = (2, 0)
        self.assertTrue(incantation.is_incantation_leader())


class TestShareTeamStatus(IncantationTestCase):
    def test_returns_false_without_team_name(self):
        with patch.object(incantation, "send_command_to_server") as sent:
            self.assertFalse(incantation.share_team_status(MagicMock()))
            sent.assert_not_called()

    def test_does_not_broadcast_before_interval_elapses(self):
        incantation.TEAM_NAME = "Team"
        with patch.object(incantation, "send_command_to_server") as sent:
            for _ in range(incantation.TEAM_STATUS_INTERVAL - 1):
                self.assertFalse(incantation.share_team_status(MagicMock()))
            sent.assert_not_called()

    def test_broadcasts_once_interval_elapses(self):
        incantation.TEAM_NAME = "Team"
        with patch.object(incantation, "send_command_to_server") as sent:
            for _ in range(incantation.TEAM_STATUS_INTERVAL - 1):
                incantation.share_team_status(MagicMock())
            self.assertTrue(incantation.share_team_status(MagicMock()))
            sent.assert_called_once_with(
                unittest.mock.ANY, "Broadcast team_status|Team|1|self-id")
            self.assertEqual(incantation.TEAM_STATUS_TURNS, 0)


class TestResetIncantationCalls(IncantationTestCase):
    def test_clears_all_coordination_state(self):
        incantation.LAST_INCANTATION_CALL = (1, 1)
        incantation.LAST_FOLLOWED_CALL = ("peer", 0)
        incantation.HELP_MESSAGE_SENT = True
        incantation.READY_MESSAGE_SENT = True
        incantation.INCANTATION_CALLS["peer"] = (1, 1, 0)
        incantation.READY_PLAYERS["peer"] = 0

        incantation.reset_incantation_calls()

        self.assertIsNone(incantation.LAST_INCANTATION_CALL)
        self.assertIsNone(incantation.LAST_FOLLOWED_CALL)
        self.assertFalse(incantation.HELP_MESSAGE_SENT)
        self.assertFalse(incantation.READY_MESSAGE_SENT)
        self.assertEqual(incantation.INCANTATION_CALLS, {})
        self.assertEqual(incantation.READY_PLAYERS, {})


class TestGetCallToFollow(IncantationTestCase):
    def test_no_calls_returns_none(self):
        self.assertIsNone(incantation.get_call_to_follow())

    def test_returns_lowest_caller_id_at_matching_level(self):
        incantation.INCANTATION_CALLS["zzz"] = (1, 1, 0)
        incantation.INCANTATION_CALLS["aaa"] = (2, 1, 0)
        self.assertEqual(incantation.get_call_to_follow(), ("aaa", 2, 1))

    def test_calls_at_other_level_are_ignored(self):
        incantation.INCANTATION_CALLS["peer"] = (1, 2, 0)
        self.assertIsNone(incantation.get_call_to_follow())

    def test_expired_calls_are_removed(self):
        incantation.INCANTATION_CALLS["peer"] = (1, 1, 0)
        incantation.COORDINATION_TURN = incantation.CALL_LIFETIME_TURNS + 1
        self.assertIsNone(incantation.get_call_to_follow())
        self.assertEqual(incantation.INCANTATION_CALLS, {})


class TestCountReadyPlayers(IncantationTestCase):
    def test_counts_ready_players(self):
        incantation.READY_PLAYERS["peer-1"] = 0
        incantation.READY_PLAYERS["peer-2"] = 0
        self.assertEqual(incantation.count_ready_players(), 2)

    def test_expired_ready_players_are_removed(self):
        incantation.READY_PLAYERS["peer-1"] = 0
        incantation.COORDINATION_TURN = incantation.READY_LIFETIME_TURNS + 1
        self.assertEqual(incantation.count_ready_players(), 0)
        self.assertEqual(incantation.READY_PLAYERS, {})


class TestCallWasAlreadyFollowed(IncantationTestCase):
    def test_unknown_caller_returns_false(self):
        self.assertFalse(incantation.call_was_already_followed("peer"))

    def test_first_follow_returns_false_then_true(self):
        incantation.INCANTATION_CALLS["peer"] = (1, 1, 5)
        self.assertFalse(incantation.call_was_already_followed("peer"))
        self.assertTrue(incantation.call_was_already_followed("peer"))

    def test_new_call_turn_resets_follow_state(self):
        incantation.INCANTATION_CALLS["peer"] = (1, 1, 5)
        incantation.call_was_already_followed("peer")
        incantation.INCANTATION_CALLS["peer"] = (1, 1, 6)
        self.assertFalse(incantation.call_was_already_followed("peer"))


class TestUpdatePlayerLevel(IncantationTestCase):
    def test_unrelated_response_is_ignored(self):
        self.assertFalse(incantation.update_player_level("ko"))
        self.assertEqual(incantation.PLAYER_LEVEL, 1)

    def test_non_numeric_level_is_ignored(self):
        self.assertFalse(incantation.update_player_level("Current level: abc"))
        self.assertEqual(incantation.PLAYER_LEVEL, 1)

    def test_out_of_range_level_is_ignored(self):
        self.assertFalse(incantation.update_player_level("Current level: 9"))
        self.assertEqual(incantation.PLAYER_LEVEL, 1)

    def test_valid_level_updates_and_resets_calls(self):
        incantation.INCANTATION_CALLS["peer"] = (1, 1, 0)
        self.assertTrue(incantation.update_player_level("Current level: 2"))
        self.assertEqual(incantation.PLAYER_LEVEL, 2)
        self.assertEqual(incantation.INCANTATION_CALLS, {})


class TestGetResourcePriority(IncantationTestCase):
    def test_level_needed_resources_come_first(self):
        incantation.PLAYER_LEVEL = 2
        priority = incantation.get_resource_priority()
        self.assertEqual(priority[:3], ["linemate", "deraumere", "sibur"])
        self.assertEqual(set(priority), set(incantation.RESOURCE_PRIORITY))


class TestPlaceMissingStonesForLevel(IncantationTestCase):
    def test_returns_true_when_nothing_missing(self):
        with patch.object(incantation, "send_command_to_server") as sent:
            self.assertTrue(incantation.place_missing_stones_for_level(
                MagicMock(), {}, ["linemate"], 1))
            sent.assert_not_called()

    def test_sets_missing_resource_from_inventory(self):
        inventory = {"linemate": 1}
        with patch.object(incantation, "send_command_to_server", return_value="ok") as sent:
            self.assertTrue(incantation.place_missing_stones_for_level(
                MagicMock(), inventory, [], 1))
            sent.assert_called_once_with(unittest.mock.ANY, "Set linemate")
        self.assertEqual(inventory["linemate"], 0)

    def test_returns_false_when_inventory_lacks_resource(self):
        with patch.object(incantation, "send_command_to_server") as sent:
            self.assertFalse(incantation.place_missing_stones_for_level(
                MagicMock(), {}, [], 1))
            sent.assert_not_called()

    def test_returns_false_when_set_command_fails(self):
        inventory = {"linemate": 1}
        with patch.object(incantation, "send_command_to_server", return_value="ko"):
            self.assertFalse(incantation.place_missing_stones_for_level(
                MagicMock(), inventory, [], 1))


class TestHasIncantationHelpRequirements(IncantationTestCase):
    def test_low_food_returns_false(self):
        incantation.PLAYER_LEVEL = 2
        self.assertFalse(incantation.has_incantation_help_requirements({"food": 1}, [], 2))

    def test_enough_players_already_present_returns_false(self):
        incantation.PLAYER_LEVEL = 2
        incantation.READY_PLAYERS["peer"] = 0
        inventory = {"food": 10, "linemate": 1, "deraumere": 1, "sibur": 1}
        tile = ["player", "player"]
        self.assertFalse(incantation.has_incantation_help_requirements(inventory, tile, 2))

    def test_missing_stones_returns_false(self):
        incantation.PLAYER_LEVEL = 2
        inventory = {"food": 10}
        self.assertFalse(incantation.has_incantation_help_requirements(inventory, [], 2))

    def test_enough_food_and_stones_without_players_returns_true(self):
        incantation.PLAYER_LEVEL = 2
        inventory = {"food": 10, "linemate": 1, "deraumere": 1, "sibur": 1}
        self.assertTrue(incantation.has_incantation_help_requirements(inventory, [], 2))


class TestBroadcastIncantationHelp(IncantationTestCase):
    def test_first_call_records_self_and_broadcasts(self):
        incantation.TEAM_NAME = "Team"
        incantation.PLAYER_LEVEL = 2
        with patch.object(incantation, "send_command_to_server") as sent:
            incantation.broadcast_incantation_help(MagicMock())
            sent.assert_called_once_with(
                unittest.mock.ANY, "Broadcast incantation_request|Team|2|self-id")
        self.assertIn("self-id", incantation.INCANTATION_CALLS)
        self.assertTrue(incantation.HELP_MESSAGE_SENT)

    def test_second_call_toggles_off_and_looks(self):
        incantation.TEAM_NAME = "Team"
        with patch.object(incantation, "send_command_to_server") as sent:
            incantation.broadcast_incantation_help(MagicMock())
            incantation.broadcast_incantation_help(MagicMock())
            calls = [call.args[1] for call in sent.call_args_list]
        self.assertEqual(calls[1], "Look")
        self.assertFalse(incantation.HELP_MESSAGE_SENT)


class TestSayReadyForIncantation(IncantationTestCase):
    def test_first_call_broadcasts_ready(self):
        incantation.TEAM_NAME = "Team"
        with patch.object(incantation, "send_command_to_server") as sent:
            incantation.say_ready_for_incantation(MagicMock(), "peer")
            sent.assert_called_once_with(
                unittest.mock.ANY, "Broadcast incantation_ready|Team|1|peer|self-id")
        self.assertTrue(incantation.READY_MESSAGE_SENT)

    def test_second_call_toggles_off_and_looks(self):
        incantation.TEAM_NAME = "Team"
        with patch.object(incantation, "send_command_to_server") as sent:
            incantation.say_ready_for_incantation(MagicMock(), "peer")
            incantation.say_ready_for_incantation(MagicMock(), "peer")
            calls = [call.args[1] for call in sent.call_args_list]
        self.assertEqual(calls[1], "Look")
        self.assertFalse(incantation.READY_MESSAGE_SENT)


class TestRequestIncantationHelp(IncantationTestCase):
    def test_solo_level_returns_false(self):
        incantation.PLAYER_LEVEL = 1
        self.assertFalse(incantation.request_incantation_help(MagicMock(), {}, []))

    def test_not_leader_returns_false(self):
        incantation.PLAYER_LEVEL = 2
        incantation.TEAM_PLAYERS["aaa-peer"] = (2, 0)
        self.assertFalse(incantation.request_incantation_help(MagicMock(), {"food": 10}, []))

    def test_existing_call_from_someone_else_returns_false(self):
        incantation.PLAYER_LEVEL = 2
        incantation.INCANTATION_CALLS["other-peer"] = (1, 2, 0)
        self.assertFalse(incantation.request_incantation_help(MagicMock(), {"food": 10}, []))

    def test_leader_without_requirements_returns_false(self):
        incantation.PLAYER_LEVEL = 2
        self.assertFalse(incantation.request_incantation_help(MagicMock(), {"food": 1}, []))

    def test_leader_with_requirements_broadcasts(self):
        incantation.PLAYER_LEVEL = 2
        incantation.TEAM_NAME = "Team"
        inventory = {"food": 10, "linemate": 1, "deraumere": 1, "sibur": 1}
        with patch.object(incantation, "send_command_to_server") as sent:
            self.assertTrue(incantation.request_incantation_help(MagicMock(), inventory, []))
            sent.assert_called_once_with(
                unittest.mock.ANY, "Broadcast incantation_request|Team|2|self-id")


class TestFollowIncantationCall(IncantationTestCase):
    def test_no_call_returns_false(self):
        self.assertFalse(incantation.follow_incantation_call(MagicMock(), {"food": 20}))

    def test_own_call_is_not_followed(self):
        incantation.INCANTATION_CALLS["self-id"] = (0, 1, 0)
        self.assertFalse(incantation.follow_incantation_call(MagicMock(), {"food": 20}))

    def test_low_food_returns_false(self):
        incantation.INCANTATION_CALLS["peer"] = (3, 1, 0)
        self.assertFalse(incantation.follow_incantation_call(MagicMock(), {"food": 1}))

    def test_legacy_call_with_wrong_level_returns_false(self):
        incantation.PLAYER_LEVEL = 2
        incantation.LAST_INCANTATION_CALL = (3, 1)
        self.assertFalse(incantation.follow_incantation_call(MagicMock(), {"food": 20}))

    def test_forward_directions_move_forward(self):
        for direction in (1, 2, 8):
            with self.subTest(direction=direction):
                incantation.reset_incantation_calls()
                incantation.INCANTATION_CALLS["peer"] = (direction, 1, 0)
                with patch.object(incantation, "send_command_to_server") as sent:
                    self.assertTrue(incantation.follow_incantation_call(MagicMock(), {"food": 20}))
                    sent.assert_called_once_with(unittest.mock.ANY, "Forward")

    def test_left_directions_turn_left(self):
        for direction in (3, 4, 5):
            with self.subTest(direction=direction):
                incantation.reset_incantation_calls()
                incantation.INCANTATION_CALLS["peer"] = (direction, 1, 0)
                with patch.object(incantation, "send_command_to_server") as sent:
                    self.assertTrue(incantation.follow_incantation_call(MagicMock(), {"food": 20}))
                    sent.assert_called_once_with(unittest.mock.ANY, "Left")

    def test_right_directions_turn_right(self):
        for direction in (6, 7):
            with self.subTest(direction=direction):
                incantation.reset_incantation_calls()
                incantation.INCANTATION_CALLS["peer"] = (direction, 1, 0)
                with patch.object(incantation, "send_command_to_server") as sent:
                    self.assertTrue(incantation.follow_incantation_call(MagicMock(), {"food": 20}))
                    sent.assert_called_once_with(unittest.mock.ANY, "Right")

    def test_on_caller_tile_sends_ready_message(self):
        incantation.TEAM_NAME = "Team"
        incantation.INCANTATION_CALLS["peer"] = (0, 1, 0)
        with patch.object(incantation, "send_command_to_server") as sent:
            self.assertTrue(incantation.follow_incantation_call(MagicMock(), {"food": 20}))
            sent.assert_called_once_with(
                unittest.mock.ANY, "Broadcast incantation_ready|Team|1|peer|self-id")

    def test_legacy_call_without_caller_looks(self):
        incantation.LAST_INCANTATION_CALL = (0, 1)
        with patch.object(incantation, "send_command_to_server") as sent:
            self.assertTrue(incantation.follow_incantation_call(MagicMock(), {"food": 20}))
            sent.assert_called_once_with(unittest.mock.ANY, "Look")

    def test_already_followed_call_looks_again(self):
        incantation.INCANTATION_CALLS["peer"] = (3, 1, 0)
        incantation.call_was_already_followed("peer")
        with patch.object(incantation, "send_command_to_server") as sent:
            self.assertTrue(incantation.follow_incantation_call(MagicMock(), {"food": 20}))
            sent.assert_called_once_with(unittest.mock.ANY, "Look")


if __name__ == "__main__":
    unittest.main()
