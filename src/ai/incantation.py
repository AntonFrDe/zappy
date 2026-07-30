from dataclasses import dataclass
from secrets import token_hex

from client import receive_response_from_server, send_command_to_server
from strategy import (FOOD_SURVIVAL_LIMIT,
    INCANTATION_STONES,
    RESOURCE_PRIORITY,
    count_visible_item,
    get_current_tile,
    has_required_stones_for_level,)
from team import TEAM_SIZE


@dataclass(frozen=True)
class TeamPlayerStatus:
    level: int
    updated_turn: int
    inventory: dict


@dataclass(frozen=True)
class IncantationCall:
    caller_id: str
    direction: int
    level: int
    created_turn: int
    session_id: str = ""
    cohort: tuple = ()


@dataclass(frozen=True)
class ReadyPlayer:
    updated_turn: int
    session_id: str

PLAYER_LEVEL = 1
INCANTATION_PLAYERS = {1: 1, 2: 2, 3: 2, 4: 4, 5: 4, 6: 6, 7: 6}
WINNING_COHORT_SIZE = 6
TEAM_NAME = ""
TEAM_PLAYERS = {}
TEAM_COHORT = ()
LAST_FOLLOWED_CALL = None
PLAYER_ID = token_hex(8)
INCANTATION_CALLS = {}
READY_PLAYERS = {}
CALL_LIFETIME_TURNS = 64
CALL_SESSION_MAX_TURNS = 128
READY_LIFETIME_TURNS = 16
TEAM_STATUS_LIFETIME_TURNS = 128
TEAM_STATUS_INTERVAL = 8
INCANTATION_FOOD_RESERVE = FOOD_SURVIVAL_LIMIT
TEAM_STATUS_TURNS = 0
COORDINATION_TURN = 0
TEAM_STATUS_RESOURCES = ["food"] + RESOURCE_PRIORITY
WAITING_FOR_INCANTATION = False
ARRIVAL_CONFIRMATION_CALLER = None
INCANTATION_SESSION_ID = None
INCANTATION_SESSION_STARTED_TURN = None
MAX_COHORT_SIZE = TEAM_SIZE


def advance_coordination_turn():

    global COORDINATION_TURN

    COORDINATION_TURN += 1


def is_waiting_for_incantation():

    return WAITING_FOR_INCANTATION


def cancel_incantation_wait():

    global WAITING_FOR_INCANTATION, ARRIVAL_CONFIRMATION_CALLER

    WAITING_FOR_INCANTATION = False
    ARRIVAL_CONFIRMATION_CALLER = None


def remember_incantation_call(message):

    try:
        header, payload = message.split(",", 1)
    except ValueError:
        return

    payload = payload.strip()
    header_parts = header.split()
    if len(header_parts) != 2 or header_parts[0] != "message":
        return
    if not header_parts[1].isdigit():
        return

    direction = int(header_parts[1])

    if direction < 0 or direction > 8:
        return

    message_parts = payload.split("|")

    if len(message_parts) >= 4 and message_parts[0] == "team_status":
        save_team_status(message_parts[1], message_parts[2], message_parts[3],
            message_parts[4:])
        return
    if len(message_parts) == 6 and message_parts[0] == "incantation_request":
        if message_parts[1] == TEAM_NAME:
            remember_team_player_activity(message_parts[2], message_parts[3])
            save_incantation_call(direction, message_parts[2], message_parts[3],
                message_parts[4], message_parts[5])
        return
    if len(message_parts) == 6 and message_parts[0] == "incantation_ready":
        if message_parts[1] == TEAM_NAME:
            remember_team_player_activity(message_parts[2], message_parts[4])
            save_ready_player(direction, message_parts[2], message_parts[3],
                message_parts[4], message_parts[5])
        return


def parse_cohort_ids(cohort_text):

    if not cohort_text:
        return ()
    cohort = tuple(dict.fromkeys(cohort_text.split(",")))
    if len(cohort) > MAX_COHORT_SIZE:
        return ()
    if any(not player_id for player_id in cohort):
        return ()
    return cohort


