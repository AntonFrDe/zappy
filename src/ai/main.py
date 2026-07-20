#!/usr/bin/env python3
import socket
import sys
from pathlib import Path
from random import choice

AI_DIR = Path(__file__).resolve().parent
if str(AI_DIR) not in sys.path:
    sys.path.insert(0, str(AI_DIR))

from strategy import (
    EXPLORATION_FORWARD_TARGETS,
    EXPLORATION_TURNS,
    RESOURCE_PRIORITY,
    SURVIVAL_MOVES,
    can_try_level_two,
    count_visible_item,
    get_current_tile,
    get_guarded_resource_to_take,
    parse_inventory_response,
    should_avoid_busy_resource_tile,
    should_check_inventory,
    should_collect_rare_stone,
    should_collect_stone,
    should_follow_egg,
    should_ignore_egg_when_hungry,
    should_leave_player_only_tile,
    should_seek_food_before_rare_stone,
    should_seek_linemate_before_other_stones,
    should_skip_extra_stone,
    should_skip_food_when_full,
    should_take_deraumere_first,
    should_take_extra_food,
    should_take_food,
    should_take_guarded_stone,
    should_take_linemate_first,
    should_take_mendiane_first,
    should_take_phiras_first,
    should_take_sibur_first,
    should_take_thystame_first,
    should_turn_near_players,
    should_turn_on_busy_tile,
    should_wait_before_incantation,
)


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


def keep_player_alive(server_connection):

    inventory = parse_inventory_response(send_command_to_server(server_connection, "Inventory"))

    if inventory.get("food", 0) > 5:
        return False

    look = send_command_to_server(server_connection, "Look")
    tile = get_current_tile(look)

    if "food" in tile:
        send_command_to_server(server_connection, "Take food")
        return True

    send_command_to_server(server_connection, choice(SURVIVAL_MOVES))

    return True


def collect_visible_resource(server_connection, tile):

    for resource in RESOURCE_PRIORITY:

        if resource in tile:
            send_command_to_server(server_connection, "Take " + resource)
            return True

    return False


def choose_exploration_command(tile):

    if count_visible_item(tile, "player") > 0:
        return "Right"
    if "egg" in tile:
        return "Left"
    for resource in EXPLORATION_FORWARD_TARGETS:
        if resource in tile:
            return "Forward"
    if len(tile) == 0:
        return choice(EXPLORATION_TURNS)

    return choice(SURVIVAL_MOVES)


def try_first_incantation(server_connection, inventory, tile):

    if inventory.get("linemate", 0) < 1 and "linemate" not in tile:
        return False
    if "linemate" not in tile:
        reponse = send_command_to_server(server_connection, "Set linemate")
        if reponse != "ok":
            return False
    reponse = send_command_to_server(server_connection, "Incantation")

    if reponse == "Elevation underway":
        reponse = receive_response_from_server(server_connection)
        if reponse.startswith("Current level:"):
            return True
    return False


def try_level_two_incantation(server_connection, inventory, tile):
    if not can_try_level_two(inventory, tile):
        return False
    if count_visible_item(tile, "player") > 1:
        send_command_to_server(server_connection, "Right")
        return True
    return try_first_incantation(server_connection, inventory, tile)


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


def shold_move_after_look(server_connection, tile):

    smal_tile = len(tile) < 2

    if "food" in tile or should_collect_stone(tile) or not smal_tile:
        return False

    send_command_to_server(server_connection, "Forward")
    return True


def if_low_food_seek_food(server_connection, inventory, tile):

    food = inventory.get("food", 0)

    if food > 8:
        return False
    if "food" not in tile:
        return False
    if count_visible_item(tile, "player") > 1:
        send_command_to_server(server_connection, "Take food")
        return True
    if count_visible_item(tile, "food") > 0:
        send_command_to_server(server_connection, "Take food")
        return True
    return False


def try_create_team_slot(server_connection, inventory, tile):

    if can_try_level_two(inventory, tile):
        return False
    if inventory.get("food", 0) < 8:
        return False
    if inventory.get("linemate", 0) < 1:
        return False
    reponse = send_command_to_server(server_connection, "Connect_nbr")
    if not reponse.isdigit():
        return False
    if int(reponse) > 0:
        return False
    send_command_to_server(server_connection, "Fork")
    return True


