from secrets import token_hex

from strategy import (INCANTATION_STONES,
    RESOURCE_PRIORITY,
    can_try_level_two,
    count_visible_item,
    has_required_stones_for_level,)

PLAYER_LEVEL = 1
INCANTATION_PLAYERS = {1: 1, 2: 2, 3: 2, 4: 4, 5: 4, 6: 6, 7: 6}
TEAM_NAME = ""
TEAM_PLAYERS = {}
LAST_INCANTATION_CALL = None
LAST_FOLLOWED_CALL = None
PLAYER_ID = token_hex(8)
INCANTATION_CALLS = {}
READY_PLAYERS = {}
CALL_LIFETIME_TURNS = 24
READY_LIFETIME_TURNS = 24
TEAM_STATUS_LIFETIME_TURNS = 32
TEAM_STATUS_INTERVAL = 8
INCANTATION_LEADER_FOOD_RESERVE = 9
INCANTATION_FOLLOWER_FOOD_RESERVE = 9
TEAM_STATUS_TURNS = 0
HELP_MESSAGE_SENT = False
READY_MESSAGE_SENT = False
COORDINATION_TURN = 0


def advance_coordination_turn():

    global COORDINATION_TURN

    COORDINATION_TURN += 1


def send_command_to_server(server_connection, command):

    from client import send_command_to_server as send_command

    return send_command(server_connection, command)


def receive_response_from_server(server_connection, expected_command=None):

    from client import receive_response_from_server as receive_response

    return receive_response(server_connection, expected_command)


def remember_incantation_call(message):

    global LAST_INCANTATION_CALL

    try:
        header, payload = message.split(",", 1)
    except ValueError:
        return

    payload = payload.strip()
    header_parts = header.split()
    parts = payload.split()

    if len(header_parts) != 2 or header_parts[0] != "message":
        return
    if not header_parts[1].isdigit():
        return

    direction = int(header_parts[1])

    if direction < 0 or direction > 8:
        return

    message_parts = payload.split("|")

    if len(message_parts) == 4 and message_parts[0] == "team_status":
        save_team_status(message_parts[1], message_parts[2], message_parts[3])
        return
    if len(message_parts) == 4 and message_parts[0] == "incantation_request":
        if message_parts[1] == TEAM_NAME:
            save_incantation_call(direction, message_parts[2], message_parts[3])
        return
    if len(message_parts) == 5 and message_parts[0] == "incantation_ready":
        if message_parts[1] == TEAM_NAME:
            save_ready_player(direction, message_parts[2], message_parts[3], message_parts[4])
        return
    if TEAM_NAME:
        return

    compact_parts = payload.split("_")

    if len(compact_parts) == 4 and compact_parts[:2] == ["incantation", "request"]:
        save_incantation_call(direction, compact_parts[2], compact_parts[3])
        return
    if len(compact_parts) == 5 and compact_parts[:2] == ["incantation", "ready"]:
        save_ready_player(direction, compact_parts[2], compact_parts[3], compact_parts[4])
        return

    if len(parts) == 2 and parts[0] == "incantation":
        if not parts[1].isdigit() or int(parts[1]) != PLAYER_LEVEL:
            return
        LAST_INCANTATION_CALL = (direction, PLAYER_LEVEL)
        return

    if len(parts) == 4 and parts[:2] == ["incantation", "request"]:
        save_incantation_call(direction, parts[2], parts[3])
        return
    if len(parts) == 5 and parts[:2] == ["incantation", "ready"]:
        save_ready_player(direction, parts[2], parts[3], parts[4])


def save_incantation_call(direction, level_text, caller_id):

    if not level_text.isdigit():
        return

    level = int(level_text)

    if level != PLAYER_LEVEL:
        return
    INCANTATION_CALLS[caller_id] = (direction, level, COORDINATION_TURN)


def save_ready_player(direction, level_text, caller_id, player_id):

    if not level_text.isdigit():
        return

    level = int(level_text)

    if level != PLAYER_LEVEL or caller_id != PLAYER_ID or player_id == PLAYER_ID:
        return
    if direction != 0:
        return
    READY_PLAYERS[player_id] = COORDINATION_TURN


def set_team_name(team_name):

    global TEAM_NAME

    TEAM_NAME = team_name


