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

// wrap torique: ramene x/y dans [0, width[ / [0, height[ meme si negatif
static int64_t wrap(int64_t v, int64_t max)
{
    return ((v % max) + max) % max;
}

tile_t *map_get_tile(map_t *map, int64_t x, int64_t y)
{
    int64_t wx = wrap(x, (int64_t)map->width);
    int64_t wy = wrap(y, (int64_t)map->height);

    return &map->tiles[wy * (int64_t)map->width + wx];
}
