from strategy import (
    choose_direction_toward_item,
    count_visible_item,
    get_current_tile,
    parse_inventory_response,
)

SURVIVAL_FOOD_LIMIT = 8
SURVIVAL_SEARCH_MOVES = ["Forward", "Forward", "Left"]
SURVIVAL_SEARCH_INDEX = 0


def choose_survival_search_command():

    global SURVIVAL_SEARCH_INDEX

    command = SURVIVAL_SEARCH_MOVES[SURVIVAL_SEARCH_INDEX]
    SURVIVAL_SEARCH_INDEX = (SURVIVAL_SEARCH_INDEX + 1) % len(SURVIVAL_SEARCH_MOVES)
    return command


def reset_survival_search():

    global SURVIVAL_SEARCH_INDEX

    SURVIVAL_SEARCH_INDEX = 0


def keep_player_alive(server_connection, send_command_to_server):

    inventory = parse_inventory_response(send_command_to_server(server_connection, "Inventory"))

    if inventory.get("food", 0) > SURVIVAL_FOOD_LIMIT:
        reset_survival_search()
        return False

    look = send_command_to_server(server_connection, "Look")
    tile = get_current_tile(look)

    if "food" in tile:
        for _ in range(count_visible_item(tile, "food")):
            if send_command_to_server(server_connection, "Take food") != "ok":
                break
        reset_survival_search()
        return True
    direction = choose_direction_toward_item(look, "food")
    if direction is not None:
        send_command_to_server(server_connection, direction)
        reset_survival_search()
        return True

    send_command_to_server(server_connection, choose_survival_search_command())
    return True
