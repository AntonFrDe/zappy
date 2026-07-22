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

// aligne sur le protocole gui (orientation O: 1=N 2=E 3=S 4=W)
    typedef enum {
        DIR_NORTH = 1,
        DIR_EAST = 2,
        DIR_SOUTH = 3,
        DIR_WEST = 4
    } direction_t;

    map_t *map_create(uint64_t width, uint64_t height);
    void map_destroy(map_t *map);
    // x/y wrappes torique (accepte negatif et >= width/height)
    tile_t *map_get_tile(map_t *map, int64_t x, int64_t y);
    // avance x/y d'une case dans dir, wrap torique inclus
    void map_move(map_t *map, int64_t *x, int64_t *y, direction_t dir);
    // place les ressources initiales (densite * width * height, min 1 de chaque)
    void map_spawn_resources(map_t *map);
    const char *resource_name(resource_t r);
    // -1 si nom inconnu
    int resource_from_name(const char *name);
    // direction (0-8) de to_x/to_y vue depuis from_x/from_y + son orientation
    // 0 = meme case, 1 = devant, sens horaire, chemin le plus court (torique)
    int map_direction(map_t *map, int64_t from_x, int64_t from_y,
        direction_t from_orientation, int64_t to_x, int64_t to_y);

// TODO: respawn periodique toutes les 20 unites de temps
// -> a faire avec le scheduler (pas encore code)

#endif /* !MAP_H_ */
