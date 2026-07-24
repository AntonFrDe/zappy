from random import choice

from client import send_command_to_server
from incantation import (follow_incantation_call,
    get_resource_priority,
    request_incantation_help,
    try_current_incantation,)
from spawner import try_create_team_slot
from strategy import (EXPLORATION_FORWARD_TARGETS,
    SURVIVAL_MOVES,
    choose_direction_toward_item,
    count_other_players,
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
    should_take_linemate_first,
    should_turn_near_players,
    should_turn_on_busy_tile,
    should_wait_before_incantation,)

SURVIVAL_FOOD_LIMIT = 8


def keep_player_alive(server_connection):

    inventory = parse_inventory_response(send_command_to_server(server_connection, "Inventory"))

    if inventory.get("food", 0) > SURVIVAL_FOOD_LIMIT:
        return False

    look = send_command_to_server(server_connection, "Look")
    tile = get_current_tile(look)

    if "food" in tile:
        for _ in range(count_visible_item(tile, "food")):
            if send_command_to_server(server_connection, "Take food") != "ok":
                break
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


def choose_exploration_command(tile):

    player_count = count_visible_item(tile, "player")

    if count_other_players(tile) > 0:
        return "Right"
    if "egg" in tile:
        return "Left"
    for resource in EXPLORATION_FORWARD_TARGETS:
        if resource in tile:
            return "Forward"
    if len(tile) == 0 or len(tile) == player_count:
        return "Forward"

    return choice(SURVIVAL_MOVES)


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

    send_command_to_server(server_connection, "Forward")
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

    if should_skip_food_when_full(tile, inventory) and not should_collect_stone(tile) and not should_collect_rare_stone(tile):
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
        if follow_incantation_call(server_connection, inventory):
            continue
        if try_create_team_slot(server_connection, inventory, tile):
            continue
        if prepare_incantation_step(server_connection, inventory, look, tile):
            continue
        if try_current_incantation(server_connection, inventory, tile):
            continue
        if request_incantation_help(server_connection, inventory, tile):
            continue
        if len(tile) == 0:
            send_command_to_server(server_connection, "Look")
        if turn_on_empty_tile(server_connection, tile):
            continue
        if move_when_food_is_low(server_connection, inventory, tile):
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
