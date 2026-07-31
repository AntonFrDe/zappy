from client import send_command_to_server
from coordination_state import STATE, TeamPlayerStatus
from strategy import (INCANTATION_PLAYERS, INCANTATION_STONES, RESOURCE_PRIORITY, count_visible_item,)

TEAM_SIZE = 7
WINNING_COHORT_SIZE = 6
TEAM_STATUS_LIFETIME_TURNS = 128
TEAM_STATUS_INTERVAL = 8
TEAM_STATUS_RESOURCES = ["food"] + RESOURCE_PRIORITY


def set_team_name(team_name):

    STATE.team_name = team_name


def parse_team_inventory(values):

    if len(values) != len(TEAM_STATUS_RESOURCES):
        return {}
    if not all(value.isdigit() for value in values):
        return {}
    return {resource: int(value)
        for resource, value in zip(TEAM_STATUS_RESOURCES, values)}


def remember_team_player_activity(level_text, player_id):

    if not level_text.isdigit() or player_id == STATE.player_id:
        return
    level = int(level_text)
    if level < 1 or level > 8:
        return
    status = STATE.team_players.get(player_id)
    inventory = status.inventory if status is not None else {}
    STATE.team_players[player_id] = TeamPlayerStatus(level, STATE.coordination_turn, inventory)


def save_team_status(team_name, level_text, player_id, inventory_values=()):

    if team_name != STATE.team_name or not level_text.isdigit():
        return
    level = int(level_text)
    if level < 1 or level > 8 or player_id == STATE.player_id:
        return
    inventory = parse_team_inventory(inventory_values)
    STATE.team_players[player_id] = TeamPlayerStatus(level, STATE.coordination_turn, inventory)


def remove_old_team_players():

    old_players = []

    for player_id, status in STATE.team_players.items():
        waiting_turns = STATE.coordination_turn - status.updated_turn
        if waiting_turns > TEAM_STATUS_LIFETIME_TURNS:
            old_players.append(player_id)

    for player_id in old_players:
        del STATE.team_players[player_id]
    if STATE.team_cohort:
        active_players = set(STATE.team_players)
        active_players.add(STATE.player_id)
        STATE.team_cohort = tuple(player_id for player_id in STATE.team_cohort
            if player_id in active_players)


def count_team_players_at_level(level):

    remove_old_team_players()
    players = [STATE.player_id] if level == STATE.player_level else []

    for player_id, status in STATE.team_players.items():
        if status.level == level:
            players.append(player_id)
    return len(players)


def get_group_size(level):

    minimum_players = INCANTATION_PLAYERS.get(level, 0)

    if minimum_players <= 1:
        return minimum_players
    if not STATE.team_cohort:
        return TEAM_SIZE
    return WINNING_COHORT_SIZE


def get_incantation_cohort_ids(level=None):

    remove_old_team_players()
    if level is None:
        level = STATE.player_level
    group_size = get_group_size(level)
    if group_size == 0:
        return []
    if group_size == 1:
        return [STATE.player_id] if level == STATE.player_level else []
    if STATE.team_cohort:
        player_ids = []

        for player_id in STATE.team_cohort:
            if player_id == STATE.player_id:
                if level == STATE.player_level:
                    player_ids.append(player_id)
                continue
            status = STATE.team_players.get(player_id)
            if status is not None and status.level == level:
                player_ids.append(player_id)
        return player_ids
    player_ids = [STATE.player_id] if level == STATE.player_level else []

    for player_id, status in STATE.team_players.items():
        if status.level == level:
            player_ids.append(player_id)
    player_ids.sort()
    return player_ids


def get_player_resource_targets():

    requirements = INCANTATION_STONES.get(STATE.player_level, {})
    cohort = get_incantation_cohort_ids()

    if not requirements or STATE.player_id not in cohort:
        return {}
    player_index = cohort.index(STATE.player_id)
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
    quantities = {resource: inventory.get(resource, 0) + count_visible_item(tile, resource)
        for resource in requirements}
    cohort = get_incantation_cohort_ids(level)

    for player_id in cohort:
        if player_id == STATE.player_id:
            continue
        status = STATE.team_players.get(player_id)
        if status is None:
            continue
        for resource in requirements:
            quantities[resource] += status.inventory.get(resource, 0)
    return all(quantities[resource] >= count
        for resource, count in requirements.items())


def is_incantation_leader():

    cohort = get_incantation_cohort_ids()

    return len(cohort) > 0 and STATE.player_id == cohort[0]


def share_team_status(server_connection, inventory=None):

    if not STATE.team_name:
        return False
    STATE.team_status_turns += 1

    if STATE.team_status_turns < TEAM_STATUS_INTERVAL:
        return False
    STATE.team_status_turns = 0
    message = ("team_status|" + STATE.team_name + "|" + str(STATE.player_level) + "|" + STATE.player_id)

    if inventory is not None:
        values = [str(inventory.get(resource, 0))
            for resource in TEAM_STATUS_RESOURCES]
        message += "|" + "|".join(values)

    send_command_to_server(server_connection, "Broadcast " + message)
    return True


def remember_successful_cohort(new_level, call):

    if new_level != STATE.player_level + 1:
        return
    if call is None or STATE.player_id not in call.cohort:
        return
    if len(call.cohort) < WINNING_COHORT_SIZE:
        return

    STATE.team_cohort = call.cohort
    for player_id in STATE.team_cohort:
        if player_id == STATE.player_id:
            continue
        status = STATE.team_players.get(player_id)
        inventory = status.inventory if status is not None else {}
        STATE.team_players[player_id] = TeamPlayerStatus(new_level, STATE.coordination_turn, inventory)
