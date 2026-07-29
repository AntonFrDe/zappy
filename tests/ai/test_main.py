#!/usr/bin/env python3
import importlib.util
import sys
import unittest
from pathlib import Path
from unittest.mock import MagicMock, patch

MAIN_PY = Path(__file__).resolve().parents[2] / "src" / "ai" / "main.py"
SPEC = importlib.util.spec_from_file_location("zappy_ai_main", MAIN_PY)
ai = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(ai)


class TestReadAndGetArguments(unittest.TestCase):
    def test_returns_value_following_flag(self):
        argv = ["main.py", "-p", "4242"]
        with patch.object(sys, "argv", argv):
            self.assertEqual(ai.read_and_get_arguments("-p"), "4242")

    def test_returns_default_when_flag_absent(self):
        argv = ["main.py", "-p", "4242"]
        with patch.object(sys, "argv", argv):
            self.assertEqual(ai.read_and_get_arguments("-h", "localhost"), "localhost")

    def test_returns_none_when_flag_absent_and_no_default(self):
        argv = ["main.py"]
        with patch.object(sys, "argv", argv):
            self.assertIsNone(ai.read_and_get_arguments("-n"))

    def test_flag_as_last_argument_raises_index_error(self):
        argv = ["main.py", "-p"]
        with patch.object(sys, "argv", argv):
            with self.assertRaises(IndexError):
                ai.read_and_get_arguments("-p")


class TestParseInventoryResponse(unittest.TestCase):
    def test_parses_multiple_items(self):
        result = ai.parse_inventory_response("[food 5, linemate 2]")
        self.assertEqual(result, {"food": 5, "linemate": 2})

    def test_empty_inventory_returns_empty_dict(self):
        self.assertEqual(ai.parse_inventory_response("[]"), {})

    def test_item_missing_quantity_is_ignored(self):
        self.assertEqual(ai.parse_inventory_response("[food]"), {})

    def test_non_digit_quantity_is_ignored(self):
        self.assertEqual(ai.parse_inventory_response("[food abc]"), {})

    def test_negative_quantity_is_ignored(self):
        # str.isdigit() rejects the leading '-', so negative counts are
        # silently dropped by the current implementation.
        self.assertEqual(ai.parse_inventory_response("[food -5]"), {})

    def test_extra_whitespace_is_tolerated(self):
        result = ai.parse_inventory_response("[  food   5 , linemate 2 ]")
        self.assertEqual(result, {"food": 5, "linemate": 2})


class TestParseLookResponse(unittest.TestCase):
    def test_parses_tiles_and_items(self):
        result = ai.parse_look_response("[food linemate,,player,deraumere]")
        self.assertEqual(result, [["food", "linemate"], [], ["player"], ["deraumere"]])


class TestChooseDirectionTowardItem(unittest.TestCase):
    def test_left_food_returns_left(self):
        self.assertEqual(ai.choose_direction_toward_item("[,food,,]", "food"), "Left")

    def test_center_food_returns_forward(self):
        self.assertEqual(ai.choose_direction_toward_item("[,,food,]", "food"), "Forward")

    def test_right_food_returns_right(self):
        self.assertEqual(ai.choose_direction_toward_item("[,,,food]", "food"), "Right")

    def test_current_tile_item_returns_none(self):
        self.assertIsNone(ai.choose_direction_toward_item("[food,,,]", "food"))

    def test_missing_item_returns_none(self):
        self.assertIsNone(ai.choose_direction_toward_item("[linemate,,,]", "food"))


class TestReceiveResponseFromServer(unittest.TestCase):
    def _connection_yielding(self, payload):
        connection = MagicMock()
        connection.recv.side_effect = [bytes([byte]) for byte in payload]
        return connection

    def test_reads_until_newline_and_strips_it(self):
        connection = self._connection_yielding(b"ok\n")
        self.assertEqual(ai.receive_response_from_server(connection), "ok")

    def test_dead_response_exits_zero(self):
        connection = self._connection_yielding(b"dead\n")
        with self.assertRaises(SystemExit) as ctx:
            ai.receive_response_from_server(connection)
        self.assertEqual(ctx.exception.code, 0)

    def test_connection_closed_exits_84(self):
        connection = MagicMock()
        connection.recv.side_effect = [b""]
        with self.assertRaises(SystemExit) as ctx:
            ai.receive_response_from_server(connection)
        self.assertEqual(ctx.exception.code, 84)


