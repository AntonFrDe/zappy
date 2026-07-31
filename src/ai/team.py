from client import send_command_to_server
from coordination_state import STATE, TeamPlayerStatus
from strategy import INCANTATION_PLAYERS

TEAM_SIZE = 2
TEAM_STATUS_LIFETIME_TURNS = 128
TEAM_STATUS_INTERVAL = 8


def set_team_name(team_name):

    STATE.team_name = team_name


def remember_team_player_activity(level_text, player_id):

    if not level_text.isdigit() or player_id == STATE.player_id:
        return
    level = int(level_text)
    if level < 1 or level > 8:
        return
    STATE.team_players[player_id] = TeamPlayerStatus(level, STATE.coordination_turn)


def save_team_status(team_name, level_text, player_id):

    if team_name != STATE.team_name or not level_text.isdigit():
        return
    level = int(level_text)
    if level < 1 or level > 8 or player_id == STATE.player_id:
        return
    STATE.team_players[player_id] = TeamPlayerStatus(level, STATE.coordination_turn)


def remove_old_team_players():

    old_players = []

    for player_id, status in STATE.team_players.items():
        waiting_turns = STATE.coordination_turn - status.updated_turn
        if waiting_turns > TEAM_STATUS_LIFETIME_TURNS:
            old_players.append(player_id)

    for player_id in old_players:
        del STATE.team_players[player_id]


def count_team_players_at_level(level):

    remove_old_team_players()
    players = [STATE.player_id] if level == STATE.player_level else []

    for player_id, status in STATE.team_players.items():
        if status.level == level:
            players.append(player_id)
    return len(players)


def get_group_size(level):

    return INCANTATION_PLAYERS.get(level, 0)


def get_incantation_cohort_ids(level=None):

    remove_old_team_players()
    if level is None:
        level = STATE.player_level
    group_size = get_group_size(level)
    if group_size == 0:
        return []
    player_ids = [STATE.player_id] if level == STATE.player_level else []

    for player_id, status in STATE.team_players.items():
        if status.level == level:
            player_ids.append(player_id)
    player_ids.sort()
    return player_ids[:group_size]


def is_incantation_leader():

    cohort = get_incantation_cohort_ids()

    return len(cohort) > 0 and STATE.player_id == cohort[0]


def share_team_status(server_connection):

    if not STATE.team_name:
        return False
    STATE.team_status_turns += 1

    if STATE.team_status_turns < TEAM_STATUS_INTERVAL:
        return False
    STATE.team_status_turns = 0
    message = ("team_status|" + STATE.team_name + "|" + str(STATE.player_level) + "|" + STATE.player_id)

    send_command_to_server(server_connection, "Broadcast " + message)
    return True
