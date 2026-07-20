from random import choice

RESOURCE_PRIORITY = ["linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame"]
RARE_STONES = ["sibur", "mendiane", "phiras", "thystame"]
EARLY_STONE_DISTRACTIONS = ["deraumere", "sibur"]
GUARDED_STONE_LIMITS = {"sibur": 1, "mendiane": 1, "phiras": 0, "thystame": 0}
FOOD_LOW_LIMIT, FOOD_FULL_LIMIT = 4, 12
SURVIVAL_MOVES = ["Forward", "Left", "Right"]
EXPLORATION_TURNS = ["Left", "Right"]
EXPLORATION_FORWARD_TARGETS = ["food", "linemate", "deraumere"]
INCANTATION_STONES = {
    1: {"linemate": 1},
    2: {"linemate": 1, "deraumere": 1, "sibur": 1},
    3: {"linemate": 2, "sibur": 1, "phiras": 2},
    4: {"linemate": 1, "deraumere": 1, "sibur": 2, "phiras": 1},
    5: {"linemate": 1, "deraumere": 2, "sibur": 1, "mendiane": 3},
    6: {"linemate": 1, "deraumere": 2, "sibur": 3, "phiras": 1},
    7: {"linemate": 2, "deraumere": 2, "sibur": 2, "mendiane": 2, "phiras": 2, "thystame": 1},
}


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


def count_visible_item(tile, item):

    return tile.count(item)


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


def should_take_extra_food(tile):

    return count_visible_item(tile, "food") > 1


def should_skip_food_when_full(tile, inventory):

    if "food" not in tile:
        return False

    return inventory.get("food", 0) > FOOD_FULL_LIMIT


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


def has_required_stones_for_level(level, inventory, tile):

    requirements = INCANTATION_STONES.get(level)
    if requirements is None:
        return False
    for resource, count in requirements.items():
        if inventory.get(resource, 0) + count_visible_item(tile, resource) < count:
            return False
    return True


def can_try_level_two(inventory, tile):

    if inventory.get("food", 0) < 2:
        return False

    return has_required_stones_for_level(1, inventory, tile)


def should_skip_extra_stone(inventory, tile):

    if inventory.get("linemate", 0) > 1 and "linemate" in tile:
        return True
    if inventory.get("deraumere", 0) > 1 and "deraumere" in tile:
        return True
    if inventory.get("sibur", 0) > 1 and "sibur" in tile:
        return True
    return False


def should_turn_near_players(tile):

    if count_visible_item(tile, "player") > 1:
        return True

    return False


def should_leave_player_only_tile(tile):

    player_count = count_visible_item(tile, "player")

    if player_count == 0:
        return False

    return len(tile) == player_count


def should_collect_rare_stone(tile):

    for resource in RARE_STONES:
        if resource in tile:
            return True
    return False


def should_ignore_egg_when_hungry(tile, inventory):

    if "egg" not in tile or "food" in tile:
        return False
    return inventory.get("food", 0) < FOOD_LOW_LIMIT


def should_follow_egg(tile):

    if "egg" in tile:
        return True
    return False


def should_check_inventory(tile):

    if count_visible_item(tile, "player") > 0 and "food" not in tile:
        return True
    return False


def should_turn_on_busy_tile(tile):

    if len(tile) > 4:
        return True
    if count_visible_item(tile, "player") > 0 and len(tile) > 2:
        return True
    return False


def should_take_linemate_first(tile):

    if "linemate" not in tile:
        return False
    if "food" in tile:
        return False
    return True


def should_seek_linemate_before_other_stones(tile, inventory):

    if inventory.get("linemate", 0) > 0:
        return False
    if "linemate" in tile or "food" in tile:
        return False
    if count_visible_item(tile, "player") > 0:
        return False
    return any(resource in tile for resource in EARLY_STONE_DISTRACTIONS)


def should_take_deraumere_first(tile, inventory):

    return should_take_guarded_stone(tile, inventory, "deraumere", 1)


def should_take_guarded_stone(tile, inventory, resource, max_count):

    if resource not in tile or inventory.get(resource, 0) > max_count:
        return False

    if "food" in tile and inventory.get("food", 0) < 5:
        return False

    return count_visible_item(tile, "player") == 0


def get_guarded_resource_to_take(tile, inventory):

    for resource in RARE_STONES:
        limit = GUARDED_STONE_LIMITS[resource]
        if should_take_guarded_stone(tile, inventory, resource, limit):
            return resource
    return None


def should_take_sibur_first(tile, inventory):

    return should_take_guarded_stone(tile, inventory, "sibur", 1)


def should_take_mendiane_first(tile, inventory):

    return should_take_guarded_stone(tile, inventory, "mendiane", 1)


def should_take_phiras_first(tile, inventory):

    return should_take_guarded_stone(tile, inventory, "phiras", 0)


def should_take_thystame_first(tile, inventory):

    return should_take_guarded_stone(tile, inventory, "thystame", 0)


def should_avoid_busy_resource_tile(tile):

    if count_visible_item(tile, "player") == 0:
        return False
    if "linemate" in tile:
        return True
    if "deraumere" in tile:
        return True
    if "sibur" in tile or "mendiane" in tile:
        return True
    return False


def should_seek_food_before_rare_stone(tile, inventory):

    if inventory.get("food", 0) >= FOOD_LOW_LIMIT:
        return False
    if "food" in tile or count_visible_item(tile, "player") > 0:
        return False
    for resource in RARE_STONES:
        if resource in tile:
            return True
    return False
