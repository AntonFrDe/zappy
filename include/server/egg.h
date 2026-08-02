#ifndef EGG_H_
    #define EGG_H_
    #include <stdint.h>
    #include <stdbool.h>

    #define INITIAL_EGG_PARENT (-1)

    typedef struct {
        int id;
        int team_index;
        int parent_id;
        int64_t x;
        int64_t y;
    } egg_t;

    typedef struct {
        egg_t *eggs;
        int count;
        int capacity;
    } egg_list_t;

    void egg_list_init(egg_list_t *list);
    
    void egg_list_destroy(egg_list_t *list);
    
    int egg_list_add(egg_list_t *list, int *next_id, int team_index,
        int parent_id, int64_t x, int64_t y);
    
    bool egg_list_pop_random(egg_list_t *list, int team_index,
        int64_t *x, int64_t *y, int *id);
    
    int egg_list_destroy_at(egg_list_t *list, int64_t x, int64_t y,
        int *destroyed_ids, int max_ids);
    
    
#endif /* !EGG_H_ */