def save_team_status(team_name, level_text, player_id):

    if team_name != TEAM_NAME or not level_text.isdigit():
        return
    level = int(level_text)
    if level < 1 or level > 8 or player_id == PLAYER_ID:
        return
    TEAM_PLAYERS[player_id] = (level, COORDINATION_TURN)


def remove_old_team_players():

    old_players = []

    for player_id, status in TEAM_PLAYERS.items():
        last_status_turn = status[1]
        waiting_turns = COORDINATION_TURN - last_status_turn
        if waiting_turns > TEAM_STATUS_LIFETIME_TURNS:
            old_players.append(player_id)

    for player_id in old_players:
        del TEAM_PLAYERS[player_id]


def count_team_players_at_level(level):

    remove_old_team_players()
    players = 1

    for player_level, _ in TEAM_PLAYERS.values():
        if player_level == level:
            players += 1
    return players


def is_incantation_leader():

    remove_old_team_players()
    same_level_ids = [PLAYER_ID]

    for player_id, status in TEAM_PLAYERS.items():
        if status[0] == PLAYER_LEVEL:
            same_level_ids.append(player_id)
    return PLAYER_ID == min(same_level_ids)


def share_team_status(server_connection):

    global TEAM_STATUS_TURNS

    if not TEAM_NAME:
        return False
    TEAM_STATUS_TURNS += 1
    if TEAM_STATUS_TURNS < TEAM_STATUS_INTERVAL:
        return False
    TEAM_STATUS_TURNS = 0
    message = "team_status|" + TEAM_NAME + "|" + str(PLAYER_LEVEL) + "|" + PLAYER_ID
    send_command_to_server(server_connection, "Broadcast " + message)
    return True


def reset_incantation_calls():

    global LAST_INCANTATION_CALL, LAST_FOLLOWED_CALL
    global HELP_MESSAGE_SENT, READY_MESSAGE_SENT

    LAST_INCANTATION_CALL = None
    LAST_FOLLOWED_CALL = None
    HELP_MESSAGE_SENT = False
    READY_MESSAGE_SENT = False
    INCANTATION_CALLS.clear()
    READY_PLAYERS.clear()


def remove_old_calls():

    old_calls = [caller_id for caller_id, call in INCANTATION_CALLS.items()
        if COORDINATION_TURN - call[2] > CALL_LIFETIME_TURNS]
    old_players = []

    for player_id, last_ready in READY_PLAYERS.items():
        waiting_turns = COORDINATION_TURN - last_ready
        if waiting_turns > READY_LIFETIME_TURNS:
            old_players.append(player_id)

    for caller_id in old_calls:
        del INCANTATION_CALLS[caller_id]
    for player_id in old_players:
        del READY_PLAYERS[player_id]


def get_call_to_follow():

    remove_old_calls()
    callers = [caller_id for caller_id, call in INCANTATION_CALLS.items() if call[1] == PLAYER_LEVEL]

    if not callers:
        return None

    caller_id = min(callers)
    direction, level, _ = INCANTATION_CALLS[caller_id]
    return caller_id, direction, level


def count_ready_players():

    remove_old_calls()
    return len(READY_PLAYERS)


def call_was_already_followed(caller_id):

    global LAST_FOLLOWED_CALL

    call = INCANTATION_CALLS.get(caller_id)

    if call is None:
        return False

    current_call = (caller_id, call[2])

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

    return INCANTATION_PLAYERS.get(level, 0)


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
    PLAYER_LEVEL = level
    reset_incantation_calls()
    return True


def place_missing_stones_for_level(server_connection, inventory, tile, level):

    for resource, count in INCANTATION_STONES.get(level, {}).items():
        missing = count - count_visible_item(tile, resource)
        while missing > 0:
            if inventory.get(resource, 0) <= 0:
                return False
            reponse = send_command_to_server(server_connection, "Set " + resource)
            if reponse != "ok":
                return False
            inventory[resource] -= 1
            missing -= 1
    return True


def try_first_incantation(server_connection, inventory, tile):

    if not place_missing_stones_for_level(server_connection, inventory, tile, 1):
        return False
    reponse = send_command_to_server(server_connection, "Incantation")

    if update_player_level(reponse):
        return True
    if reponse == "Elevation underway":
        reponse = receive_response_from_server(server_connection, "Incantation")
        if update_player_level(reponse):
            return True
    return False


def try_level_two_incantation(server_connection, inventory, tile):

    if not can_try_level_two(inventory, tile):
        return False
    if count_visible_item(tile, "player") > 1:
        send_command_to_server(server_connection, "Right")
        return True
    return try_first_incantation(server_connection, inventory, tile)


