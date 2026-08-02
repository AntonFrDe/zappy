/*
** Unit tests for src/server/map.c (grid + torus wrapping)
** Framework: Criterion
*/

#include <criterion/criterion.h>
#include <server/server.h>

static uint32_t total_of(map_t *map, resource_t r)
{
    uint32_t total = 0;

    for (uint64_t i = 0; i < map->width * map->height; i++)
        total += map->tiles[i].quantities[r];
    return total;
}

Test(map, create_and_destroy)
{
    map_t *map = map_create(10, 10);

    cr_assert_not_null(map);
    cr_assert_eq(map->width, (uint64_t)10);
    cr_assert_eq(map->height, (uint64_t)10);
    cr_assert_not_null(map->tiles);
    map_destroy(map);
}

Test(map, tiles_start_at_zero)
{
    map_t *map = map_create(5, 5);
    tile_t *tile = map_get_tile(map, 2, 3);

    for (int i = 0; i < RESOURCE_COUNT; i++)
        cr_assert_eq(tile->quantities[i], (uint16_t)0);
    map_destroy(map);
}

Test(map, get_tile_in_bounds_matches_expected_cell)
{
    map_t *map = map_create(10, 10);
    tile_t *tile = map_get_tile(map, 3, 4);

    tile->quantities[RESOURCE_FOOD] = 7;
    cr_assert_eq(map->tiles[4 * 10 + 3].quantities[RESOURCE_FOOD], (uint16_t)7);
    map_destroy(map);
}

Test(map, wraps_on_right_edge)
{
    map_t *map = map_create(10, 10);
    tile_t *left_edge = map_get_tile(map, 0, 5);
    tile_t *wrapped = map_get_tile(map, 10, 5);

    left_edge->quantities[RESOURCE_LINEMATE] = 3;
    cr_assert_eq(wrapped->quantities[RESOURCE_LINEMATE], (uint16_t)3);
    map_destroy(map);
}

Test(map, wraps_on_negative_x)
{
    map_t *map = map_create(10, 10);
    tile_t *last_col = map_get_tile(map, 9, 5);
    tile_t *negative = map_get_tile(map, -1, 5);

    last_col->quantities[RESOURCE_SIBUR] = 5;
    cr_assert_eq(negative->quantities[RESOURCE_SIBUR], (uint16_t)5);
    map_destroy(map);
}

Test(map, wraps_on_negative_y)
{
    map_t *map = map_create(8, 8);
    tile_t *last_row = map_get_tile(map, 2, 7);
    tile_t *negative = map_get_tile(map, 2, -1);

    last_row->quantities[RESOURCE_THYSTAME] = 1;
    cr_assert_eq(negative->quantities[RESOURCE_THYSTAME], (uint16_t)1);
    map_destroy(map);
}

Test(map, wraps_far_beyond_one_full_loop)
{
    map_t *map = map_create(10, 10);
    tile_t *origin = map_get_tile(map, 0, 0);
    tile_t *two_loops = map_get_tile(map, 20, 30);

    origin->quantities[RESOURCE_MENDIANE] = 9;
    cr_assert_eq(two_loops->quantities[RESOURCE_MENDIANE], (uint16_t)9);
    map_destroy(map);
}

Test(map, move_north_decreases_y)
{
    map_t *map = map_create(10, 10);
    int64_t x = 5, y = 5;

    map_move(map, &x, &y, DIR_NORTH);
    cr_assert_eq(x, (int64_t)5);
    cr_assert_eq(y, (int64_t)4);
    map_destroy(map);
}

Test(map, move_south_increases_y)
{
    map_t *map = map_create(10, 10);
    int64_t x = 5, y = 5;

    map_move(map, &x, &y, DIR_SOUTH);
    cr_assert_eq(x, (int64_t)5);
    cr_assert_eq(y, (int64_t)6);
    map_destroy(map);
}

Test(map, move_east_increases_x)
{
    map_t *map = map_create(10, 10);
    int64_t x = 5, y = 5;

    map_move(map, &x, &y, DIR_EAST);
    cr_assert_eq(x, (int64_t)6);
    cr_assert_eq(y, (int64_t)5);
    map_destroy(map);
}

Test(map, move_west_decreases_x)
{
    map_t *map = map_create(10, 10);
    int64_t x = 5, y = 5;

    map_move(map, &x, &y, DIR_WEST);
    cr_assert_eq(x, (int64_t)4);
    cr_assert_eq(y, (int64_t)5);
    map_destroy(map);
}

Test(map, move_north_wraps_from_top_row)
{
    map_t *map = map_create(10, 10);
    int64_t x = 3, y = 0;

    map_move(map, &x, &y, DIR_NORTH);
    cr_assert_eq(x, (int64_t)3);
    cr_assert_eq(y, (int64_t)9);
    map_destroy(map);
}

Test(map, move_east_wraps_from_right_column)
{
    map_t *map = map_create(10, 10);
    int64_t x = 9, y = 4;

    map_move(map, &x, &y, DIR_EAST);
    cr_assert_eq(x, (int64_t)0);
    cr_assert_eq(y, (int64_t)4);
    map_destroy(map);
}

// exemple exact du sujet: carte 10x10 -> 50 food, 5 thystame etc.
Test(map, spawn_matches_subject_example_10x10)
{
    map_t *map = map_create(10, 10);

    map_spawn_resources(map, NULL, 0);
    cr_assert_eq(total_of(map, RESOURCE_FOOD), (uint32_t)50);
    cr_assert_eq(total_of(map, RESOURCE_LINEMATE), (uint32_t)30);
    cr_assert_eq(total_of(map, RESOURCE_DERAUMERE), (uint32_t)15);
    cr_assert_eq(total_of(map, RESOURCE_SIBUR), (uint32_t)10);
    cr_assert_eq(total_of(map, RESOURCE_MENDIANE), (uint32_t)10);
    cr_assert_eq(total_of(map, RESOURCE_PHIRAS), (uint32_t)8);
    cr_assert_eq(total_of(map, RESOURCE_THYSTAME), (uint32_t)5);
    map_destroy(map);
}

