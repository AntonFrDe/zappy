#ifndef MAP_H_
    #define MAP_H_
    #include <stdint.h>

// ordre aligne sur le protocole gui (bct X Y q0 q1 q2 q3 q4 q5 q6)
    typedef enum {
        RESOURCE_FOOD = 0,
        RESOURCE_LINEMATE,
        RESOURCE_DERAUMERE,
        RESOURCE_SIBUR,
        RESOURCE_MENDIANE,
        RESOURCE_PHIRAS,
        RESOURCE_THYSTAME,
        RESOURCE_COUNT
    } resource_t;

    typedef struct {
        uint16_t quantities[RESOURCE_COUNT];
    } tile_t;

    typedef struct {
        uint64_t width;
        uint64_t height;
        tile_t *tiles; // row-major, tiles[y * width + x]
    } map_t;

    map_t *map_create(uint64_t width, uint64_t height);
    void map_destroy(map_t *map);
    // x/y wrappes torique (accepte negatif et >= width/height)
    tile_t *map_get_tile(map_t *map, int64_t x, int64_t y);

// TODO: spawn des ressources (densite * width * height, respawn 20 u.t.)
// -> prochaine tache, pas fait ici volontairement

#endif /* !MAP_H_ */