class TestSendCommandToServer(unittest.TestCase):
    def test_sends_command_with_trailing_newline_and_returns_response(self):
        connection = MagicMock()
        connection.recv.side_effect = [bytes([byte]) for byte in b"ok\n"]
        result = ai.send_command_to_server(connection, "Forward")
        connection.sendall.assert_called_once_with(b"Forward\n")
        self.assertEqual(result, "ok")


class TestKeepPlayerAlive(unittest.TestCase):
    def test_returns_false_when_food_above_threshold(self):
        with patch.object(ai, "send_command_to_server", return_value="[food 9]") as sent:
            self.assertFalse(ai.keep_player_alive(MagicMock()))
            sent.assert_called_once_with(unittest.mock.ANY, "Inventory")

    def test_takes_food_when_visible_on_current_tile(self):
        responses = iter(["[food 2]", "[food]", "ok"])
        with patch.object(ai, "send_command_to_server", side_effect=lambda _c, _cmd: next(responses)) as sent:
            self.assertTrue(ai.keep_player_alive(MagicMock()))
            calls = [call.args[1] for call in sent.call_args_list]
            self.assertEqual(calls, ["Inventory", "Look", "Take food"])

    def test_moves_when_no_food_visible(self):
        responses = iter(["[food 2]", "[linemate 1]", "ok"])
        with patch.object(ai, "send_command_to_server", side_effect=lambda _c, _cmd: next(responses)) as sent, \
             patch.object(ai, "choice", return_value="Left") as choice_mock:
            self.assertTrue(ai.keep_player_alive(MagicMock()))
            calls = [call.args[1] for call in sent.call_args_list]
            self.assertEqual(calls, ["Inventory", "Look", "Left"])
            choice_mock.assert_called_once_with(["Forward", "Left", "Right"])


class TestCountVisibleItem(unittest.TestCase):
    def test_counts_multiple_occurrences(self):
        self.assertEqual(ai.count_visible_item(["food", "player", "food"], "food"), 2)

    def test_returns_zero_when_absent(self):
        self.assertEqual(ai.count_visible_item(["player"], "food"), 0)

    def test_empty_tile_returns_zero(self):
        self.assertEqual(ai.count_visible_item([], "food"), 0)


class TestChooseExplorationCommand(unittest.TestCase):
    def test_player_takes_priority(self):
        self.assertEqual(ai.choose_exploration_command(["player", "player", "egg"]), "Right")

    def test_egg_takes_priority_over_food(self):
        self.assertEqual(ai.choose_exploration_command(["egg", "food"]), "Left")

    def test_food_returns_forward(self):
        self.assertEqual(ai.choose_exploration_command(["food"]), "Forward")

    def test_linemate_returns_forward(self):
        self.assertEqual(ai.choose_exploration_command(["linemate"]), "Forward")

    def test_deraumere_returns_forward(self):
        self.assertEqual(ai.choose_exploration_command(["deraumere"]), "Forward")

    def test_empty_tile_returns_forward_without_choosing(self):
        with patch.object(ai, "choice") as choice_mock:
            self.assertEqual(ai.choose_exploration_command([]), "Forward")
            choice_mock.assert_not_called()

    def test_unknown_tile_picks_from_all_three(self):
        with patch.object(ai, "choice", return_value="Forward") as choice_mock:
            self.assertEqual(ai.choose_exploration_command(["sibur"]), "Forward")
            choice_mock.assert_called_once_with(["Forward", "Left", "Right"])


