#include "../../include/server/player.h"
#include <stdlib.h>
#include <string.h>

static void init_player_common(player_t *player)
{
    memset(player->inventory, 0, sizeof(player->inventory));
    player->orientation = (direction_t)(1 + rand() % 4);
    player->level = 1;
    player->inventory[RESOURCE_FOOD] = 10;
}

player_t *player_create(map_t *map)
{
    player_t *player = malloc(sizeof(player_t));

    if (player == NULL)
        return NULL;
    init_player_common(player);
    player->x = (int64_t)((uint64_t)rand() % map->width);
    player->y = (int64_t)((uint64_t)rand() % map->height);
    return player;
}

player_t *player_create_at(int64_t x, int64_t y)
{
    player_t *player = malloc(sizeof(player_t));

    if (player == NULL)
        return NULL;
    init_player_common(player);
    player->x = x;
    player->y = y;
    return player;
}

void player_destroy(player_t *player)
{
    if (player == NULL)
        return;
    free(player);
}
