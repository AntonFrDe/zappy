#!/usr/bin/env python3
import socket
import sys
from pathlib import Path
from random import choice

AI_DIR = Path(__file__).resolve().parent
PROJECT_AI_DIR = AI_DIR / "src" / "ai"
for import_path in (AI_DIR, PROJECT_AI_DIR):
    if import_path.exists() and str(import_path) not in sys.path:
        sys.path.insert(0, str(import_path))

from strategy import (
    EXPLORATION_FORWARD_TARGETS,
    EXPLORATION_TURNS,
    INCANTATION_STONES,
    RESOURCE_PRIORITY,
    SURVIVAL_MOVES,
    can_try_level_two,
    choose_direction_toward_item,
    count_visible_item,
    get_current_tile,
    get_guarded_resource_to_take,
    has_required_stones_for_level,
    parse_look_response,
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

PLAYER_LEVEL = 1
INCANTATION_PLAYERS = {1: 1, 2: 2, 3: 2, 4: 4, 5: 4, 6: 6, 7: 6}
LAST_INCANTATION_CALL = None
INCANTATION_HELP_SENT = False


def read_and_get_arguments(argument_type, default=None):

    if argument_type in sys.argv:
        return sys.argv[sys.argv.index(argument_type) + 1]

    return default


def receive_response_from_server(server_connection):

    while True:
        data = b""

        while not data.endswith(b"\n"):
            part_of_response = server_connection.recv(1)
            if not part_of_response:
                sys.exit(84)
            data += part_of_response

        normal_text = data.decode().strip()

        if normal_text == "dead":
            sys.exit(0)
        if normal_text.startswith("message "):
            remember_incantation_call(normal_text)
            continue
        if normal_text.startswith("eject:"):
            continue

        return normal_text


def remember_incantation_call(message):

    global LAST_INCANTATION_CALL

    parts = message.replace(",", "").split()
    if len(parts) != 4 or parts[2] != "incantation" or not parts[1].isdigit() or not parts[3].isdigit():
        return
    direction, level = int(parts[1]), int(parts[3])
    if level != PLAYER_LEVEL:
        return
    LAST_INCANTATION_CALL = (direction, level)


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
    direction = choose_direction_toward_item(look, "food")
    if direction is not None:
        send_command_to_server(server_connection, direction)
        return True

    send_command_to_server(server_connection, choice(SURVIVAL_MOVES))

    return True


def collect_visible_resource(server_connection, tile):

    for resource in get_resource_priority():

        if resource in tile:
            send_command_to_server(server_connection, "Take " + resource)
            return True

    return False


def get_resource_priority():

    needed_resources = list(INCANTATION_STONES.get(PLAYER_LEVEL, {}).keys())

    return needed_resources + [resource for resource in RESOURCE_PRIORITY if resource not in needed_resources]


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


def update_player_level(response):

    global PLAYER_LEVEL, INCANTATION_HELP_SENT

    if not response.startswith("Current level:"):
        return False
    PLAYER_LEVEL = int(response.split(":")[1].strip())
    INCANTATION_HELP_SENT = False
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
        reponse = receive_response_from_server(server_connection)
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
    if count_visible_item(tile, "player") + 1 < INCANTATION_PLAYERS[PLAYER_LEVEL]:
        return False
    if not has_required_stones_for_level(PLAYER_LEVEL, inventory, tile):
        return False
    if not place_missing_stones_for_level(server_connection, inventory, tile, PLAYER_LEVEL):
        return False
    reponse = send_command_to_server(server_connection, "Incantation")
    if update_player_level(reponse):
        return True
    if reponse != "Elevation underway":
        return False
    return update_player_level(receive_response_from_server(server_connection))


def request_incantation_help(server_connection, inventory, tile):

    global INCANTATION_HELP_SENT

    if INCANTATION_HELP_SENT:
        return False
    required_players = INCANTATION_PLAYERS.get(PLAYER_LEVEL)
    if required_players is None or required_players <= 1:
        return False
    if inventory.get("food", 0) < 4:
        return False
    if count_visible_item(tile, "player") + 1 >= required_players:
        return False
    if not has_required_stones_for_level(PLAYER_LEVEL, inventory, tile):
        return False
    send_command_to_server(server_connection, "Broadcast incantation " + str(PLAYER_LEVEL))
    INCANTATION_HELP_SENT = True
    return True


def follow_incantation_call(server_connection, inventory):

    global LAST_INCANTATION_CALL

    call = LAST_INCANTATION_CALL
    LAST_INCANTATION_CALL = None

    if call is None:
        return False
    direction, level = call

    if inventory.get("food", 0) < 3:
        return False
    if level != PLAYER_LEVEL or direction == 0:
        return False
    if direction in (1, 2, 8):
        send_command_to_server(server_connection, "Forward")
        return True
    if direction in (3, 4, 5):
        send_command_to_server(server_connection, "Left")
        return True

    send_command_to_server(server_connection, "Right")
    return True


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

    if inventory.get("food", 0) < 8:
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
    direction = "Forward"
    if isinstance(look, str):
        direction = choose_direction_toward_item(look, resource)
        if direction is None:
            return False
    send_command_to_server(server_connection, direction)
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


def handle_player_pressure_step(server_connection, tile):

    if should_check_inventory(tile):
        send_command_to_server(server_connection, "Inventory")
    if should_turn_on_busy_tile(tile):
        send_command_to_server(server_connection, "Right")
        return True
    if should_turn_near_players(tile):
        send_command_to_server(server_connection, "Left")
        return True
    if count_visible_item(tile, "player") > 1:
        send_command_to_server(server_connection, "Right")
        return True
    return False


def avoid_crowded_tile_step(server_connection, tile):

    if not should_avoid_busy_resource_tile(tile) and not should_leave_player_only_tile(tile):
        return False
    send_command_to_server(server_connection, "Right")
    return True


def take_guarded_resource_step(server_connection, inventory, tile):

    guarded_resource = get_guarded_resource_to_take(tile, inventory)
    if guarded_resource is None:
        return False
    send_command_to_server(server_connection, "Take " + guarded_resource)
    return True


def handle_visible_food_step(server_connection, inventory, tile):

    if should_skip_food_when_full(tile, inventory):
        send_command_to_server(server_connection, "Forward")
        return True
    if should_take_food(tile, inventory):
        send_command_to_server(server_connection, "Take food")
        return True
    return False


def collect_visible_stone_step(server_connection, tile):

    if not should_collect_stone(tile) and not should_collect_rare_stone(tile):
        return False
    collect_visible_resource(server_connection, tile)
    return True


def handle_early_resource_step(server_connection, inventory, tile):

    if should_take_linemate_first(tile):
        send_command_to_server(server_connection, "Take linemate")
        return True
    if should_seek_linemate_before_other_stones(tile, inventory):
        send_command_to_server(server_connection, "Forward")
        return True
    if should_take_deraumere_first(tile, inventory):
        send_command_to_server(server_connection, "Take deraumere")
        return True
    if should_seek_food_before_rare_stone(tile, inventory):
        send_command_to_server(server_connection, "Forward")
        return True
    return False


def run_ai_loop(server_connection):

    while True:
        if keep_player_alive(server_connection):
            continue
        look = send_command_to_server(server_connection, "Look")
        tile = get_current_tile(look)

        inventory = parse_inventory_response(send_command_to_server(server_connection, "Inventory"))
        if prepare_incantation_step(server_connection, inventory, look, tile):
            continue
        if try_current_incantation(server_connection, inventory, tile):
            continue
        if len(tile) == 0:
            send_command_to_server(server_connection, "Look")
        if turn_on_empty_tile(server_connection, tile):
            continue
        if move_when_food_is_low(server_connection, inventory, tile):
            continue
        if request_incantation_help(server_connection, inventory, tile):
            continue
        if follow_incantation_call(server_connection, inventory):
            continue
        if try_create_team_slot(server_connection, inventory, tile):
            continue
        if if_low_food_seek_food(server_connection, inventory, tile):
            continue
        if avoid_crowded_tile_step(server_connection, tile):
            continue
        if handle_early_resource_step(server_connection, inventory, tile):
            continue
        if take_guarded_resource_step(server_connection, inventory, tile):
            continue
        if handle_visible_food_step(server_connection, inventory, tile):
            continue
        if should_skip_extra_stone(inventory, tile):
            send_command_to_server(server_connection, "Forward")
            continue
        if shold_move_after_look(server_connection, tile):
            continue
        if should_take_extra_food(tile):
            send_command_to_server(server_connection, "Take food")
            continue
        if collect_visible_stone_step(server_connection, tile):
            continue
        if should_ignore_egg_when_hungry(tile, inventory):
            send_command_to_server(server_connection, "Forward")
            continue
        if should_follow_egg(tile):
            send_command_to_server(server_connection, "Left")
            continue
        if handle_player_pressure_step(server_connection, tile):
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