class TestShouldTakeFood(unittest.TestCase):
    def test_no_food_visible_returns_false(self):
        self.assertFalse(ai.should_take_food(["player"], {}))

    def test_food_visible_and_inventory_low_returns_true(self):
        self.assertTrue(ai.should_take_food(["food"], {"food": 3}))

    def test_many_food_on_tile_returns_true_even_if_full(self):
        self.assertTrue(ai.should_take_food(["food", "food", "food"], {"food": 20}))

    def test_tile_only_food_returns_true_even_if_full(self):
        self.assertTrue(ai.should_take_food(["food"], {"food": 20}))

    def test_full_inventory_and_mixed_tile_returns_false(self):
        self.assertFalse(ai.should_take_food(["food", "linemate"], {"food": 20}))


class TestShouldCollectStone(unittest.TestCase):
    def test_linemate_present_returns_true(self):
        self.assertTrue(ai.should_collect_stone(["linemate"]))

    def test_deraumere_present_returns_true(self):
        self.assertTrue(ai.should_collect_stone(["deraumere"]))

    def test_neither_returns_false(self):
        self.assertFalse(ai.should_collect_stone(["food"]))


class TestShouldWaitBeforeIncantation(unittest.TestCase):
    def test_no_food_returns_true(self):
        self.assertTrue(ai.should_wait_before_incantation({"food": 0, "linemate": 5}))

    def test_low_food_returns_true(self):
        self.assertTrue(ai.should_wait_before_incantation({"food": 1, "linemate": 5}))

    def test_no_linemate_returns_true(self):
        self.assertTrue(ai.should_wait_before_incantation({"food": 10, "linemate": 0}))

    def test_enough_food_and_linemate_returns_false(self):
        self.assertFalse(ai.should_wait_before_incantation({"food": 4, "linemate": 1}))


class TestCanTryLevelTwo(unittest.TestCase):
    def test_low_food_returns_false(self):
        self.assertFalse(ai.can_try_level_two({"food": 1}, []))

    def test_enough_food_and_linemate_in_inventory_returns_true(self):
        self.assertTrue(ai.can_try_level_two({"food": 2, "linemate": 1}, []))

    def test_enough_food_and_linemate_on_tile_returns_true(self):
        self.assertTrue(ai.can_try_level_two({"food": 2}, ["linemate"]))

    def test_enough_food_but_no_linemate_anywhere_returns_false(self):
        self.assertFalse(ai.can_try_level_two({"food": 2}, []))


class TestShouldSkipExtraStone(unittest.TestCase):
    def test_extra_linemate_on_tile_returns_true(self):
        self.assertTrue(ai.should_skip_extra_stone({"linemate": 2}, ["linemate"]))

    def test_extra_deraumere_on_tile_returns_true(self):
        self.assertTrue(ai.should_skip_extra_stone({"deraumere": 2}, ["deraumere"]))

    def test_extra_sibur_on_tile_returns_true(self):
        self.assertTrue(ai.should_skip_extra_stone({"sibur": 2}, ["sibur"]))

    def test_not_enough_in_inventory_returns_false(self):
        self.assertFalse(ai.should_skip_extra_stone({"linemate": 1}, ["linemate"]))


class TestShouldTurnNearPlayers(unittest.TestCase):
    def test_more_than_one_player_returns_true(self):
        self.assertTrue(ai.should_turn_near_players(["player", "player"]))

    def test_one_or_no_player_returns_false(self):
        self.assertFalse(ai.should_turn_near_players(["player"]))
        self.assertFalse(ai.should_turn_near_players([]))


class TestShouldCollectRareStone(unittest.TestCase):
    def test_sibur_returns_true(self):
        self.assertTrue(ai.should_collect_rare_stone(["sibur"]))

    def test_mendiane_returns_true(self):
        self.assertTrue(ai.should_collect_rare_stone(["mendiane"]))

    def test_phiras_or_thystame_returns_true(self):
        self.assertTrue(ai.should_collect_rare_stone(["phiras"]))
        self.assertTrue(ai.should_collect_rare_stone(["thystame"]))

    def test_none_returns_false(self):
        self.assertFalse(ai.should_collect_rare_stone(["food"]))


