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

void egg_list_add(egg_list_t *list, int team_index, int64_t x, int64_t y)
{
    egg_t *bigger;

    if (list->count >= list->capacity) {
        list->capacity = list->capacity == 0 ? 4 : list->capacity * 2;
        bigger = realloc(list->eggs, sizeof(egg_t) * (size_t)list->capacity);
        if (bigger == NULL)
            return;
        list->eggs = bigger;
    }
    list->eggs[list->count].team_index = team_index;
    list->eggs[list->count].x = x;
    list->eggs[list->count].y = y;
    list->count++;
}

bool egg_list_pop_random(egg_list_t *list, int team_index,
    int64_t *x, int64_t *y)
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
    list->eggs[chosen] = list->eggs[list->count - 1];
    list->count--;
    return true;
}

void egg_list_destroy_at(egg_list_t *list, int64_t x, int64_t y)
{
    for (int i = 0; i < list->count; i++) {
        if (list->eggs[i].x == x && list->eggs[i].y == y) {
            list->eggs[i] = list->eggs[list->count - 1];
            list->count--;
            i--;
        }
    }
}