def save_incantation_call(direction, level_text, caller_id,
        session_id="", cohort_text=""):

    if not level_text.isdigit():
        return

    level = int(level_text)

    cohort = parse_cohort_ids(cohort_text)

    if level != PLAYER_LEVEL:
        return
    if cohort and (caller_id not in cohort or PLAYER_ID not in cohort):
        return
    INCANTATION_CALLS[caller_id] = IncantationCall(caller_id, direction,
        level, COORDINATION_TURN, session_id, cohort)


def save_ready_player(direction, level_text, caller_id, player_id, session_id=""):

    if not level_text.isdigit():
        return

    level = int(level_text)

    call = INCANTATION_CALLS.get(PLAYER_ID)

    if level != PLAYER_LEVEL or caller_id != PLAYER_ID or player_id == PLAYER_ID:
        return
    if direction != 0:
        return
    if call is None:
        return
    cohort = call.cohort
    if cohort and player_id not in cohort:
        return
    if call.session_id != session_id:
        return
    READY_PLAYERS[player_id] = ReadyPlayer(COORDINATION_TURN, session_id)


def set_team_name(team_name):

    global TEAM_NAME

    TEAM_NAME = team_name


def parse_team_inventory(values):

    if len(values) != len(TEAM_STATUS_RESOURCES):
        return {}
    if not all(value.isdigit() for value in values):
        return {}
    return {resource: int(value)
        for resource, value in zip(TEAM_STATUS_RESOURCES, values)}


def remember_team_player_activity(level_text, player_id):

    if not level_text.isdigit() or player_id == PLAYER_ID:
        return
    level = int(level_text)
    if level < 1 or level > 8:
        return
    status = TEAM_PLAYERS.get(player_id)
    inventory = status.inventory if status is not None else {}
    TEAM_PLAYERS[player_id] = TeamPlayerStatus(level, COORDINATION_TURN,
        inventory)


def save_team_status(team_name, level_text, player_id, inventory_values=()):

    if team_name != TEAM_NAME or not level_text.isdigit():
        return
    level = int(level_text)
    if level < 1 or level > 8 or player_id == PLAYER_ID:
        return
    inventory = parse_team_inventory(inventory_values)
    TEAM_PLAYERS[player_id] = TeamPlayerStatus(level, COORDINATION_TURN,
        inventory)


def remove_old_team_players():

    global TEAM_COHORT

    old_players = []

    for player_id, status in TEAM_PLAYERS.items():
        waiting_turns = COORDINATION_TURN - status.updated_turn
        if waiting_turns > TEAM_STATUS_LIFETIME_TURNS:
            old_players.append(player_id)

    for player_id in old_players:
        del TEAM_PLAYERS[player_id]
    if TEAM_COHORT:
        active_players = set(TEAM_PLAYERS)
        active_players.add(PLAYER_ID)
        TEAM_COHORT = tuple(player_id for player_id in TEAM_COHORT
            if player_id in active_players)


def count_team_players_at_level(level):

    remove_old_team_players()
    players = [PLAYER_ID] if level == PLAYER_LEVEL else []

    for player_id, status in TEAM_PLAYERS.items():
        if status.level == level:
            players.append(player_id)
    return len(players)


def get_incantation_cohort_ids(level=None):

    remove_old_team_players()
    if level is None:
        level = PLAYER_LEVEL
    group_size = get_group_size(level)
    if group_size == 0:
        return []
    if group_size == 1:
        return [PLAYER_ID] if level == PLAYER_LEVEL else []
    if TEAM_COHORT:
        player_ids = []

        for player_id in TEAM_COHORT:
            if player_id == PLAYER_ID:
                if level == PLAYER_LEVEL:
                    player_ids.append(player_id)
                continue
            status = TEAM_PLAYERS.get(player_id)
            if status is not None and status.level == level:
                player_ids.append(player_id)
        return player_ids
    player_ids = [PLAYER_ID] if level == PLAYER_LEVEL else []

    for player_id, status in TEAM_PLAYERS.items():
        if status.level == level:
            player_ids.append(player_id)
    player_ids.sort()
    return player_ids


