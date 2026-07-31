import fcntl
from hashlib import sha256
import subprocess
import sys
import time
from pathlib import Path
from tempfile import gettempdir

from client import send_command_to_server, stop_with_error
from strategy import FOOD_SURVIVAL_LIMIT

AI_MAIN = Path(__file__).resolve().parent / "main.py"
START_COMMAND = []
CHILD_AI = None
PLAYER_NUMBER = 1
RETRY_AFTER = 0
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
    START_COMMAND = [sys.executable, str(AI_MAIN), "-p", str(port), "-n", team, "-h", host]
    lock_key = (host + "\0" + str(port) + "\0" + team).encode()
    lock_id = sha256(lock_key).hexdigest()[:20]
    LOCK_PREFIX = Path(gettempdir()) / ("zappy_ai_" + lock_id)
    PLAYER_SLOT_LOCK = try_acquire_lock("player_" + str(player_number))

    if PLAYER_SLOT_LOCK is None:
        stop_with_error("player slot " + str(player_number) + " is already active")
    if player_number == 1:
        is_team_manager()


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
    slot_lock = try_acquire_lock(
        "player_" + str(get_partner_player_number()))
    if slot_lock is None:
        return False
    release_lock(slot_lock)
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

    if not is_team_manager():
        return False
    if inventory.get("food", 0) < FOOD_SURVIVAL_LIMIT:
        return False
    if not can_start_partner():
        return False
    response = send_command_to_server(server_connection, "Connect_nbr")
    if not response.isdigit():
        return False
    if int(response) > 0:
        return start_partner_ai()
    if send_command_to_server(server_connection, "Fork") != "ok":
        return False
    return start_partner_ai()