class TestShouldFollowEgg(unittest.TestCase):
    def test_egg_present_returns_true(self):
        self.assertTrue(ai.should_follow_egg(["egg"]))

    def test_egg_absent_returns_false(self):
        self.assertFalse(ai.should_follow_egg([]))


class TestShouldCheckInventory(unittest.TestCase):
    def test_player_without_food_returns_true(self):
        self.assertTrue(ai.should_check_inventory(["player", "player"]))

    def test_player_with_food_returns_false(self):
        self.assertFalse(ai.should_check_inventory(["player", "food"]))

    def test_no_player_returns_false(self):
        self.assertFalse(ai.should_check_inventory(["food"]))


class TestShouldTurnOnBusyTile(unittest.TestCase):
    def test_many_items_returns_true(self):
        self.assertTrue(ai.should_turn_on_busy_tile(["food", "linemate", "sibur", "egg", "player"]))

    def test_player_and_more_than_two_items_returns_true(self):
        self.assertTrue(ai.should_turn_on_busy_tile(["player", "player", "food", "egg"]))

    def test_sparse_tile_returns_false(self):
        self.assertFalse(ai.should_turn_on_busy_tile(["food"]))


class TestShouldTakeLinemateFirst(unittest.TestCase):
    def test_no_linemate_returns_false(self):
        self.assertFalse(ai.should_take_linemate_first(["food"]))

    def test_linemate_with_food_returns_false(self):
        self.assertFalse(ai.should_take_linemate_first(["linemate", "food"]))

    def test_linemate_alone_returns_true(self):
        self.assertTrue(ai.should_take_linemate_first(["linemate"]))


class TestShouldTakeDeraumereFirst(unittest.TestCase):
    def test_no_deraumere_returns_false(self):
        self.assertFalse(ai.should_take_deraumere_first(["food"], {}))

    def test_already_has_enough_returns_false(self):
        self.assertFalse(ai.should_take_deraumere_first(["deraumere"], {"deraumere": 2}))

    def test_food_present_but_low_inventory_food_returns_false(self):
        self.assertFalse(ai.should_take_deraumere_first(["deraumere", "food"], {"food": 2}))

    def test_player_present_returns_false(self):
        self.assertFalse(ai.should_take_deraumere_first(["deraumere", "player", "player"], {}))

    def test_clear_tile_returns_true(self):
        self.assertTrue(ai.should_take_deraumere_first(["deraumere"], {"deraumere": 1}))


class TestShouldTakeSiburFirst(unittest.TestCase):
    def test_no_sibur_returns_false(self):
        self.assertFalse(ai.should_take_sibur_first(["food"], {}))

    def test_already_has_enough_returns_false(self):
        self.assertFalse(ai.should_take_sibur_first(["sibur"], {"sibur": 2}))

    def test_player_present_returns_false(self):
        self.assertFalse(ai.should_take_sibur_first(["sibur", "player", "player"], {}))

    def test_clear_tile_returns_true(self):
        self.assertTrue(ai.should_take_sibur_first(["sibur"], {}))


class TestShouldTakeMendianeFirst(unittest.TestCase):
    def test_no_mendiane_returns_false(self):
        self.assertFalse(ai.should_take_mendiane_first(["food"], {}))

    def test_already_has_enough_returns_false(self):
        self.assertFalse(ai.should_take_mendiane_first(["mendiane"], {"mendiane": 2}))

    def test_player_present_returns_false(self):
        self.assertFalse(ai.should_take_mendiane_first(["mendiane", "player", "player"], {}))

    def test_clear_tile_returns_true(self):
        self.assertTrue(ai.should_take_mendiane_first(["mendiane"], {}))


class TestShouldTakePhirasFirst(unittest.TestCase):
    def test_no_phiras_returns_false(self):
        self.assertFalse(ai.should_take_phiras_first(["food"], {}))

    def test_already_has_one_returns_false(self):
        self.assertFalse(ai.should_take_phiras_first(["phiras"], {"phiras": 1}))

    def test_clear_tile_returns_true(self):
        self.assertTrue(ai.should_take_phiras_first(["phiras"], {}))


