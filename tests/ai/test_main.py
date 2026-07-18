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
        with patch.object(ai, "send_command_to_server", return_value="[food 6]") as sent:
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


if __name__ == "__main__":
    unittest.main()
