import subprocess
import sys
import time
from pathlib import Path

from client import send_command_to_server
from strategy import FOOD_SURVIVAL_LIMIT
from team import TEAM_SIZE

AI_MAIN = Path(__file__).resolve().parent / "main.py"
START_COMMAND = []
CHILDREN = {}
PLAYER_NUMBER = 1
PLAYER_RETRY_AFTER = {}
SPAWN_RETRY_DELAY = 2


def set_start_command(host, port, team, player_number):

    global START_COMMAND, PLAYER_NUMBER

    PLAYER_NUMBER = player_number
    START_COMMAND = [sys.executable, str(AI_MAIN), "-p", str(port),
        "-n", team, "-h", host]


def get_available_player_number():

    now = time.monotonic()

    for player_number in range(2, TEAM_SIZE + 1):
        if player_number in CHILDREN:
            continue
        if now < PLAYER_RETRY_AFTER.get(player_number, 0):
            continue
        return player_number
    return None


def start_new_ai():

    player_number = get_available_player_number()

    if not START_COMMAND or player_number is None:
        return False
    command = START_COMMAND + ["-i", str(player_number)]
    try:
        child = subprocess.Popen(command, close_fds=True)
    except OSError:
        PLAYER_RETRY_AFTER[player_number] = (time.monotonic()
            + SPAWN_RETRY_DELAY)
        return False
    CHILDREN[player_number] = child
    PLAYER_RETRY_AFTER.pop(player_number, None)
    return True


def remove_dead_children():

    for player_number, child in list(CHILDREN.items()):
        if child.poll() is not None:
            del CHILDREN[player_number]
            PLAYER_RETRY_AFTER[player_number] = (time.monotonic()
                + SPAWN_RETRY_DELAY)


def try_create_team_slot(server_connection, inventory):

    if PLAYER_NUMBER != 1:
        return False
    remove_dead_children()
    if (len(CHILDREN) >= TEAM_SIZE - 1
            or get_available_player_number() is None):
        return False
    if inventory.get("food", 0) < FOOD_SURVIVAL_LIMIT:
        return False
    response = send_command_to_server(server_connection, "Connect_nbr")
    if not response.isdigit():
        return False
    if int(response) > 0:
        return start_new_ai()
    if send_command_to_server(server_connection, "Fork") != "ok":
        return False
    return start_new_ai()
