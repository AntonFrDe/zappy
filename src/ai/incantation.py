from secrets import token_hex

from client import receive_response_from_server, send_command_to_server
from coordination import (CALL_SESSION_MAX_TURNS,
    all_cohort_players_are_ready,
    call_was_already_followed,
    get_call_to_follow,
    update_player_level,)
from coordination_state import IncantationCall, STATE
from strategy import (FOOD_SURVIVAL_LIMIT,
    INCANTATION_PLAYERS,
    INCANTATION_STONES,
    TARGET_LEVEL,
    count_visible_item,
    get_current_tile,
    has_required_stones_for_level,)
from team import (count_team_players_at_level,
    get_group_size,
    get_incantation_cohort_ids,
    is_incantation_leader,)

INCANTATION_FOOD_RESERVE = FOOD_SURVIVAL_LIMIT


def place_missing_stones_for_level(server_connection, inventory, tile, level):

    all_stones_are_placed = True

    for resource, count in INCANTATION_STONES.get(level, {}).items():
        missing = count - count_visible_item(tile, resource)
        available = min(max(missing, 0), inventory.get(resource, 0))

        while available > 0:
            response = send_command_to_server(server_connection, "Set " + resource)
            if response != "ok":
                return False
            inventory[resource] -= 1
            missing -= 1
            available -= 1
        if missing > 0:
            all_stones_are_placed = False
    return all_stones_are_placed


def try_current_incantation(server_connection, inventory, tile):

    if STATE.player_level not in INCANTATION_PLAYERS:
        return False
    if inventory.get("food", 0) < 2:
        return False

    minimum_players = INCANTATION_PLAYERS[STATE.player_level]
    required_players = get_group_size(STATE.player_level)
    cohort = (STATE.player_id,)

    if minimum_players > 1:
        call = get_call_to_follow()
        if call is None or call.caller_id != STATE.player_id:
            return False
        cohort = call.cohort
        if len(cohort) < required_players:
            return False
        if not all_cohort_players_are_ready(call):
            return False

    if count_visible_item(tile, "player") < len(cohort):
        return False
    if not has_required_stones_for_level(
            STATE.player_level, inventory, tile):
        return False
    if not place_missing_stones_for_level(
            server_connection, inventory, tile, STATE.player_level):
        return False

    response = send_command_to_server(server_connection, "Incantation")

    if update_player_level(response):
        return True
    if response != "Elevation underway":
        STATE.ready_players.clear()
        return False
    level_response = receive_response_from_server(server_connection, "Incantation")
    level_up = update_player_level(level_response)

    if not level_up:
        STATE.ready_players.clear()
    return level_up


def has_incantation_help_requirements(inventory, tile, required_players):

    food_reserve = (4 if STATE.waiting_for_incantation else INCANTATION_FOOD_RESERVE)

    if inventory.get("food", 0) < food_reserve:
        return False
    if STATE.incantation_calls.get(STATE.player_id) is not None:
        return True
    if count_team_players_at_level(STATE.player_level) < required_players:
        return False

    return has_required_stones_for_level(STATE.player_level, inventory, tile)


def broadcast_incantation_help(server_connection):

    STATE.waiting_for_incantation = True
    call = STATE.incantation_calls.get(STATE.player_id)
    session_expired = (call is not None and STATE.incantation_session_started_turn is not None
        and STATE.coordination_turn - STATE.incantation_session_started_turn > CALL_SESSION_MAX_TURNS)

    if session_expired:
        STATE.incantation_calls.pop(STATE.player_id, None)
        STATE.ready_players.clear()
        STATE.incantation_session_id = None
        STATE.incantation_session_started_turn = None
        call = None

    if call is None or not call.session_id or not call.cohort:
        cohort = tuple(get_incantation_cohort_ids())
        if len(cohort) < get_group_size(STATE.player_level):
            STATE.waiting_for_incantation = False
            return False
        STATE.incantation_session_id = token_hex(6)
        STATE.incantation_session_started_turn = STATE.coordination_turn
        STATE.ready_players.clear()
    else:
        STATE.incantation_session_id = call.session_id
        cohort = call.cohort
    STATE.incantation_calls[STATE.player_id] = IncantationCall(STATE.player_id, 0, STATE.player_level, STATE.coordination_turn,
    STATE.incantation_session_id, cohort,)
    message = ("incantation_request|" + STATE.team_name + "|" + str(STATE.player_level) + "|" + STATE.player_id + "|"
    + STATE.incantation_session_id + "|" + ",".join(cohort))

    send_command_to_server(server_connection, "Broadcast " + message)
    return True


def say_ready_for_incantation(server_connection, caller_id, session_id):

    message = ("incantation_ready|" + STATE.team_name + "|" + str(STATE.player_level) + "|" + caller_id + "|" + STATE.player_id + "|" + session_id)

    send_command_to_server(server_connection, "Broadcast " + message)


def request_incantation_help(server_connection, inventory, tile):

    minimum_players = INCANTATION_PLAYERS.get(STATE.player_level)

    if minimum_players is None or minimum_players <= 1:
        return False
    if not is_incantation_leader():
        return False

    call = get_call_to_follow()

    if call is not None and call.caller_id != STATE.player_id:
        return False

    required_players = get_group_size(STATE.player_level)

    if len(get_incantation_cohort_ids()) < required_players:
        return False
    if not has_incantation_help_requirements(inventory, tile, required_players):
        return False
    return broadcast_incantation_help(server_connection)


def follow_incantation_call(server_connection, inventory):

    if STATE.player_level >= TARGET_LEVEL:
        STATE.waiting_for_incantation = False
        STATE.arrival_confirmation_caller = None
        return False

    call = get_call_to_follow()

    if call is None:
        STATE.waiting_for_incantation = False
        STATE.arrival_confirmation_caller = None
        return False
    if call.caller_id == STATE.player_id:
        return False

    food_reserve = (4 if STATE.waiting_for_incantation else INCANTATION_FOOD_RESERVE)

    if inventory.get("food", 0) < food_reserve:
        return False
    if call.level != STATE.player_level:
        return False
    if STATE.player_id not in call.cohort:
        return False
    if call_was_already_followed(call.caller_id):
        send_command_to_server(server_connection, "Look")
        return True

    if call.direction == 0:
        STATE.waiting_for_incantation = True
        confirmation_key = (call.caller_id, call.session_id)
        if STATE.arrival_confirmation_caller != confirmation_key:
            STATE.arrival_confirmation_caller = confirmation_key
            send_command_to_server(server_connection, "Look")
        else:
            look = send_command_to_server(server_connection, "Look")
            tile = get_current_tile(look)
            place_missing_stones_for_level(server_connection, inventory, tile, STATE.player_level)
            say_ready_for_incantation(server_connection, call.caller_id, call.session_id)
        return True

    STATE.waiting_for_incantation = False
    STATE.arrival_confirmation_caller = None

    if call.direction in (1, 2, 8):
        send_command_to_server(server_connection, "Forward")
        return True
    if call.direction in (3, 4):
        send_command_to_server(server_connection, "Left")
        return True
    send_command_to_server(server_connection, "Right")
    return True