// sur une petite carte, la densite peut arrondir a 0 mais le sujet
// garantit au moins une ressource de chaque sur la carte
Test(map, spawn_guarantees_at_least_one_of_each_on_tiny_map)
{
    map_t *map = map_create(2, 2);

    map_spawn_resources(map, NULL, 0);
    for (int r = 0; r < RESOURCE_COUNT; r++)
        cr_assert_geq(total_of(map, r), (uint32_t)1);
    map_destroy(map);
}

// remplit la map pile a la densite cible, comme apres un premier spawn
static void fill_to_target(map_t *map)
{
    map_spawn_resources(map, NULL, 0);
}

// enleve nb unites de r la ou il y en a vraiment (les positions sont
// random, on peut pas viser une case en particulier)
static void remove_resource(map_t *map, resource_t r, int nb)
{
    uint64_t tiles = map->width * map->height;

    for (uint64_t i = 0; i < tiles && nb > 0; i++)
        while (map->tiles[i].quantities[r] > 0 && nb > 0) {
            map->tiles[i].quantities[r]--;
            nb--;
        }
    cr_assert_eq(nb, 0, "pas assez de ressource %d a enlever", (int)r);
}

// rien ne manque -> aucune case touchee, donc aucun bct a pousser.
// c'est le test central de #24: pas de blast de toute la map
Test(map, spawn_reports_no_change_when_nothing_is_missing)
{
    map_t *map = map_create(5, 5);
    tile_pos_t changed[64];

    fill_to_target(map);
    cr_assert_eq(map_spawn_resources(map, changed, 64), 0);
    map_destroy(map);
}

// une seule ressource retiree -> exactement une case touchee
Test(map, spawn_reports_one_tile_when_one_resource_is_missing)
{
    map_t *map = map_create(5, 5);
    tile_pos_t changed[64];

    fill_to_target(map);
    remove_resource(map, RESOURCE_FOOD, 1);
    cr_assert_eq(map_spawn_resources(map, changed, 64), 1);
    map_destroy(map);
}

// toutes les cases reportees doivent vraiment avoir bouge. le snapshot est
// pris apres le retrait: spawn ne fait qu'incrementer, donc une case
// touchee differe forcement de l'etat au moment de l'appel
Test(map, spawn_only_reports_tiles_that_really_changed)
{
    map_t *map = map_create(6, 6);
    tile_pos_t changed[512];
    tile_t before[36];
    int count;
    int idx;

    fill_to_target(map);
    remove_resource(map, RESOURCE_FOOD, 3);
    remove_resource(map, RESOURCE_SIBUR, 2);
    for (int i = 0; i < 36; i++)
        before[i] = map->tiles[i];
    count = map_spawn_resources(map, changed, 512);
    cr_assert_gt(count, 0);
    cr_assert_leq(count, 5);
    for (int i = 0; i < count; i++) {
        idx = (int)(changed[i].y * 6 + changed[i].x);
        cr_assert_neq(memcmp(&before[idx], &map->tiles[idx], sizeof(tile_t)),
            0, "case %d %d reportee mais identique", (int)changed[i].x,
            (int)changed[i].y);
    }
    map_destroy(map);
}

// carte 1x1: les 7 ressources tombent forcement sur la mm case, il ne
// doit y avoir qu'une seule entree
Test(map, spawn_does_not_report_the_same_tile_twice)
{
    map_t *map = map_create(1, 1);
    tile_pos_t changed[16];

    cr_assert_eq(map_spawn_resources(map, changed, 16), 1);
    cr_assert_eq(changed[0].x, (int64_t)0);
    cr_assert_eq(changed[0].y, (int64_t)0);
    map_destroy(map);
}

// liste trop petite -> -1, l'appelant devra pousser toute la map
Test(map, spawn_returns_minus_one_when_list_overflows)
{
    map_t *map = map_create(10, 10);
    tile_pos_t changed[2];

    cr_assert_eq(map_spawn_resources(map, changed, 2), -1);
    map_destroy(map);
}

// sans liste -> -1, mais la map est remplie normalement
Test(map, spawn_without_list_still_fills_the_map)
{
    map_t *map = map_create(10, 10);

    cr_assert_eq(map_spawn_resources(map, NULL, 0), -1);
    cr_assert_eq(total_of(map, RESOURCE_FOOD), (uint32_t)50);
    cr_assert_eq(total_of(map, RESOURCE_THYSTAME), (uint32_t)5);
    map_destroy(map);
}

// map_create est publique: le parser protege le binaire, pas l'API
Test(map, create_rejects_dimensions_above_max)
{
    cr_assert_null(map_create(MAP_MAX_WIDTH + 1, 10));
    cr_assert_null(map_create(10, MAP_MAX_HEIGHT + 1));
}

// largeur 0 -> calloc(0) reussit, puis secure_mod divise par zero
Test(map, create_rejects_zero_dimensions)
{
    cr_assert_null(map_create(0, 10));
    cr_assert_null(map_create(10, 0));
}

Test(map, create_accepts_the_max_itself)
{
    map_t *map = map_create(MAP_MAX_WIDTH, 1);

    cr_assert_not_null(map);
    cr_assert_eq(map->width, (uint64_t)MAP_MAX_WIDTH);
    map_destroy(map);
}