def try_current_incantation(server_connection, inventory, tile):

    if PLAYER_LEVEL not in INCANTATION_PLAYERS:
        return False
    if inventory.get("food", 0) < 2:
        return False
    minimum_players = INCANTATION_PLAYERS[PLAYER_LEVEL]
    group_size = get_group_size(PLAYER_LEVEL)
    if minimum_players > 1:
        call = get_call_to_follow()
        if call is None or call[0] != PLAYER_ID:
            return False
        if count_ready_players() < group_size - 1:
            return False
    if count_visible_item(tile, "player") < group_size:
        return False
    if not has_required_stones_for_level(PLAYER_LEVEL, inventory, tile):
        return False
    if not place_missing_stones_for_level(server_connection, inventory, tile, PLAYER_LEVEL):
        return False
    reponse = send_command_to_server(server_connection, "Incantation")
    if update_player_level(reponse):
        return True
    if reponse != "Elevation underway":
        READY_PLAYERS.clear()
        return False
    level_up = update_player_level(receive_response_from_server(server_connection, "Incantation"))
    if not level_up:
        READY_PLAYERS.clear()
    return level_up


def has_incantation_help_requirements(inventory, tile, required_players):

    if inventory.get("food", 0) < INCANTATION_LEADER_FOOD_RESERVE:
        return False
    enough_players = (count_ready_players() >= required_players - 1
        and count_visible_item(tile, "player") >= required_players)
    if enough_players:
        return False
    return has_required_stones_for_level(PLAYER_LEVEL, inventory, tile)


def broadcast_incantation_help(server_connection):

    global HELP_MESSAGE_SENT

    if HELP_MESSAGE_SENT:
        HELP_MESSAGE_SENT = False
        send_command_to_server(server_connection, "Look")
        return
    HELP_MESSAGE_SENT = True
    INCANTATION_CALLS[PLAYER_ID] = (0, PLAYER_LEVEL, COORDINATION_TURN)
    message = "incantation_request|" + TEAM_NAME + "|" + str(PLAYER_LEVEL) + "|" + PLAYER_ID
    send_command_to_server(server_connection, "Broadcast " + message)


def say_ready_for_incantation(server_connection, caller_id):

    global READY_MESSAGE_SENT

    if READY_MESSAGE_SENT:
        READY_MESSAGE_SENT = False
        send_command_to_server(server_connection, "Look")
        return
    READY_MESSAGE_SENT = True
    message = "incantation_ready|" + TEAM_NAME + "|" + str(PLAYER_LEVEL) + "|" + caller_id + "|" + PLAYER_ID
    send_command_to_server(server_connection, "Broadcast " + message)


def request_incantation_help(server_connection, inventory, tile):

    minimum_players = INCANTATION_PLAYERS.get(PLAYER_LEVEL)
    if minimum_players is None or minimum_players <= 1:
        return False
    if not is_incantation_leader():
        return False
    call = get_call_to_follow()
    if call is not None and call[0] != PLAYER_ID:
        return False
    if not has_incantation_help_requirements(inventory, tile, get_group_size(PLAYER_LEVEL)):
        return False
    broadcast_incantation_help(server_connection)
    return True


def follow_incantation_call(server_connection, inventory):

    global LAST_INCANTATION_CALL

    call = get_call_to_follow()
    caller_id = None
    if call is not None:
        caller_id, direction, level = call
        if caller_id == PLAYER_ID:
            return False
    else:
        call = LAST_INCANTATION_CALL
        LAST_INCANTATION_CALL = None
        if call is None:
            return False
        direction, level = call

    if inventory.get("food", 0) < INCANTATION_FOLLOWER_FOOD_RESERVE:
        return False
    if level != PLAYER_LEVEL:
        return False
    if caller_id is not None and call_was_already_followed(caller_id):
        send_command_to_server(server_connection, "Look")
        return True
    if direction == 0:
        if caller_id is None:
            send_command_to_server(server_connection, "Look")
        else:
            say_ready_for_incantation(server_connection, caller_id)
        return True
    if direction in (1, 2, 8):
        send_command_to_server(server_connection, "Forward")
        return True
    if direction in (3, 4, 5):
        send_command_to_server(server_connection, "Left")
        return True

    send_command_to_server(server_connection, "Right")
    return True
