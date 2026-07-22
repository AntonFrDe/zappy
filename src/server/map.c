#include "../../include/server/map.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// densite par ressource, cf sujet (ex: 10x10 -> 50 food, 5 thystame)
static const double RESOURCE_DENSITY[RESOURCE_COUNT] = {
    [RESOURCE_FOOD] = 0.5,
    [RESOURCE_LINEMATE] = 0.3,
    [RESOURCE_DERAUMERE] = 0.15,
    [RESOURCE_SIBUR] = 0.1,
    [RESOURCE_MENDIANE] = 0.1,
    [RESOURCE_PHIRAS] = 0.08,
    [RESOURCE_THYSTAME] = 0.05,
};

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

// arrondi au plus proche, jamais moins de 1 (garanti par le sujet)
static uint16_t resource_count_for(uint64_t width, uint64_t height, double density)
{
    double raw = (double)(width * height) * density;
    uint16_t count = (uint16_t)(raw + 0.5);

    return count < 1 ? 1 : count;
}

void map_spawn_resources(map_t *map)
{
    uint64_t nb_tiles = map->width * map->height;
    uint16_t count;
    uint64_t idx;

    for (int r = 0; r < RESOURCE_COUNT; r++) {
        count = resource_count_for(map->width, map->height, RESOURCE_DENSITY[r]);
        for (uint16_t i = 0; i < count; i++) {
            idx = (uint64_t)rand() % nb_tiles;
            map->tiles[idx].quantities[r]++;
        }
    }
}

static const char *RESOURCE_NAMES[RESOURCE_COUNT] = {
    "food", "linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame"
};

const char *resource_name(resource_t r)
{
    return RESOURCE_NAMES[r];
}

int resource_from_name(const char *name)
{
    for (int r = 0; r < RESOURCE_COUNT; r++)
        if (strcmp(RESOURCE_NAMES[r], name) == 0)
            return r;
    return -1;
}

static int64_t shortest_delta(int64_t raw, int64_t size)
{
    int64_t d = ((raw % size) + size) % size;

    if (d > size / 2)
        d -= size;
    return d;
}

static void rotate_to_frame(direction_t orientation, int64_t dx, int64_t dy,
    int64_t *rx, int64_t *ry)
{
    switch (orientation) {
    case DIR_NORTH:
        *rx = dx;
        *ry = dy;
        break;
    case DIR_EAST:
        *rx = dy;
        *ry = -dx;
        break;
    case DIR_SOUTH:
        *rx = -dx;
        *ry = -dy;
        break;
    case DIR_WEST:
        *rx = -dy;
        *ry = dx;
        break;
    }
}

int map_direction(map_t *map, int64_t from_x, int64_t from_y,
    direction_t from_orientation, int64_t to_x, int64_t to_y)
{
    int64_t dx = shortest_delta(to_x - from_x, (int64_t)map->width);
    int64_t dy = shortest_delta(to_y - from_y, (int64_t)map->height);
    int64_t rx, ry;
    double angle;

    if (dx == 0 && dy == 0)
        return 0;
    rotate_to_frame(from_orientation, dx, dy, &rx, &ry);
    angle = atan2((double)rx, (double)-ry) * (180.0 / M_PI);
    if (angle < 0)
        angle += 360.0;
    return (int)((angle + 22.5) / 45.0) % 8 + 1;
}
