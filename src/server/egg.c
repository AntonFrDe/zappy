#include "../../include/server/egg.h"
#include <stdlib.h>

void egg_list_init(egg_list_t *list)
{
    list->eggs = NULL;
    list->count = 0;
    list->capacity = 0;
}

void egg_list_destroy(egg_list_t *list)
{
    free(list->eggs);
    list->eggs = NULL;
    list->count = 0;
    list->capacity = 0;
}

int egg_list_add(egg_list_t *list, int *next_id, int team_index,
    int parent_id, int64_t x, int64_t y)
{
    egg_t *bigger;
    int id = (*next_id)++;

    if (list->count >= list->capacity) {
        list->capacity = list->capacity == 0 ? 4 : list->capacity * 2;
        bigger = realloc(list->eggs, sizeof(egg_t) * (size_t)list->capacity);
        if (bigger == NULL)
            return id;
        list->eggs = bigger;
    }
    list->eggs[list->count].id = id;
    list->eggs[list->count].team_index = team_index;
    list->eggs[list->count].parent_id = parent_id;
    list->eggs[list->count].x = x;
    list->eggs[list->count].y = y;
    list->count++;
    return id;
}

bool egg_list_pop_random(egg_list_t *list, int team_index,
    int64_t *x, int64_t *y, int *id)
{
    int match_count = 0;
    int chosen = -1;

    for (int i = 0; i < list->count; i++) {
        if (list->eggs[i].team_index != team_index)
            continue;
        match_count++;
        if (rand() % match_count == 0)
            chosen = i;
    }
    if (chosen < 0)
        return false;
    *x = list->eggs[chosen].x;
    *y = list->eggs[chosen].y;
    *id = list->eggs[chosen].id;
    list->eggs[chosen] = list->eggs[list->count - 1];
    list->count--;
    return true;
}

int egg_list_destroy_at(egg_list_t *list, int64_t x, int64_t y,
    int *destroyed_ids, int max_ids)
{
    int destroyed = 0;

    for (int i = 0; i < list->count; i++) {
        if (list->eggs[i].x == x && list->eggs[i].y == y) {
            if (destroyed < max_ids)
                destroyed_ids[destroyed] = list->eggs[i].id;
            destroyed++;
            list->eggs[i] = list->eggs[list->count - 1];
            list->count--;
            i--;
        }
    }
    return destroyed;
}