class TestShouldTakeThystameFirst(unittest.TestCase):
    def test_no_thystame_returns_false(self):
        self.assertFalse(ai.should_take_thystame_first(["food"], {}))

    def test_already_has_one_returns_false(self):
        self.assertFalse(ai.should_take_thystame_first(["thystame"], {"thystame": 1}))

    def test_player_present_returns_false(self):
        self.assertFalse(ai.should_take_thystame_first(["thystame", "player", "player"], {}))

    def test_clear_tile_returns_true(self):
        self.assertTrue(ai.should_take_thystame_first(["thystame"], {}))


class TestShouldAvoidBusyResourceTile(unittest.TestCase):
    def test_no_player_returns_false(self):
        self.assertFalse(ai.should_avoid_busy_resource_tile(["linemate"]))

    def test_player_with_linemate_returns_true(self):
        self.assertTrue(ai.should_avoid_busy_resource_tile(["player", "player", "linemate"]))

    def test_player_with_deraumere_returns_true(self):
        self.assertTrue(ai.should_avoid_busy_resource_tile(["player", "player", "deraumere"]))

    def test_player_with_sibur_or_mendiane_returns_true(self):
        self.assertTrue(ai.should_avoid_busy_resource_tile(["player", "player", "sibur"]))
        self.assertTrue(ai.should_avoid_busy_resource_tile(["player", "player", "mendiane"]))

    def test_player_alone_returns_false(self):
        self.assertFalse(ai.should_avoid_busy_resource_tile(["player"]))


class TestCollectVisibleResource(unittest.TestCase):
    def test_takes_first_matching_resource_in_priority_order(self):
        with patch.object(ai, "send_command_to_server") as sent:
            self.assertTrue(ai.collect_visible_resource(MagicMock(), ["thystame", "linemate"]))
            sent.assert_called_once_with(unittest.mock.ANY, "Take linemate")

    def test_no_resource_returns_false(self):
        with patch.object(ai, "send_command_to_server") as sent:
            self.assertFalse(ai.collect_visible_resource(MagicMock(), ["food", "player"]))
            sent.assert_not_called()


class TestTryFirstIncantation(unittest.TestCase):
    def test_no_linemate_anywhere_returns_false_without_calls(self):
        with patch.object(ai, "send_command_to_server") as sent:
            self.assertFalse(ai.try_first_incantation(MagicMock(), {"linemate": 0}, []))
            sent.assert_not_called()

    def test_drops_linemate_then_succeeds(self):
        responses = iter(["ok", "Elevation underway"])
        with patch.object(ai, "send_command_to_server", side_effect=lambda _c, _cmd: next(responses)) as sent, \
             patch.object(ai, "receive_response_from_server", return_value="Current level: 2"):
            self.assertTrue(ai.try_first_incantation(MagicMock(), {"linemate": 1}, []))
            calls = [call.args[1] for call in sent.call_args_list]
            self.assertEqual(calls, ["Set linemate", "Incantation"])

    def test_set_linemate_failure_returns_false(self):
        with patch.object(ai, "send_command_to_server", return_value="ko") as sent:
            self.assertFalse(ai.try_first_incantation(MagicMock(), {"linemate": 1}, []))
            sent.assert_called_once_with(unittest.mock.ANY, "Set linemate")

    def test_linemate_already_on_tile_skips_set(self):
        with patch.object(ai, "send_command_to_server", return_value="ko") as sent:
            self.assertFalse(ai.try_first_incantation(MagicMock(), {}, ["linemate"]))
            sent.assert_called_once_with(unittest.mock.ANY, "Incantation")

    def test_incantation_refused_returns_false(self):
        with patch.object(ai, "send_command_to_server", return_value="ko"):
            self.assertFalse(ai.try_first_incantation(MagicMock(), {}, ["linemate"]))

    def test_elevation_underway_without_level_up_returns_false(self):
        with patch.object(ai, "send_command_to_server", return_value="Elevation underway"), \
             patch.object(ai, "receive_response_from_server", return_value="dead"):
            self.assertFalse(ai.try_first_incantation(MagicMock(), {}, ["linemate"]))


