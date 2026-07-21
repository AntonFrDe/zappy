/*
** Unit tests for src/server/player.c (player_t creation)
** Framework: Criterion
*/

#include <criterion/criterion.h>
#include <server/server.h>

Test(player, create_returns_valid_player)
{
    map_t *map = map_create(10, 10);
    player_t *player = player_create(map);

    cr_assert_not_null(player);
    cr_assert_geq(player->x, (int64_t)0);
    cr_assert_lt(player->x, (int64_t)10);
    cr_assert_geq(player->y, (int64_t)0);
    cr_assert_lt(player->y, (int64_t)10);
    cr_assert_geq((int)player->orientation, 1);
    cr_assert_leq((int)player->orientation, 4);
    cr_assert_eq(player->level, 1);
    player_destroy(player);
    map_destroy(map);
}

Test(player, starts_with_ten_food_and_nothing_else)
{
    map_t *map = map_create(10, 10);
    player_t *player = player_create(map);

    cr_assert_eq(player->inventory[RESOURCE_FOOD], (uint16_t)10);
    cr_assert_eq(player->inventory[RESOURCE_LINEMATE], (uint16_t)0);
    cr_assert_eq(player->inventory[RESOURCE_DERAUMERE], (uint16_t)0);
    cr_assert_eq(player->inventory[RESOURCE_SIBUR], (uint16_t)0);
    cr_assert_eq(player->inventory[RESOURCE_MENDIANE], (uint16_t)0);
    cr_assert_eq(player->inventory[RESOURCE_PHIRAS], (uint16_t)0);
    cr_assert_eq(player->inventory[RESOURCE_THYSTAME], (uint16_t)0);
    player_destroy(player);
    map_destroy(map);
}

Test(player, destroy_null_is_safe)
{
    player_destroy(NULL);
}
