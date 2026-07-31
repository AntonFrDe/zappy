from coordination_state import IncantationCall, ReadyPlayer, STATE
from team import (TEAM_SIZE,
    remember_team_player_activity,
    save_team_status,)

CALL_LIFETIME_TURNS = 64
CALL_SESSION_MAX_TURNS = 128
READY_LIFETIME_TURNS = 16
MAX_COHORT_SIZE = TEAM_SIZE


def advance_coordination_turn():

    STATE.coordination_turn += 1


def is_waiting_for_incantation():

    return STATE.waiting_for_incantation


def cancel_incantation_wait():

    STATE.waiting_for_incantation = False
    STATE.arrival_confirmation_caller = None


def handle_broadcast_message(message):

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
        save_team_status(message_parts[1], message_parts[2], message_parts[3])
        return
    if len(message_parts) == 6 and message_parts[0] == "incantation_request":
        if message_parts[1] == STATE.team_name:
            remember_team_player_activity(message_parts[2], message_parts[3])
            save_incantation_call(direction, message_parts[2], message_parts[3], message_parts[4], message_parts[5],)
        return
    if len(message_parts) == 6 and message_parts[0] == "incantation_ready":
        if message_parts[1] == STATE.team_name:
            remember_team_player_activity(message_parts[2], message_parts[4])
            save_ready_player(direction, message_parts[2], message_parts[3], message_parts[4], message_parts[5],)


def parse_cohort_ids(cohort_text):

    if not cohort_text:
        return ()
    cohort = tuple(dict.fromkeys(cohort_text.split(",")))
    if len(cohort) > MAX_COHORT_SIZE:
        return ()
    if any(not player_id for player_id in cohort):
        return ()
    return cohort


def save_incantation_call(direction, level_text, caller_id, session_id="", cohort_text=""):

    if not level_text.isdigit():
        return

    level = int(level_text)
    cohort = parse_cohort_ids(cohort_text)

    if level != STATE.player_level:
        return
    if cohort and (caller_id not in cohort or STATE.player_id not in cohort):
        return
    STATE.incantation_calls[caller_id] = IncantationCall(caller_id, direction, level,
        STATE.coordination_turn, session_id, cohort,)


def save_ready_player(direction, level_text, caller_id, player_id, session_id=""):

    if not level_text.isdigit():
        return

    level = int(level_text)
    call = STATE.incantation_calls.get(STATE.player_id)

    if (level != STATE.player_level or caller_id != STATE.player_id or player_id == STATE.player_id):
        return
    if direction != 0 or call is None:
        return
    if call.cohort and player_id not in call.cohort:
        return
    if call.session_id != session_id:
        return
    STATE.ready_players[player_id] = ReadyPlayer(STATE.coordination_turn, session_id)


def reset_incantation_calls():

    STATE.last_followed_call = None
    STATE.waiting_for_incantation = False
    STATE.arrival_confirmation_caller = None
    STATE.incantation_session_id = None
    STATE.incantation_session_started_turn = None
    STATE.incantation_calls.clear()
    STATE.ready_players.clear()


def remove_old_calls():

    old_calls = [caller_id for caller_id, call in STATE.incantation_calls.items()
        if STATE.coordination_turn - call.created_turn > CALL_LIFETIME_TURNS]
    old_players = [player_id for player_id, ready in STATE.ready_players.items()
        if STATE.coordination_turn - ready.updated_turn > READY_LIFETIME_TURNS]

    for caller_id in old_calls:
        del STATE.incantation_calls[caller_id]
    for player_id in old_players:
        del STATE.ready_players[player_id]


def get_call_to_follow():

    remove_old_calls()

    callers = [caller_id for caller_id, call in STATE.incantation_calls.items()
        if call.level == STATE.player_level]

    if not callers:
        return None
    return STATE.incantation_calls[min(callers)]


def all_cohort_players_are_ready(call):

    remove_old_calls()

    if not call.cohort or call.caller_id not in call.cohort:
        return False
    expected_players = set(call.cohort) - {call.caller_id}

    for player_id in expected_players:
        ready = STATE.ready_players.get(player_id)
        if ready is None or ready.session_id != call.session_id:
            return False
    return True


def call_was_already_followed(caller_id):

    call = STATE.incantation_calls.get(caller_id)

    if call is None:
        return False
    current_call = (caller_id, call.created_turn)
    if current_call == STATE.last_followed_call:
        return True
    STATE.last_followed_call = current_call
    return False


def update_player_level(response):

    if not response.startswith("Current level:"):
        return False
    try:
        level = int(response.split(":", 1)[1].strip())
    except (IndexError, ValueError):
        return False
    if level < 1 or level > 8:
        return False
    STATE.player_level = level
    reset_incantation_calls()
    return True