class TestTryLevelTwoIncantation(unittest.TestCase):
    def test_not_ready_returns_false_without_calls(self):
        with patch.object(ai, "send_command_to_server") as sent:
            self.assertFalse(ai.try_level_two_incantation(MagicMock(), {"food": 0}, []))
            sent.assert_not_called()

    def test_multiple_players_turns_instead_of_incanting(self):
        with patch.object(ai, "send_command_to_server", return_value="ok") as sent:
            inventory = {"food": 5, "linemate": 1}
            self.assertTrue(ai.try_level_two_incantation(MagicMock(), inventory, ["player", "player"]))
            sent.assert_called_once_with(unittest.mock.ANY, "Right")

    def test_delegates_to_first_incantation_when_alone(self):
        inventory = {"food": 5, "linemate": 1}
        with patch.object(ai, "try_first_incantation", return_value=True) as fallback:
            self.assertTrue(ai.try_level_two_incantation(MagicMock(), inventory, []))
            fallback.assert_called_once()


class TestMoveWhenFoodIsLow(unittest.TestCase):
    def test_enough_food_returns_false(self):
        with patch.object(ai, "send_command_to_server") as sent:
            self.assertFalse(ai.move_when_food_is_low(MagicMock(), {"food": 3}, []))
            sent.assert_not_called()

    def test_low_food_takes_visible_food(self):
        with patch.object(ai, "send_command_to_server") as sent:
            self.assertTrue(ai.move_when_food_is_low(MagicMock(), {"food": 1}, ["food"]))
            sent.assert_called_once_with(unittest.mock.ANY, "Take food")

    def test_low_food_moves_forward_when_no_food_visible(self):
        with patch.object(ai, "send_command_to_server") as sent:
            self.assertTrue(ai.move_when_food_is_low(MagicMock(), {"food": 1}, []))
            sent.assert_called_once_with(unittest.mock.ANY, "Forward")


class TestTurnOnEmptyTile(unittest.TestCase):
    def test_non_empty_tile_returns_false(self):
        with patch.object(ai, "send_command_to_server") as sent:
            self.assertFalse(ai.turn_on_empty_tile(MagicMock(), ["food"]))
            sent.assert_not_called()

    def test_empty_tile_turns_right(self):
        with patch.object(ai, "send_command_to_server") as sent:
            self.assertTrue(ai.turn_on_empty_tile(MagicMock(), []))
            sent.assert_called_once_with(unittest.mock.ANY, "Right")


class TestSholdMoveAfterLook(unittest.TestCase):
    def test_food_on_tile_returns_false(self):
        with patch.object(ai, "send_command_to_server") as sent:
            self.assertFalse(ai.shold_move_after_look(MagicMock(), ["food"]))
            sent.assert_not_called()

    def test_collectible_stone_returns_false(self):
        with patch.object(ai, "send_command_to_server") as sent:
            self.assertFalse(ai.shold_move_after_look(MagicMock(), ["linemate"]))
            sent.assert_not_called()

    def test_large_tile_returns_false(self):
        with patch.object(ai, "send_command_to_server") as sent:
            self.assertFalse(ai.shold_move_after_look(MagicMock(), ["egg", "player"]))
            sent.assert_not_called()

    def test_small_empty_tile_moves_forward(self):
        with patch.object(ai, "send_command_to_server") as sent:
            self.assertTrue(ai.shold_move_after_look(MagicMock(), []))
            sent.assert_called_once_with(unittest.mock.ANY, "Forward")


class TestIfLowFoodSeekFood(unittest.TestCase):
    def test_enough_food_returns_false(self):
        with patch.object(ai, "send_command_to_server") as sent:
            self.assertFalse(ai.if_low_food_seek_food(MagicMock(), {"food": 9}, ["food"]))
            sent.assert_not_called()

    def test_no_food_on_tile_returns_false(self):
        with patch.object(ai, "send_command_to_server") as sent:
            self.assertFalse(ai.if_low_food_seek_food(MagicMock(), {"food": 1}, []))
            sent.assert_not_called()

    def test_low_food_with_player_takes_food(self):
        with patch.object(ai, "send_command_to_server") as sent:
            self.assertTrue(ai.if_low_food_seek_food(MagicMock(), {"food": 1}, ["food", "player"]))
            sent.assert_called_once_with(unittest.mock.ANY, "Take food")

    def test_low_food_without_player_still_takes_food(self):
        with patch.object(ai, "send_command_to_server") as sent:
            self.assertTrue(ai.if_low_food_seek_food(MagicMock(), {"food": 1}, ["food"]))
            sent.assert_called_once_with(unittest.mock.ANY, "Take food")


