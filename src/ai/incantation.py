from strategy import (
    INCANTATION_STONES,
    can_try_level_two,
    count_visible_item,
    has_required_stones_for_level,
)

INCANTATION_PLAYERS = {1: 1, 2: 2, 3: 2, 4: 4, 5: 4, 6: 6, 7: 6}


def remember_incantation_call(message, player_level):

    parts = message.replace(",", "").split()
    if len(parts) != 4 or parts[2] != "incantation" or not parts[1].isdigit() or not parts[3].isdigit():
        return None
    direction, level = int(parts[1]), int(parts[3])
    if level != player_level:
        return None
    return direction, level


def parse_level_response(response):

    if not response.startswith("Current level:"):
        return None
    return int(response.split(":")[1].strip())


def place_missing_stones_for_level(server_connection, inventory, tile, level, send_command_to_server):

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


def try_first_incantation(server_connection, inventory, tile, send_command_to_server,
    receive_response_from_server, update_player_level):

    if not place_missing_stones_for_level(server_connection, inventory, tile, 1, send_command_to_server):
        return False
    reponse = send_command_to_server(server_connection, "Incantation")

    if update_player_level(reponse):
        return True
    if reponse == "Elevation underway":
        reponse = receive_response_from_server(server_connection)
        if update_player_level(reponse):
            return True
    return False


def try_level_two_incantation(server_connection, inventory, tile, send_command_to_server,
    try_first_incantation_callback):

    if not can_try_level_two(inventory, tile):
        return False
    if count_visible_item(tile, "player") > 1:
        send_command_to_server(server_connection, "Right")
        return True
    return try_first_incantation_callback(server_connection, inventory, tile)


def try_current_incantation(server_connection, inventory, tile, player_level,
    send_command_to_server, receive_response_from_server, update_player_level):

    if player_level not in INCANTATION_PLAYERS:
        return False
    if inventory.get("food", 0) < 2:
        return False
    if count_visible_item(tile, "player") + 1 < INCANTATION_PLAYERS[player_level]:
        return False
    if not has_required_stones_for_level(player_level, inventory, tile):
        return False
    if not place_missing_stones_for_level(server_connection, inventory, tile, player_level, send_command_to_server):
        return False
    reponse = send_command_to_server(server_connection, "Incantation")
    if update_player_level(reponse):
        return True
    if reponse != "Elevation underway":
        return False
    return update_player_level(receive_response_from_server(server_connection))


def has_incantation_help_requirements(inventory, tile, player_level, required_players):

    if inventory.get("food", 0) < 4:
        return False
    if count_visible_item(tile, "player") + 1 >= required_players:
        return False
    return has_required_stones_for_level(player_level, inventory, tile)


def broadcast_incantation_help(server_connection, player_level, send_command_to_server):

    send_command_to_server(server_connection, "Broadcast incantation " + str(player_level))


def request_incantation_help(server_connection, inventory, tile, player_level, send_command_to_server):

    required_players = INCANTATION_PLAYERS.get(player_level)
    if required_players is None or required_players <= 1:
        return False
    if not has_incantation_help_requirements(inventory, tile, player_level, required_players):
        return False
    broadcast_incantation_help(server_connection, player_level, send_command_to_server)
    return True


def follow_incantation_call(server_connection, inventory, call, player_level, send_command_to_server):

    if call is None:
        return False
    direction, level = call

    if inventory.get("food", 0) < 3:
        return False
    if level != player_level:
        return False
    if direction == 0:
        send_command_to_server(server_connection, "Look")
        return True
    if direction in (1, 2, 8):
        send_command_to_server(server_connection, "Forward")
        return True
    if direction in (3, 4, 5):
        send_command_to_server(server_connection, "Left")
        return True

    send_command_to_server(server_connection, "Right")
    return True
