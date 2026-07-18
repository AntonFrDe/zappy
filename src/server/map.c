#include "../../include/server/map.h"
#include <stdlib.h>

map_t *map_create(uint64_t width, uint64_t height)
{
    map_t *map = malloc(sizeof(map_t));

    if (map == NULL)
        return NULL;
    map->width = width;
    map->height = height;
    map->tiles = calloc((size_t)(width * height), sizeof(tile_t));
    if (map->tiles == NULL) {
        free(map);
        return NULL;
    }
    return map;
}

void map_destroy(map_t *map)
{
    if (map == NULL)
        return;
    free(map->tiles);
    free(map);
}

// Empeche la creation de map qui ferai crash
static int64_t secure_mod(int64_t v, int64_t max)
{
    return ((v % max) + max) % max;
}

tile_t *map_get_tile(map_t *map, int64_t x, int64_t y)
{
    int64_t wx = secure_mod(x, (int64_t)map->width);
    int64_t wy = secure_mod(y, (int64_t)map->height);

    return &map->tiles[wy * (int64_t)map->width + wx];
}

void map_move(map_t *map, int64_t *x, int64_t *y, direction_t dir)
{
    switch (dir) {
    case DIR_NORTH:
        *y -= 1;
        break;
    case DIR_EAST:
        *x += 1;
        break;
    case DIR_SOUTH:
        *y += 1;
        break;
    case DIR_WEST:
        *x -= 1;
        break;
    }
    *x = secure_mod(*x, (int64_t)map->width);
    *y = secure_mod(*y, (int64_t)map->height);
}
