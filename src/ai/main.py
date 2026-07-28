#!/usr/bin/env python3
import os
import socket
import sys
from pathlib import Path
from types import FunctionType as _FunctionType

AI_DIR = Path(__file__).resolve().parent
PROJECT_AI_DIR = AI_DIR / "src" / "ai"
for import_path in (AI_DIR, PROJECT_AI_DIR):
    if import_path.exists() and str(import_path) not in sys.path:
        sys.path.insert(0, str(import_path))

import strategy as _strategy
import client as _client
import incantation as _incantation
import spawner as _spawner
import bot as _bot

from strategy import *
from client import *
from incantation import *
from spawner import *
from bot import *


def _export_functions(module, excluded=()):

    for name, function in vars(module).items():
        if name in excluded or name.startswith("_"):
            continue
        if not isinstance(function, _FunctionType) or function.__module__ != module.__name__:
            continue
        exported_function = _FunctionType(function.__code__, globals(), name,
            function.__defaults__, function.__closure__)
        exported_function.__kwdefaults__ = function.__kwdefaults__
        exported_function.__annotations__ = function.__annotations__
        globals()[name] = exported_function


_export_functions(_strategy)
_export_functions(_client)
_export_functions(_incantation, ("send_command_to_server", "receive_response_from_server"))
_export_functions(_spawner)
_export_functions(_bot)


def read_and_get_arguments(argument_type, default=None):

    if argument_type in sys.argv:
        return sys.argv[sys.argv.index(argument_type) + 1]

    return default


def read_text(argument, default=None):

    try:
        value = read_and_get_arguments(argument, default)
    except IndexError:
        stop_with_error("missing value after " + argument)
    if value is None or value == "" or value.startswith("-"):
        stop_with_error("missing value for " + argument)
    return value


def read_number(argument, default, minimum, maximum):

    value = read_text(argument, default)
    try:
        number = int(value)
    except ValueError:
        stop_with_error(argument + " must be a number")
    if number < minimum or number > maximum:
        stop_with_error(argument + " is out of range")
    return number


def print_usage():

    print("USAGE: ./zappy_ai -p port -n name -h machine")
    print("  -p port     port number")
    print("  -n name     name of the team")
    print("  -h machine  name of the machine (default: localhost)")


def read_settings():

    port = read_number("-p", None, 1, 65535)
    team = read_text("-n")
    host = read_text("-h", "localhost")
    player_number = read_number("-i", "1", 1, BACKUP_PLAYER_NUMBER)
    main_process_id = read_number("--main-pid", str(os.getpid()), 1, sys.maxsize)
    return host, port, team, player_number, main_process_id


def main():

    if "--help" in sys.argv[1:]:
        print_usage()
        return

    host, port, team, player_number, main_process_id = read_settings()

    set_start_command(host, port, team, player_number, main_process_id)
    try:
        server_connection = socket.create_connection((host, port), timeout=CONNECT_WAIT)
    except OSError:
        stop_with_error("could not connect to the server")

    try:
        with server_connection:
            server_connection.settimeout(ANSWER_WAIT)
            join_team(server_connection, team)
            run_ai_loop(server_connection)
    finally:
        SAVED_ANSWERS.pop(server_connection, None)


if __name__ == "__main__":
    main()
