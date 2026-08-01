from secrets import token_hex

from client import receive_response_from_server, send_command_to_server
from coordination import (call_was_already_followed,
    get_call_to_follow,
    reset_incantation_call,
    update_player_level,)
from coordination_state import IncantationCall, STATE
from strategy import (FOOD_SURVIVAL_LIMIT,
    INCANTATION_PLAYERS,
    INCANTATION_STONES,
    TARGET_LEVEL,
    count_visible_item,
    get_current_tile,
    has_required_stones_for_level,)


def place_missing_stones(server_connection, inventory, tile):

    requirements = INCANTATION_STONES.get(STATE.player_level, {})

    for resource, required in requirements.items():

        missing = max(required - count_visible_item(tile, resource), 0)

        if inventory.get(resource, 0) < missing:
            return False
        for _ in range(missing):
            if send_command_to_server(server_connection, "Set " + resource) != "ok":
                return False
            inventory[resource] -= 1

    return True


def try_current_incantation(server_connection, inventory, tile):

    required_players = INCANTATION_PLAYERS.get(STATE.player_level)

    if required_players is None or inventory.get("food", 0) < 2:
        return False
    if required_players == 2:

        call = get_call_to_follow()

        if (call is None or call.caller_id != STATE.player_id or STATE.ready_player_id is None):
            return False
    if count_visible_item(tile, "player") < required_players:
        return False
    if not has_required_stones_for_level(STATE.player_level, inventory, tile):
        return False
    if not place_missing_stones(server_connection, inventory, tile):
        return False

    response = send_command_to_server(server_connection, "Incantation")

    if update_player_level(response):
        return True
    if response != "Elevation underway":
        reset_incantation_call()
        return False

    response = receive_response_from_server(server_connection, "Incantation")

    if update_player_level(response):
        return True

    reset_incantation_call()
    return False


def can_request_help(inventory, tile):

    return (inventory.get("food", 0) >= FOOD_SURVIVAL_LIMIT and has_required_stones_for_level(STATE.player_level, inventory, tile))


def request_incantation_help(server_connection, inventory, tile):

    if INCANTATION_PLAYERS.get(STATE.player_level, 0) != 2:
        return False

    call = get_call_to_follow()

    if call is not None and call.caller_id != STATE.player_id:
        return False
    if call is None:
        if not can_request_help(inventory, tile):
            return False
        call = IncantationCall(STATE.player_id, 0, STATE.player_level, STATE.coordination_turn, token_hex(6))
        STATE.incantation_call = call
        STATE.ready_player_id = None

    STATE.waiting_for_incantation = True
    message = "|".join(("incantation_request", STATE.team_name,
        str(STATE.player_level), STATE.player_id, call.session_id))
    send_command_to_server(server_connection, "Broadcast " + message)
    return True


def say_ready(server_connection, call):

    message = "|".join(("incantation_ready", STATE.team_name,
        str(STATE.player_level), call.caller_id, STATE.player_id, call.session_id))
    send_command_to_server(server_connection, "Broadcast " + message)


def follow_incantation_call(server_connection, inventory):

    if STATE.player_level >= TARGET_LEVEL:
        reset_incantation_call()
        return False

    call = get_call_to_follow()

    if call is None or call.caller_id == STATE.player_id:
        return False
    if inventory.get("food", 0) < FOOD_SURVIVAL_LIMIT:
        return False
    if call_was_already_followed(call):
        return True
    if call.direction == 0:
        STATE.waiting_for_incantation = True
        look = send_command_to_server(server_connection, "Look")
        tile = get_current_tile(look)
        place_missing_stones(server_connection, inventory, tile)
        say_ready(server_connection, call)
        return True

    STATE.waiting_for_incantation = False

    if call.direction in (1, 2, 8):
        command = "Forward"
    elif call.direction in (3, 4):
        command = "Left"
    else:
        command = "Right"

    send_command_to_server(server_connection, command)
    return True
