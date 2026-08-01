import subprocess
import sys
import time
from pathlib import Path

from client import send_command_to_server
from coordination_state import STATE
from strategy import FOOD_SURVIVAL_LIMIT

AI_MAIN = Path(__file__).resolve().parent / "main.py"
START_COMMAND = []
CHILD_AI = None
PLAYER_NUMBER = 1
RETRY_AFTER = 0
SPAWN_RETRY_DELAY = 2
TEAM_SIZE = 2
PRESENCE_INTERVAL = 1
PRESENCE_TIMEOUT = 5
ELECTION_DELAY = 2
ELECTION_STARTED_AT = 0
LAST_PRESENCE_AT = 0


def set_start_command(host, port, team, player_number):

    global START_COMMAND, PLAYER_NUMBER, ELECTION_STARTED_AT, LAST_PRESENCE_AT

    PLAYER_NUMBER = player_number
    START_COMMAND = [sys.executable, str(AI_MAIN), "-p", str(port), "-n", team, "-h", host]
    ELECTION_STARTED_AT = time.monotonic()
    LAST_PRESENCE_AT = 0
    STATE.team_peers.clear()


def remove_missing_players(now):

    for player_id, last_seen in list(STATE.team_peers.items()):
        if now - last_seen > PRESENCE_TIMEOUT:
            del STATE.team_peers[player_id]


def active_team_players(now=None):

    now = time.monotonic() if now is None else now
    remove_missing_players(now)

    return {STATE.player_id, *STATE.team_peers}


def update_team_presence(server_connection):

    global LAST_PRESENCE_AT

    now = time.monotonic()
    remove_missing_players(now)

    if not STATE.team_name or now - LAST_PRESENCE_AT < PRESENCE_INTERVAL:
        return False

    message = "|".join(("team_presence", STATE.team_name, STATE.player_id))
    LAST_PRESENCE_AT = now
    response = send_command_to_server(server_connection, "Broadcast " + message)
    return response == "ok"


def is_team_manager(now=None):

    now = time.monotonic() if now is None else now
    players = active_team_players(now)

    if now - ELECTION_STARTED_AT < ELECTION_DELAY:
        return False

    return STATE.player_id == min(players)


def team_needs_partner(now=None):

    return len(active_team_players(now)) < TEAM_SIZE


def is_excess_team_player(now=None):

    selected_players = sorted(active_team_players(now))[:TEAM_SIZE]

    return STATE.player_id not in selected_players


def get_partner_player_number():

    return 2 if PLAYER_NUMBER == 1 else 1


def can_start_partner():

    global CHILD_AI, RETRY_AFTER

    now = time.monotonic()

    if CHILD_AI is not None:
        if CHILD_AI.poll() is None:
            return False
        CHILD_AI = None
        RETRY_AFTER = now + SPAWN_RETRY_DELAY
        return False
    if now < RETRY_AFTER:
        return False
    return True


def start_partner_ai():

    global CHILD_AI, RETRY_AFTER

    if not START_COMMAND or CHILD_AI is not None:
        return False

    command = START_COMMAND + ["-i", str(get_partner_player_number())]

    try:
        CHILD_AI = subprocess.Popen(command, close_fds=True)
    except OSError:
        CHILD_AI = None
        RETRY_AFTER = time.monotonic() + SPAWN_RETRY_DELAY
        return False
    RETRY_AFTER = 0
    return True


def try_create_team_slot(server_connection, inventory):

    if not is_team_manager() or not team_needs_partner():
        return False
    if inventory.get("food", 0) < FOOD_SURVIVAL_LIMIT:
        return False
    if not can_start_partner():
        return False

    response = send_command_to_server(server_connection, "Connect_nbr")

    if (not response.isdigit() or not is_team_manager() or not team_needs_partner()):
        return False
    if int(response) > 0:
        return start_partner_ai()
    if send_command_to_server(server_connection, "Fork") != "ok":
        return False
    if not is_team_manager() or not team_needs_partner():
        return False

    return start_partner_ai()
