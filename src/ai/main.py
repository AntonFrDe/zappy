#!/usr/bin/env python3
import socket
import sys
from pathlib import Path

AI_DIR = Path(__file__).resolve().parent
PROJECT_AI_DIR = AI_DIR / "src" / "ai"
for import_path in (AI_DIR, PROJECT_AI_DIR):
    if import_path.exists() and str(import_path) not in sys.path:
        sys.path.insert(0, str(import_path))

from bot import run_ai_loop
from client import ANSWER_WAIT, CONNECT_WAIT, join_team, stop_with_error
from coordination import set_team_name
from spawner import set_start_command

ARGUMENTS = {"-p", "-n", "-h", "-i"}


def print_usage():

    print("USAGE: ./zappy_ai -p port -n name -h machine")
    print("  -p port     port number")
    print("  -n name     name of the team")
    print("  -h machine  name of the machine (default: localhost)")


def read_settings():

    arguments = sys.argv[1:]

    if len(arguments) % 2 != 0:
        stop_with_error("every option requires a value")

    values = {"-h": "localhost", "-i": "1"}

    for flag, value in zip(arguments[::2], arguments[1::2]):
        if flag not in ARGUMENTS:
            stop_with_error("unknown option " + flag)
        if not value or value.startswith("-"):
            stop_with_error("missing value for " + flag)
        values[flag] = value

    if "-p" not in values or "-n" not in values:
        stop_with_error("-p and -n are required")
    try:
        port = int(values["-p"])
        player_number = int(values["-i"])
    except ValueError:
        stop_with_error("port and player number must be numbers")
    if not 1 <= port <= 65535:
        stop_with_error("port is out of range")
    if player_number not in (1, 2):
        stop_with_error("player number is out of range")

    return values["-h"], port, values["-n"], player_number


def main():

    if "--help" in sys.argv[1:]:
        print_usage()
        return

    host, port, team, player_number = read_settings()
    set_team_name(team)
    set_start_command(host, port, team, player_number)

    try:
        connection = socket.create_connection((host, port), timeout=CONNECT_WAIT)
    except OSError:
        stop_with_error("could not connect to the server")
    with connection:
        connection.settimeout(ANSWER_WAIT)
        join_team(connection, team)
        run_ai_loop(connection)


if __name__ == "__main__":
    main()
