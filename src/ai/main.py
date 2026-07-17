#!/usr/bin/env python3
import socket
import sys
from random import choice

def read_and_get_arguments(argument_type, default=None):

    if argument_type in sys.argv:
        return sys.argv[sys.argv.index(argument_type) + 1]

    return default

def receive_response_from_server(server_connection):

    data = b""

    while not data.endswith(b"\n"):
        part_of_response = server_connection.recv(1)

        if not part_of_response:
            sys.exit(84)
        data += part_of_response
    normal_text = data.decode().strip()

    if normal_text == "dead":
        sys.exit(0)
    return normal_text

def send_command_to_server(server_connection, command):

    server_connection.sendall((command + "\n").encode())
    return receive_response_from_server(server_connection)

def parse_inventory_response(response):

    inventory = {}
    items = response.strip("[]").split(",")

    for item in items:
        parts = item.split()

        if len(parts) == 2 and parts[1].isdigit():
            inventory[parts[0]] = int(parts[1])

    return inventory

def keep_player_alive(server_connection):

    inventory = parse_inventory_response(send_command_to_server(server_connection, "Inventory"))

    if inventory.get("food", 0) > 5:
        return False

    look = send_command_to_server(server_connection, "Look")
    tile = look.strip("[]").split(",")[0].split()

    if "food" in tile:
        send_command_to_server(server_connection, "Take food")
        return True
    send_command_to_server(server_connection, choice(["Forward", "Left", "Right"]))

    return True

def collect_visible_resource(server_connection, tile):

    for resource in ["linemate", "deraumere",
        "sibur",
        "mendiane",
        "phiras", "thystame",]:

        if resource in tile:
            send_command_to_server(server_connection, "Take " + resource)
            return True

    return False

def count_visible_item(tile, item):

    i = 0

    for y in tile:
        if y == item:
            i = i + 1

    return i

def choose_exploration_command(tile):

    if "player" in tile:
        return "Right"
    if "egg" in tile:
        return "Left"
    if "food" in tile:
        return "Forward"
    if "linemate" in tile:
        return "Forward"
    if "deraumere" in tile:
        return "Forward"
    if len(tile) == 0:
        return choice(["Left", "Right"])

    return choice(["Forward", "Left", "Right"])

def try_first_incantation(server_connection, inventory, tile):
    if inventory.get("linemate", 0) < 1 and "linemate" not in tile:
        return False
    if "linemate" not in tile:
        send_command_to_server(server_connection, "Set linemate")
    answer = send_command_to_server(server_connection, "Incantation")

    if answer == "Elevation underway":
        receive_response_from_server(server_connection)
        return True
    return False

def should_take_food(tile, inventory):
    food_count = count_visible_item(tile, "food")
    if food_count == 0:
        return False
    if inventory.get("food", 0) < 10:
        return True
    if food_count > 2:
        return True
    if len(tile) == food_count:
        return True
    return False

def should_collect_stone(tile):
    if count_visible_item(tile, "linemate") > 0:
        return True
    if count_visible_item(tile, "deraumere") > 0:
        return True
    return False

def should_wait_before_incantation(inventory):
    food, linemate = inventory.get("food", 0), inventory.get("linemate", 0)
    if food == 0:
        return True
    if food < 2:
        return True
    if linemate < 1:
        return True
    if food < 4 and linemate == 0:
        return True
    return False

def move_when_food_is_low(server_connection, inventory, tile):
    if inventory.get("food", 0) >= 3:
        return False
    if "food" in tile:
        send_command_to_server(server_connection, "Take food")
        return True
    send_command_to_server(server_connection, "Forward")
    return True

def turn_on_empty_tile(server_connection, tile):
    if len(tile) != 0:
        return False
    send_command_to_server(server_connection, "Right")
    return True

def should_skip_extra_stone(inventory, tile):
    if inventory.get("linemate", 0) > 1 and "linemate" in tile:
        return True
    if inventory.get("deraumere", 0) > 1 and "deraumere" in tile:
        return True
    if inventory.get("sibur", 0) > 1 and "sibur" in tile:
        return True
    return False

def shold_move_after_look(server_connection, tile):

    smal_tile = len(tile) < 2

    if "food" in tile or should_collect_stone(tile) or not smal_tile:
        return False

    send_command_to_server(server_connection, "Forward")
    return True

def should_turn_near_players(tile):
    if count_visible_item(tile, "player") > 1:
        return True

    return False

def should_collect_rare_stone(tile):
    if "sibur" in tile:
        return True

    if "mendiane" in tile:
        return True
    if "phiras" in tile or "thystame" in tile:
        return True
    return False
def should_follow_egg(tile):
    if "egg" in tile:
        return True
    return False

def should_check_inventory(tile):
    if "player" in tile and "food" not in tile:
        return True
    return False

def should_turn_on_busy_tile(tile):
    if len(tile) > 4:
        return True
    if count_visible_item(tile, "player") > 0 and len(tile) > 2:
        return True
    return False

def main():

    port_argument = int(read_and_get_arguments("-p"))
    team_argument = read_and_get_arguments("-n")
    host_argument = read_and_get_arguments("-h", "localhost")

    with socket.create_connection((host_argument, port_argument)) as server_connection:
        receive_response_from_server(server_connection)
        send_command_to_server(server_connection, team_argument)
        receive_response_from_server(server_connection)

        while True:
            if keep_player_alive(server_connection):
                continue
            look = send_command_to_server(server_connection, "Look")
            tile = look.strip("[]").split(",")[0].split()

            inventory = parse_inventory_response(send_command_to_server(server_connection, "Inventory"))
            if len(tile) == 0:
                send_command_to_server(server_connection, "Look")
            if turn_on_empty_tile(server_connection, tile):
                continue
            if move_when_food_is_low(server_connection, inventory, tile):
                continue
            if should_take_food(tile, inventory):
                send_command_to_server(server_connection, "Take food")
                continue
            if should_wait_before_incantation(inventory):
                send_command_to_server(server_connection, "Look")
                continue
            if try_first_incantation(server_connection, inventory, tile):
                continue
            if should_skip_extra_stone(inventory, tile):
                send_command_to_server(server_connection, "Forward")
                continue
            if shold_move_after_look(server_connection, tile):
                continue
            food_count = count_visible_item(tile, "food")
            if food_count > 1:
                send_command_to_server(server_connection, "Take food")
                continue
            if should_collect_stone(tile):
                collect_visible_resource(server_connection, tile)
                continue
            if should_collect_rare_stone(tile):
                collect_visible_resource(server_connection, tile)
                continue
            if should_follow_egg(tile):
                send_command_to_server(server_connection, "Left")
                continue
            if should_check_inventory(tile):
                send_command_to_server(server_connection, "Inventory")
            if should_turn_on_busy_tile(tile):
                send_command_to_server(server_connection, "Right")
                continue
            if should_turn_near_players(tile):
                send_command_to_server(server_connection, "Left")
                continue
            player_count = count_visible_item(tile, "player")
            if player_count > 0:
                send_command_to_server(server_connection, "Right")
                continue
            if "food" in tile:
                send_command_to_server(server_connection, "Take food")
            elif collect_visible_resource(server_connection, tile):
                continue
            else:
                send_command_to_server(server_connection, choose_exploration_command(tile))

if __name__ == "__main__":
    main()
