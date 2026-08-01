from coordination_state import IncantationCall, STATE

CALL_LIFETIME_TURNS = 64


def set_team_name(team_name):

    STATE.team_name = team_name


def advance_coordination_turn():

    STATE.coordination_turn += 1


def is_waiting_for_incantation():

    return STATE.waiting_for_incantation


def cancel_incantation_wait():

    STATE.waiting_for_incantation = False
    reset_incantation_call()


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

    if len(message_parts) == 3 and message_parts[0] == "team_presence":
        if message_parts[1] == STATE.team_name:
            save_team_presence(message_parts[2])
        return
    if len(message_parts) == 5 and message_parts[0] == "incantation_request":
        if message_parts[1] == STATE.team_name:
            save_incantation_call(direction, *message_parts[2:])
        return
    if len(message_parts) == 6 and message_parts[0] == "incantation_ready":
        if message_parts[1] == STATE.team_name:
            save_ready_player(direction, *message_parts[2:])


def save_team_presence(player_id):

    if player_id and player_id != STATE.player_id:
        STATE.team_peers[player_id] = STATE.coordination_turn


def save_incantation_call(direction, level_text, caller_id, session_id):

    if not level_text.isdigit():
        return

    level = int(level_text)

    if level != STATE.player_level or not caller_id or not session_id:
        return

    current = STATE.incantation_call

    if current is not None and current.session_id != session_id:
        if current.caller_id < caller_id:
            return
        STATE.ready_player_id = None
    STATE.incantation_call = IncantationCall(caller_id, direction, level, STATE.coordination_turn, session_id)


def save_ready_player(direction, level_text, caller_id, player_id, session_id):

    call = STATE.incantation_call

    if direction != 0 or call is None or not level_text.isdigit():
        return
    if int(level_text) != STATE.player_level or caller_id != STATE.player_id:
        return
    if player_id != STATE.player_id and call.session_id == session_id:
        STATE.ready_player_id = player_id


def reset_incantation_call():

    STATE.last_followed_call = None
    STATE.waiting_for_incantation = False
    STATE.incantation_call = None
    STATE.ready_player_id = None


def get_call_to_follow():

    call = STATE.incantation_call

    if call is None:
        return None
    if STATE.coordination_turn - call.created_turn > CALL_LIFETIME_TURNS:
        reset_incantation_call()
        return None

    return call


def call_was_already_followed(call):

    current_call = (call.session_id, call.created_turn)

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
    reset_incantation_call()
    return True