def get_player_resource_targets():

    requirements = INCANTATION_STONES.get(PLAYER_LEVEL, {})
    cohort = get_incantation_cohort_ids()

    if not requirements or PLAYER_ID not in cohort:
        return {}
    player_index = cohort.index(PLAYER_ID)
    targets = {}
    resource_index = 0

    for resource, count in requirements.items():
        for _ in range(count):
            if resource_index % len(cohort) == player_index:
                targets[resource] = targets.get(resource, 0) + 1
            resource_index += 1
    return targets


def team_has_required_stones_for_level(level, inventory, tile):

    requirements = INCANTATION_STONES.get(level)

    if requirements is None:
        return False
    quantities = {resource: inventory.get(resource, 0)
        + count_visible_item(tile, resource) for resource in requirements}
    cohort = get_incantation_cohort_ids(level)

    for player_id in cohort:
        if player_id == PLAYER_ID:
            continue
        status = TEAM_PLAYERS.get(player_id)
        if status is None:
            continue
        player_inventory = status.inventory
        for resource in requirements:
            quantities[resource] += player_inventory.get(resource, 0)
    return all(quantities[resource] >= count
        for resource, count in requirements.items())


def is_incantation_leader():

    cohort = get_incantation_cohort_ids()

    return len(cohort) > 0 and PLAYER_ID == cohort[0]


def share_team_status(server_connection, inventory=None):

    global TEAM_STATUS_TURNS

    if not TEAM_NAME:
        return False
    TEAM_STATUS_TURNS += 1
    if TEAM_STATUS_TURNS < TEAM_STATUS_INTERVAL:
        return False
    TEAM_STATUS_TURNS = 0
    message = "team_status|" + TEAM_NAME + "|" + str(PLAYER_LEVEL) + "|" + PLAYER_ID
    if inventory is not None:
        values = [str(inventory.get(resource, 0))
            for resource in TEAM_STATUS_RESOURCES]
        message += "|" + "|".join(values)
    send_command_to_server(server_connection, "Broadcast " + message)
    return True


def reset_incantation_calls():

    global LAST_FOLLOWED_CALL
    global WAITING_FOR_INCANTATION, ARRIVAL_CONFIRMATION_CALLER
    global INCANTATION_SESSION_ID, INCANTATION_SESSION_STARTED_TURN

    LAST_FOLLOWED_CALL = None
    WAITING_FOR_INCANTATION = False
    ARRIVAL_CONFIRMATION_CALLER = None
    INCANTATION_SESSION_ID = None
    INCANTATION_SESSION_STARTED_TURN = None
    INCANTATION_CALLS.clear()
    READY_PLAYERS.clear()


def remove_old_calls():

    old_calls = [caller_id for caller_id, call in INCANTATION_CALLS.items()
        if COORDINATION_TURN - call.created_turn > CALL_LIFETIME_TURNS]
    old_players = []

    for player_id, ready in READY_PLAYERS.items():
        waiting_turns = COORDINATION_TURN - ready.updated_turn
        if waiting_turns > READY_LIFETIME_TURNS:
            old_players.append(player_id)

    for caller_id in old_calls:
        del INCANTATION_CALLS[caller_id]
    for player_id in old_players:
        del READY_PLAYERS[player_id]


def get_call_to_follow():

    remove_old_calls()
    callers = [caller_id for caller_id, call in INCANTATION_CALLS.items()
        if call.level == PLAYER_LEVEL]

    if not callers:
        return None

    return INCANTATION_CALLS[min(callers)]


def all_cohort_players_are_ready(call):

    remove_old_calls()

    if not call.cohort or call.caller_id not in call.cohort:
        return False
    expected_players = set(call.cohort) - {call.caller_id}

    for player_id in expected_players:
        ready = READY_PLAYERS.get(player_id)
        if ready is None or ready.session_id != call.session_id:
            return False
    return True


def call_was_already_followed(caller_id):

    global LAST_FOLLOWED_CALL

    call = INCANTATION_CALLS.get(caller_id)

    if call is None:
        return False

    current_call = (caller_id, call.created_turn)

    if current_call == LAST_FOLLOWED_CALL:
        return True
    LAST_FOLLOWED_CALL = current_call
    return False


