from math import isqrt
from random import choice

FOOD_SURVIVAL_LIMIT = 8
FOOD_RESERVE_TARGET = 15
TARGET_LEVEL = 4
EXPLORATION_FORWARD_TARGETS = ["food", "linemate", "deraumere"]
EXPLORATION_MOVES = ["Forward", "Forward", "Forward", "Left", "Right"]
INCANTATION_STONES = {1: {"linemate": 1},
    2: {"linemate": 1, "deraumere": 1, "sibur": 1},
    3: {"linemate": 2, "sibur": 1, "phiras": 2},}
INCANTATION_PLAYERS = {1: 1, 2: 2, 3: 2}


def parse_inventory_response(response):

    inventory = {}
    items = response.strip("[]").split(",")

    for item in items:

        parts = item.split()

        if len(parts) == 2 and parts[1].isdigit():
            inventory[parts[0]] = int(parts[1])

    return inventory


def get_current_tile(look):

    return look.strip("[]").split(",")[0].split()


def parse_look_response(look):

    return [tile.split() for tile in look.strip("[]").split(",")]


def find_visible_item_position(look, item):

    tiles = parse_look_response(look)

    for index, tile in enumerate(tiles):
        if item not in tile:
            continue

        row = isqrt(index)
        lateral = index - row * row - row

        return row, lateral
    return None


def get_path_to_visible_item(look, item):

    position = find_visible_item_position(look, item)

    if position is None:
        return None

    row, lateral = position

    if row == 0:
        return []

    path = ["Forward"] * row

    if lateral < 0:
        path.append("Left")
    elif lateral > 0:
        path.append("Right")

    path.extend(["Forward"] * abs(lateral))

    return path


def count_visible_item(tile, item):

    return tile.count(item)


def choose_exploration_command(tile):

    for resource in EXPLORATION_FORWARD_TARGETS:
        if resource in tile:
            return "Forward"

    return choice(EXPLORATION_MOVES)


def has_required_stones_for_level(level, inventory, tile):

    requirements = INCANTATION_STONES.get(level)

    if requirements is None:
        return False
    for resource, count in requirements.items():
        if inventory.get(resource, 0) + count_visible_item(tile, resource) < count:
            return False
    return True


def get_missing_stones_for_requirements(requirements, inventory):

    missing_stones = []

    for stone, needed in requirements.items():
        if inventory.get(stone, 0) < needed:
            missing_stones.append(stone)

    return missing_stones
