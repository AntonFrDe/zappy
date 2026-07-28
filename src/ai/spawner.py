import subprocess
import sys
import time
from pathlib import Path

from client import send_command_to_server

AI_MAIN = Path(__file__).resolve().parent / "main.py"
START_COMMAND = []
CHILD_AI = None
PLAYER_NUMBER = 1
TEAM_GOAL = 6
BACKUP_PLAYER_NUMBER = TEAM_GOAL + 1
MAIN_PROCESS_ID = None
CHILD_STOPPED_AT = None
RETRY_WAIT = 2


def set_start_command(host, port, team, player_number, main_process_id):

    global START_COMMAND, PLAYER_NUMBER, MAIN_PROCESS_ID

    PLAYER_NUMBER = player_number
    MAIN_PROCESS_ID = main_process_id

    next_player = player_number + 1

    START_COMMAND = [sys.executable, str(AI_MAIN), "-p", str(port), "-n", team, "-h", host, "-i", str(next_player), "--main-pid", str(main_process_id)]


def main_bot_is_running():

    if MAIN_PROCESS_ID is None:
        return True
    return Path("/proc/" + str(MAIN_PROCESS_ID)).exists()


def can_create_bot():

    if PLAYER_NUMBER < TEAM_GOAL:
        return True
    if PLAYER_NUMBER == TEAM_GOAL:
        return not main_bot_is_running()
    return False


def child_is_running():

    global CHILD_AI, CHILD_STOPPED_AT

    if CHILD_AI is None:
        return False
    if CHILD_AI.poll() is None:
        return True
    CHILD_AI = None
    CHILD_STOPPED_AT = time.monotonic()
    return False


def can_start_child():

    if CHILD_STOPPED_AT is None:
        return True
    return time.monotonic() - CHILD_STOPPED_AT >= RETRY_WAIT


def start_new_ai():

    global CHILD_AI, CHILD_STOPPED_AT

    if child_is_running() or not can_start_child() or not START_COMMAND:
        return False
    try:
        CHILD_AI = subprocess.Popen(START_COMMAND, close_fds=True)
    except OSError:
        CHILD_AI = None
        CHILD_STOPPED_AT = time.monotonic()
        return False
    CHILD_STOPPED_AT = None
    return True


def try_create_team_slot(server_connection, inventory, tile):

    if not can_create_bot():
        return False
    if child_is_running() or not can_start_child():
        return False
    if inventory.get("food", 0) < 8:
        return False
    reponse = send_command_to_server(server_connection, "Connect_nbr")
    if not reponse.isdigit():
        return False
    if int(reponse) > 0:
        return start_new_ai()
    if send_command_to_server(server_connection, "Fork") != "ok":
        return False
    start_new_ai()
    return True
