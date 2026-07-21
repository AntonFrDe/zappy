#include "../../include/server/player.h"
#include <stdlib.h>
#include <string.h>

player_t *player_create(map_t *map)
{
    player_t *player = malloc(sizeof(player_t));

    if (player == NULL)
        return NULL;
    memset(player->inventory, 0, sizeof(player->inventory));
    player->x = (int64_t)((uint64_t)rand() % map->width);
    player->y = (int64_t)((uint64_t)rand() % map->height);
    player->orientation = (direction_t)(1 + rand() % 4);
    player->level = 1;
    player->inventory[RESOURCE_FOOD] = 10;
    return player;
}

void player_destroy(player_t *player)
{
    if (player == NULL)
        return;
    free(player);
}
