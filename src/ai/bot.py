from client import send_command_to_server
from coordination import (advance_coordination_turn,
    cancel_incantation_wait,
    is_waiting_for_incantation,)
from coordination_state import STATE
from incantation import (follow_incantation_call,
    request_incantation_help,
    try_current_incantation,)
from spawner import try_create_team_slot
from strategy import (FOOD_RESERVE_TARGET,
    FOOD_SURVIVAL_LIMIT,
    INCANTATION_STONES,
    TARGET_LEVEL,
    choose_exploration_command,
    count_visible_item,
    get_path_to_visible_item,
    get_current_tile,
    get_missing_stones_for_requirements,
    parse_inventory_response,)
from team import share_team_status

SURVIVAL_STATE = {
    "active": False,
    "run_length": 3,
    "steps_left": 3,
    "turns": 0,}


def next_survival_move():

    if SURVIVAL_STATE["steps_left"] > 0:
        SURVIVAL_STATE["steps_left"] -= 1
        return "Forward"
    SURVIVAL_STATE["turns"] += 1
    if SURVIVAL_STATE["turns"] >= 2:
        SURVIVAL_STATE["turns"] = 0
        SURVIVAL_STATE["run_length"] += 1
    SURVIVAL_STATE["steps_left"] = SURVIVAL_STATE["run_length"]
    return "Right"


def keep_player_alive(server_connection, inventory=None):

    if inventory is None:
        response = send_command_to_server(server_connection, "Inventory")
        inventory = parse_inventory_response(response)
    food = inventory.get("food", 0)

    if not SURVIVAL_STATE["active"] and food > FOOD_SURVIVAL_LIMIT:
        return False

    if food >= FOOD_RESERVE_TARGET:
        SURVIVAL_STATE.update(active=False, run_length=3, steps_left=3, turns=0)
        return False

    SURVIVAL_STATE["active"] = True

    look = send_command_to_server(server_connection, "Look")
    tile = get_current_tile(look)

    if take_food_to_target(server_connection, inventory, tile, FOOD_RESERVE_TARGET):
        return True

    if move_toward_visible_item(server_connection, look, "food", True):
        return True

    send_command_to_server(server_connection, next_survival_move())

    return True


def collect_missing_stone(server_connection, inventory, look, tile):

    if STATE.player_level >= TARGET_LEVEL:
        return False
    targets = INCANTATION_STONES.get(STATE.player_level, {})
    missing_stones = get_missing_stones_for_requirements(targets, inventory)

    for stone in missing_stones:
        if stone in tile:
            send_command_to_server(server_connection, "Take " + stone)
            return True

    for stone in missing_stones:
        if move_toward_visible_item(server_connection, look, stone, True):
            return True

    return False


def move_toward_visible_item(server_connection, look, item, take_on_arrival=False):

    path = get_path_to_visible_item(look, item)

    if path is None or len(path) == 0:
        return False
    for command in path:
        send_command_to_server(server_connection, command)
    if take_on_arrival:
        send_command_to_server(server_connection, "Take " + item)
    return True


def take_food_to_target(server_connection, inventory, tile, target):

    food_to_take = min(count_visible_item(tile, "food"), max(target - inventory.get("food", 0), 0))

    if food_to_take == 0:
        return False
    for _ in range(food_to_take):
        if send_command_to_server(server_connection, "Take food") != "ok":
            break
    return True


def collect_food(server_connection, inventory, tile):

    if "food" in tile and inventory.get("food", 0) < FOOD_RESERVE_TARGET:
        send_command_to_server(server_connection, "Take food")
        return True
    return False


def handle_waiting_incantation_step(server_connection, inventory):

    if inventory.get("food", 0) < 4:
        cancel_incantation_wait()
        return keep_player_alive(server_connection, inventory)
    if follow_incantation_call(server_connection, inventory):
        return True
    look = send_command_to_server(server_connection, "Look")
    tile = get_current_tile(look)
    if try_current_incantation(server_connection, inventory, tile):
        return True
    return request_incantation_help(server_connection, inventory, tile)


def handle_urgent_coordination_step(server_connection, inventory):

    if is_waiting_for_incantation():
        return handle_waiting_incantation_step(server_connection, inventory)
    if keep_player_alive(server_connection, inventory):
        return True
    return follow_incantation_call(server_connection, inventory)


def run_ai_loop(server_connection):

    while True:
        response = send_command_to_server(server_connection, "Inventory")
        inventory = parse_inventory_response(response)
        advance_coordination_turn()
        if handle_urgent_coordination_step(server_connection, inventory):
            continue
        if try_create_team_slot(server_connection, inventory):
            continue
        if share_team_status(server_connection):
            continue
        look = send_command_to_server(server_connection, "Look")
        tile = get_current_tile(look)
        if collect_missing_stone(server_connection, inventory, look, tile):
            continue
        if try_current_incantation(server_connection, inventory, tile):
            continue
        if request_incantation_help(server_connection, inventory, tile):
            continue
        if collect_food(server_connection, inventory, tile):
            continue
        send_command_to_server(server_connection, choose_exploration_command(tile))