def get_resource_priority():

    needed_resources = list(INCANTATION_STONES.get(PLAYER_LEVEL, {}).keys())

    return needed_resources + [resource for resource in RESOURCE_PRIORITY if resource not in needed_resources]


def get_player_level():

    return PLAYER_LEVEL


def get_group_size(level):

    minimum_players = INCANTATION_PLAYERS.get(level, 0)

    if minimum_players <= 1:
        return minimum_players
    if not TEAM_COHORT:
        return TEAM_SIZE
    return WINNING_COHORT_SIZE


def remember_successful_cohort(new_level):

    global TEAM_COHORT

    if new_level != PLAYER_LEVEL + 1:
        return
    call = get_call_to_follow()
    if call is None or PLAYER_ID not in call.cohort:
        return
    if len(call.cohort) < WINNING_COHORT_SIZE:
        return
    TEAM_COHORT = call.cohort
    for player_id in TEAM_COHORT:
        if player_id == PLAYER_ID:
            continue
        status = TEAM_PLAYERS.get(player_id)
        inventory = status.inventory if status is not None else {}
        TEAM_PLAYERS[player_id] = TeamPlayerStatus(new_level,
            COORDINATION_TURN, inventory)


def update_player_level(response):

    global PLAYER_LEVEL

    if not response.startswith("Current level:"):
        return False
    try:
        level = int(response.split(":", 1)[1].strip())
    except (IndexError, ValueError):
        return False
    if level < 1 or level > 8:
        return False
    remember_successful_cohort(level)
    PLAYER_LEVEL = level
    reset_incantation_calls()
    return True


def place_missing_stones_for_level(server_connection, inventory, tile, level):

    all_stones_are_placed = True

    for resource, count in INCANTATION_STONES.get(level, {}).items():
        missing = count - count_visible_item(tile, resource)
        available = min(max(missing, 0), inventory.get(resource, 0))

        while available > 0:
            response = send_command_to_server(server_connection,
                "Set " + resource)
            if response != "ok":
                return False
            inventory[resource] -= 1
            missing -= 1
            available -= 1
        if missing > 0:
            all_stones_are_placed = False
    return all_stones_are_placed


def try_current_incantation(server_connection, inventory, tile):

    if PLAYER_LEVEL not in INCANTATION_PLAYERS:
        return False
    if inventory.get("food", 0) < 2:
        return False
    minimum_players = INCANTATION_PLAYERS[PLAYER_LEVEL]
    required_players = get_group_size(PLAYER_LEVEL)
    cohort = (PLAYER_ID,)
    if minimum_players > 1:
        call = get_call_to_follow()
        if call is None or call.caller_id != PLAYER_ID:
            return False
        cohort = call.cohort
        if len(cohort) < required_players:
            return False
        if not all_cohort_players_are_ready(call):
            return False
    if count_visible_item(tile, "player") < len(cohort):
        return False
    if not has_required_stones_for_level(PLAYER_LEVEL, inventory, tile):
        return False
    if not place_missing_stones_for_level(server_connection, inventory, tile, PLAYER_LEVEL):
        return False
    response = send_command_to_server(server_connection, "Incantation")
    if update_player_level(response):
        return True
    if response != "Elevation underway":
        READY_PLAYERS.clear()
        return False
    level_up = update_player_level(receive_response_from_server(server_connection, "Incantation"))
    if not level_up:
        READY_PLAYERS.clear()
    return level_up


def has_incantation_help_requirements(inventory, tile, required_players):

    food_reserve = (4 if WAITING_FOR_INCANTATION
        else INCANTATION_FOOD_RESERVE)
    if inventory.get("food", 0) < food_reserve:
        return False
    if INCANTATION_CALLS.get(PLAYER_ID) is not None:
        return True
    if count_team_players_at_level(PLAYER_LEVEL) < required_players:
        return False
    return team_has_required_stones_for_level(PLAYER_LEVEL, inventory, tile)


