import fcntl
from hashlib import sha256
import subprocess
import sys
import time
from pathlib import Path
from tempfile import gettempdir

from client import send_command_to_server, stop_with_error
from strategy import FOOD_SURVIVAL_LIMIT
from team import TEAM_SIZE

AI_MAIN = Path(__file__).resolve().parent / "main.py"
START_COMMAND = []
CHILDREN = {}
PLAYER_NUMBER = 1
PLAYER_RETRY_AFTER = {}
SPAWN_RETRY_DELAY = 2
LOCK_PREFIX = None
PLAYER_SLOT_LOCK = None
MANAGER_LOCK = None


def try_acquire_lock(lock_name):

    if LOCK_PREFIX is None:
        return None
    lock_file = None
    try:
        lock_file = Path(str(LOCK_PREFIX) + "_" + lock_name + ".lock").open("a+")
        fcntl.flock(lock_file.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
    except OSError:
        if lock_file is not None:
            lock_file.close()
        return None
    return lock_file


def release_lock(lock_file):

    try:
        fcntl.flock(lock_file.fileno(), fcntl.LOCK_UN)
    finally:
        lock_file.close()


def is_team_manager():

    global MANAGER_LOCK

    if MANAGER_LOCK is not None:
        return True
    MANAGER_LOCK = try_acquire_lock("manager")
    return MANAGER_LOCK is not None


def set_start_command(host, port, team, player_number):

    global START_COMMAND, PLAYER_NUMBER, LOCK_PREFIX, PLAYER_SLOT_LOCK

    PLAYER_NUMBER = player_number
    START_COMMAND = [sys.executable, str(AI_MAIN), "-p", str(port),
        "-n", team, "-h", host]
    lock_key = (host + "\0" + str(port) + "\0" + team).encode()
    lock_id = sha256(lock_key).hexdigest()[:20]
    LOCK_PREFIX = Path(gettempdir()) / ("zappy_ai_" + lock_id)
    PLAYER_SLOT_LOCK = try_acquire_lock("player_" + str(player_number))
    if PLAYER_SLOT_LOCK is None:
        stop_with_error("player slot " + str(player_number) + " is already active")
    if player_number == 1:
        is_team_manager()


def get_available_player_number():

    now = time.monotonic()

    for player_number in range(1, TEAM_SIZE + 1):
        if player_number == PLAYER_NUMBER:
            continue
        if player_number in CHILDREN:
            continue
        if now < PLAYER_RETRY_AFTER.get(player_number, 0):
            continue
        slot_lock = try_acquire_lock("player_" + str(player_number))
        if slot_lock is not None:
            release_lock(slot_lock)
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
            PLAYER_RETRY_AFTER[player_number] = (time.monotonic() + SPAWN_RETRY_DELAY)


def try_create_team_slot(server_connection, inventory):

    if not is_team_manager():
        return False
    remove_dead_children()
    if get_available_player_number() is None:
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
