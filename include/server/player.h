#ifndef PLAYER_H_
    #define PLAYER_H_
    #include "map.h"
    #include <time.h>

    typedef struct {
        int64_t x;
        int64_t y;
        direction_t orientation;
        int level;
        uint16_t inventory[RESOURCE_COUNT];
        struct timespec next_food_tick; // prochaine decrementation de food
    } player_t;

    player_t *player_create(map_t *map);
    void player_destroy(player_t *player);

#endif /* !PLAYER_H_ */