class TestTryCreateTeamSlot(unittest.TestCase):
    def test_not_enough_food_returns_false(self):
        with patch.object(ai, "send_command_to_server") as sent:
            self.assertFalse(ai.try_create_team_slot(MagicMock(), {"food": 1}, []))
            sent.assert_not_called()

    def test_non_digit_connect_nbr_returns_false(self):
        with patch.object(ai, "send_command_to_server", return_value="ko") as sent:
            inventory = {"food": 8}
            self.assertFalse(ai.try_create_team_slot(MagicMock(), inventory, []))
            sent.assert_called_once_with(unittest.mock.ANY, "Connect_nbr")

    def test_slots_already_available_returns_false(self):
        with patch.object(ai, "send_command_to_server", return_value="2") as sent:
            inventory = {"food": 8}
            self.assertFalse(ai.try_create_team_slot(MagicMock(), inventory, []))
            sent.assert_called_once_with(unittest.mock.ANY, "Connect_nbr")

    def test_no_slots_forks(self):
        responses = iter(["0", "ok"])
        with patch.object(ai, "send_command_to_server", side_effect=lambda _c, _cmd: next(responses)) as sent:
            inventory = {"food": 8}
            self.assertTrue(ai.try_create_team_slot(MagicMock(), inventory, []))
            calls = [call.args[1] for call in sent.call_args_list]
            self.assertEqual(calls, ["Connect_nbr", "Fork"])


class TestWaitBeforeIncantation(unittest.TestCase):
    def test_takes_food_on_tile_when_low(self):
        with patch.object(ai, "send_command_to_server") as sent:
            ai.wait_before_incantation(MagicMock(), {"food": 1, "linemate": 0}, [], ["food"])
            sent.assert_called_once_with(unittest.mock.ANY, "Take food")

    def test_takes_linemate_on_tile_when_missing(self):
        with patch.object(ai, "send_command_to_server") as sent:
            ai.wait_before_incantation(MagicMock(), {"food": 5, "linemate": 0}, [], ["linemate"])
            sent.assert_called_once_with(unittest.mock.ANY, "Take linemate")

    def test_moves_forward_when_linemate_visible_in_look_only(self):
        with patch.object(ai, "send_command_to_server") as sent:
            ai.wait_before_incantation(MagicMock(), {"food": 5, "linemate": 0}, ["linemate"], [])
            sent.assert_called_once_with(unittest.mock.ANY, "Forward")

    def test_moves_forward_when_food_visible_in_look_only(self):
        with patch.object(ai, "send_command_to_server") as sent:
            ai.wait_before_incantation(MagicMock(), {"food": 1, "linemate": 5}, ["food"], [])
            sent.assert_called_once_with(unittest.mock.ANY, "Forward")

    def test_explores_when_missing_resources_are_nowhere_visible(self):
        with patch.object(ai, "send_command_to_server") as sent, \
             patch.object(ai, "choose_exploration_command", return_value="Left") as explore:
            ai.wait_before_incantation(MagicMock(), {"food": 1, "linemate": 0}, [], [])
            explore.assert_called_once_with([])
            sent.assert_called_once_with(unittest.mock.ANY, "Left")

    def test_looks_again_when_fully_ready(self):
        with patch.object(ai, "send_command_to_server") as sent:
            ai.wait_before_incantation(MagicMock(), {"food": 5, "linemate": 2}, [], [])
            sent.assert_called_once_with(unittest.mock.ANY, "Look")


if __name__ == "__main__":
    unittest.main()