def take_needed_item_on_tile(server_connection, inventory, tile):

    if inventory.get("food", 0) < 2 and "food" in tile:
        send_command_to_server(server_connection, "Take food")
        return True
    if inventory.get("linemate", 0) < 1 and "linemate" in tile:
        send_command_to_server(server_connection, "Take linemate")
        return True
    return False


def move_toward_missing_item(server_connection, inventory, look, resource, count):

    if inventory.get(resource, 0) >= count or resource not in look:
        return False
    send_command_to_server(server_connection, "Forward")
    return True


def wait_before_incantation(server_connection, inventory, look, tile):

    if take_needed_item_on_tile(server_connection, inventory, tile):
        return
    if move_toward_missing_item(server_connection, inventory, look, "linemate", 1):
        return
    if move_toward_missing_item(server_connection, inventory, look, "food", 2):
        return
    if inventory.get("linemate", 0) < 1 or inventory.get("food", 0) < 2:
        send_command_to_server(server_connection, choose_exploration_command(tile))
        return

    send_command_to_server(server_connection, "Look")


def prepare_incantation_step(server_connection, inventory, look, tile):

    if not should_wait_before_incantation(inventory):
        return False
    wait_before_incantation(server_connection, inventory, look, tile)
    return True


def do_default_action(server_connection, tile):

    if "food" in tile:
        send_command_to_server(server_connection, "Take food")
        return
    if collect_visible_resource(server_connection, tile):
        return
    send_command_to_server(server_connection, choose_exploration_command(tile))


def run_ai_loop(server_connection):

    while True:
        if keep_player_alive(server_connection):
            continue
        look = send_command_to_server(server_connection, "Look")
        tile = get_current_tile(look)

        inventory = parse_inventory_response(send_command_to_server(server_connection, "Inventory"))
        if prepare_incantation_step(server_connection, inventory, look, tile):
            continue
        if len(tile) == 0:
            send_command_to_server(server_connection, "Look")
        if turn_on_empty_tile(server_connection, tile):
            continue
        if move_when_food_is_low(server_connection, inventory, tile):
            continue
        if try_level_two_incantation(server_connection, inventory, tile):
            continue
        if try_create_team_slot(server_connection, inventory, tile):
            continue
        if if_low_food_seek_food(server_connection, inventory, tile):
            continue
        if should_avoid_busy_resource_tile(tile):
            send_command_to_server(server_connection, "Right")
            continue
        if should_leave_player_only_tile(tile):
            send_command_to_server(server_connection, "Right")
            continue
        if should_take_linemate_first(tile):
            send_command_to_server(server_connection, "Take linemate")
            continue
        if should_seek_linemate_before_other_stones(tile, inventory):
            send_command_to_server(server_connection, "Forward")
            continue
        if should_take_deraumere_first(tile, inventory):
            send_command_to_server(server_connection, "Take deraumere")
            continue
        if should_seek_food_before_rare_stone(tile, inventory):
            send_command_to_server(server_connection, "Forward")
            continue
        guarded_resource = get_guarded_resource_to_take(tile, inventory)
        if guarded_resource is not None:
            send_command_to_server(server_connection, "Take " + guarded_resource)
            continue
        if should_skip_food_when_full(tile, inventory):
            send_command_to_server(server_connection, "Forward")
            continue
        if should_take_food(tile, inventory):
            send_command_to_server(server_connection, "Take food")
            continue
        if should_skip_extra_stone(inventory, tile):
            send_command_to_server(server_connection, "Forward")
            continue
        if shold_move_after_look(server_connection, tile):
            continue
        if should_take_extra_food(tile):
            send_command_to_server(server_connection, "Take food")
            continue
        if should_collect_stone(tile):
            collect_visible_resource(server_connection, tile)
            continue
        if should_collect_rare_stone(tile):
            collect_visible_resource(server_connection, tile)
            continue
        if should_ignore_egg_when_hungry(tile, inventory):
            send_command_to_server(server_connection, "Forward")
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
        if player_count > 1:
            send_command_to_server(server_connection, "Right")
            continue
        do_default_action(server_connection, tile)


def main():

    port_argument = int(read_and_get_arguments("-p"))
    team_argument = read_and_get_arguments("-n")
    host_argument = read_and_get_arguments("-h", "localhost")

    with socket.create_connection((host_argument, port_argument)) as server_connection:
        receive_response_from_server(server_connection)
        send_command_to_server(server_connection, team_argument)
        receive_response_from_server(server_connection)
        run_ai_loop(server_connection)


if __name__ == "__main__":
    main()