def broadcast_incantation_help(server_connection):

    global WAITING_FOR_INCANTATION, INCANTATION_SESSION_ID
    global INCANTATION_SESSION_STARTED_TURN

    WAITING_FOR_INCANTATION = True
    call = INCANTATION_CALLS.get(PLAYER_ID)
    session_expired = (call is not None
        and INCANTATION_SESSION_STARTED_TURN is not None
        and COORDINATION_TURN - INCANTATION_SESSION_STARTED_TURN
            > CALL_SESSION_MAX_TURNS)
    if session_expired:
        INCANTATION_CALLS.pop(PLAYER_ID, None)
        READY_PLAYERS.clear()
        INCANTATION_SESSION_ID = None
        INCANTATION_SESSION_STARTED_TURN = None
        call = None
    if call is None or not call.session_id or not call.cohort:
        cohort = tuple(get_incantation_cohort_ids())
        if len(cohort) < get_group_size(PLAYER_LEVEL):
            WAITING_FOR_INCANTATION = False
            return False
        INCANTATION_SESSION_ID = token_hex(6)
        INCANTATION_SESSION_STARTED_TURN = COORDINATION_TURN
        READY_PLAYERS.clear()
    else:
        INCANTATION_SESSION_ID = call.session_id
        cohort = call.cohort
    INCANTATION_CALLS[PLAYER_ID] = IncantationCall(PLAYER_ID, 0,
        PLAYER_LEVEL, COORDINATION_TURN, INCANTATION_SESSION_ID, cohort)
    message = ("incantation_request|" + TEAM_NAME + "|" + str(PLAYER_LEVEL) + "|" + PLAYER_ID + "|" + INCANTATION_SESSION_ID + "|" + ",".join(cohort))
    send_command_to_server(server_connection, "Broadcast " + message)
    return True


def say_ready_for_incantation(server_connection, caller_id, session_id):

    message = ("incantation_ready|" + TEAM_NAME + "|" + str(PLAYER_LEVEL) + "|" + caller_id + "|" + PLAYER_ID + "|" + session_id)
    send_command_to_server(server_connection, "Broadcast " + message)


def request_incantation_help(server_connection, inventory, tile):

    minimum_players = INCANTATION_PLAYERS.get(PLAYER_LEVEL)
    if minimum_players is None or minimum_players <= 1:
        return False
    if not is_incantation_leader():
        return False
    call = get_call_to_follow()
    if call is not None and call.caller_id != PLAYER_ID:
        return False
    required_players = get_group_size(PLAYER_LEVEL)
    if len(get_incantation_cohort_ids()) < required_players:
        return False
    if not has_incantation_help_requirements(inventory, tile,
            required_players):
        return False
    return broadcast_incantation_help(server_connection)


def follow_incantation_call(server_connection, inventory):

    global WAITING_FOR_INCANTATION, ARRIVAL_CONFIRMATION_CALLER

    call = get_call_to_follow()
    if call is None:
        WAITING_FOR_INCANTATION = False
        ARRIVAL_CONFIRMATION_CALLER = None
        return False
    if call.caller_id == PLAYER_ID:
        return False

    food_reserve = (4 if WAITING_FOR_INCANTATION
        else INCANTATION_FOOD_RESERVE)
    if inventory.get("food", 0) < food_reserve:
        return False
    if call.level != PLAYER_LEVEL:
        return False
    if PLAYER_ID not in call.cohort:
        return False
    if call_was_already_followed(call.caller_id):
        send_command_to_server(server_connection, "Look")
        return True
    if call.direction == 0:
        WAITING_FOR_INCANTATION = True
        confirmation_key = (call.caller_id, call.session_id)
        if ARRIVAL_CONFIRMATION_CALLER != confirmation_key:
            ARRIVAL_CONFIRMATION_CALLER = confirmation_key
            send_command_to_server(server_connection, "Look")
        else:
            look = send_command_to_server(server_connection, "Look")
            tile = get_current_tile(look)
            place_missing_stones_for_level(server_connection, inventory, tile,
                PLAYER_LEVEL)
            say_ready_for_incantation(server_connection, call.caller_id,
                call.session_id)
        return True
    WAITING_FOR_INCANTATION = False
    ARRIVAL_CONFIRMATION_CALLER = None
    if call.direction in (1, 2, 8):
        send_command_to_server(server_connection, "Forward")
        return True
    if call.direction in (3, 4):
        send_command_to_server(server_connection, "Left")
        return True
    send_command_to_server(server_connection, "Right")
    return True
