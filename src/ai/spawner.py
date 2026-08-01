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
RETRY_AFTER = 0
SPAWN_RETRY_DELAY = 2
TEAM_SIZE = 2
PRESENCE_INTERVAL_TURNS = 4
PRESENCE_TIMEOUT_TURNS = 12
ELECTION_DELAY_TURNS = 2
ELECTION_STARTED_TURN = 0
LAST_PRESENCE_TURN = -PRESENCE_INTERVAL_TURNS


def set_start_command(host, port, team):

    global START_COMMAND, ELECTION_STARTED_TURN, LAST_PRESENCE_TURN

    START_COMMAND = [sys.executable, str(AI_MAIN), "-p", str(port), "-n", team, "-h", host]
    ELECTION_STARTED_TURN = STATE.coordination_turn
    LAST_PRESENCE_TURN = -PRESENCE_INTERVAL_TURNS
    STATE.team_peers.clear()


def remove_missing_players(current_turn):

    for player_id, last_seen in list(STATE.team_peers.items()):
        if current_turn - last_seen > PRESENCE_TIMEOUT_TURNS:
            del STATE.team_peers[player_id]


def active_team_players(current_turn=None):

    current_turn = STATE.coordination_turn if current_turn is None else current_turn
    remove_missing_players(current_turn)

    return {STATE.player_id, *STATE.team_peers}


def update_team_presence(server_connection):

    global LAST_PRESENCE_TURN

    current_turn = STATE.coordination_turn
    remove_missing_players(current_turn)

    if not STATE.team_name:
        return False
    if (STATE.team_peers
        and current_turn - LAST_PRESENCE_TURN < PRESENCE_INTERVAL_TURNS):
        return False

    message = "|".join(("team_presence", STATE.team_name, STATE.player_id))
    LAST_PRESENCE_TURN = current_turn
    response = send_command_to_server(server_connection, "Broadcast " + message)
    return response == "ok"


def is_team_manager(current_turn=None):

    current_turn = STATE.coordination_turn if current_turn is None else current_turn
    players = active_team_players(current_turn)

    if current_turn - ELECTION_STARTED_TURN < ELECTION_DELAY_TURNS:
        return False

    return STATE.player_id == min(players)


def team_needs_partner(current_turn=None):

    return len(active_team_players(current_turn)) < TEAM_SIZE


def is_excess_team_player(current_turn=None):

    selected_players = sorted(active_team_players(current_turn))[:TEAM_SIZE]

    return STATE.player_id not in selected_players


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

    try:
        CHILD_AI = subprocess.Popen(START_COMMAND, close_fds=True)
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
