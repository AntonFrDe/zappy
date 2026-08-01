from client import send_command_to_server
from coordination import (advance_coordination_turn,
    cancel_incantation_wait,
    is_waiting_for_incantation,)
from coordination_state import STATE
from incantation import (follow_incantation_call,
    request_incantation_help,
    try_current_incantation,)
from spawner import (is_excess_team_player,
    try_create_team_slot,
    update_team_presence,)
from strategy import (FOOD_RESERVE_TARGET,
    FOOD_SURVIVAL_LIMIT,
    INCANTATION_STONES,
    TARGET_LEVEL,
    choose_exploration_command,
    count_visible_item,
    get_current_tile,
    get_missing_stones_for_requirements,
    get_path_to_visible_item,
    parse_inventory_response,)

SURVIVAL_MODE = False


def move_toward_item(server_connection, look, item, take=False):

    path = get_path_to_visible_item(look, item)

    if not path:
        return False
    for command in path:
        send_command_to_server(server_connection, command)
    if take:
        send_command_to_server(server_connection, "Take " + item)

    return True


def take_food(server_connection, inventory, tile, target):

    amount = min(count_visible_item(tile, "food"), max(target - inventory.get("food", 0), 0))

    for _ in range(amount):
        if send_command_to_server(server_connection, "Take food") != "ok":
            break

    return amount > 0


def keep_player_alive(server_connection, inventory):

    global SURVIVAL_MODE

    food = inventory.get("food", 0)

    if not SURVIVAL_MODE and food > FOOD_SURVIVAL_LIMIT:
        return False
    if food >= FOOD_RESERVE_TARGET:
        SURVIVAL_MODE = False
        return False
    SURVIVAL_MODE = True

    look = send_command_to_server(server_connection, "Look")
    tile = get_current_tile(look)

    if take_food(server_connection, inventory, tile, FOOD_RESERVE_TARGET):
        return True
    if move_toward_item(server_connection, look, "food", True):
        return True

    send_command_to_server(server_connection, choose_exploration_command(tile))
    return True


def collect_missing_stone(server_connection, inventory, look, tile):

    if STATE.player_level >= TARGET_LEVEL:
        return False

    requirements = INCANTATION_STONES.get(STATE.player_level, {})

    for stone in get_missing_stones_for_requirements(requirements, inventory):
        if stone in tile:
            send_command_to_server(server_connection, "Take " + stone)
            return True
        if move_toward_item(server_connection, look, stone, True):
            return True

    return False


def handle_waiting_incantation(server_connection, inventory):

    if inventory.get("food", 0) < FOOD_SURVIVAL_LIMIT:
        cancel_incantation_wait()
        return keep_player_alive(server_connection, inventory)
    if follow_incantation_call(server_connection, inventory):
        return True

    look = send_command_to_server(server_connection, "Look")
    tile = get_current_tile(look)

    return (try_current_incantation(server_connection, inventory, tile)
        or request_incantation_help(server_connection, inventory, tile))


def run_ai_loop(server_connection):

    while True:

        response = send_command_to_server(server_connection, "Inventory")
        inventory = parse_inventory_response(response)
        advance_coordination_turn()
        update_team_presence(server_connection)

        if is_excess_team_player():
            return
        if is_waiting_for_incantation():
            if handle_waiting_incantation(server_connection, inventory):
                continue
        if try_create_team_slot(server_connection, inventory):
            continue
        if keep_player_alive(server_connection, inventory):
            continue
        if follow_incantation_call(server_connection, inventory):
            continue

        look = send_command_to_server(server_connection, "Look")
        tile = get_current_tile(look)

        if inventory.get("food", 0) < FOOD_RESERVE_TARGET:
            if take_food(server_connection, inventory, tile, FOOD_RESERVE_TARGET):
                continue
            if move_toward_item(server_connection, look, "food", True):
                continue
        if collect_missing_stone(server_connection, inventory, look, tile):
            continue
        if try_current_incantation(server_connection, inventory, tile):
            continue
        if request_incantation_help(server_connection, inventory, tile):
            continue
        if take_food(server_connection, inventory, tile, FOOD_RESERVE_TARGET):
            continue
        send_command_to_server(server_connection, choose_exploration_command(tile))
