#ifndef EGG_H_
    #define EGG_H_
    #include <stdint.h>
    #include <stdbool.h>

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
// ajoute un oeuf, lui assigne un id via *next_id (incremente au passage),
// et le retourne
    int egg_list_add(egg_list_t *list, int *next_id, int team_index,
        int parent_id, int64_t x, int64_t y);
// choisit un oeuf au hasard parmi ceux de team_index, le retire de la
// liste et donne sa position + son id; false si aucun oeuf pour cette equipe
    bool egg_list_pop_random(egg_list_t *list, int team_index,
        int64_t *x, int64_t *y, int *id);
// detruit tous les oeufs a cette position (ex: Eject), remplit
// destroyed_ids (jusqu'a max_ids) et retourne combien ont ete detruits
    int egg_list_destroy_at(egg_list_t *list, int64_t x, int64_t y,
        int *destroyed_ids, int max_ids);

#endif /* !EGG_H_ */
