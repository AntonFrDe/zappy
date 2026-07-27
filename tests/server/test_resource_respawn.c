/*
** Unit tests for periodic resource respawn (src/server/command.c
** spawn_resources_if_ready, called from scheduler_tick).
** Subject: "The server spawns resources upon starting and every 20 time
** units." No socket/thread needed here: scheduler_tick only touches
** server->map and server->next_resource_spawn when server->nfds == 1
** (no clients connected), so we can drive it directly.
*/

#include <criterion/criterion.h>
#include <server/server.h>
#include <unistd.h>

static flags_t *alloc_single_team_cfg(int port, uint64_t width,
    uint64_t height, uint16_t clients_nb)
{
    char **teams = malloc(sizeof(char *));
    flags_t *cfg = malloc(sizeof(flags_t));

    teams[0] = "team1";
    *cfg = (flags_t){
        .port = port, .width = width, .height = height,
        .teams = teams, .teams_nb = 1, .clients_nb = clients_nb, .freq = 2000
    };
    return cfg;
}

static uint32_t sum_resource(map_t *map, resource_t r)
{
    uint32_t total = 0;

    for (uint64_t i = 0; i < map->width * map->height; i++)
        total += map->tiles[i].quantities[r];
    return total;
}

Test(resource_respawn, does_not_respawn_before_twenty_time_units_elapsed)
{
    flags_t *cfg = alloc_single_team_cfg(5901, 10, 10, 1);
    server_t server;
    uint32_t before;

    cr_assert_eq(server_init(&server, cfg), 0);
    before = sum_resource(server.map, RESOURCE_THYSTAME);

    scheduler_tick(&server);

    cr_assert_eq(sum_resource(server.map, RESOURCE_THYSTAME), before);
}

Test(resource_respawn, respawns_again_once_twenty_time_units_have_elapsed)
{
    flags_t *cfg = alloc_single_team_cfg(5902, 10, 10, 1);
    server_t server;
    uint32_t before, after;

    cr_assert_eq(server_init(&server, cfg), 0);
    before = sum_resource(server.map, RESOURCE_THYSTAME);
    cr_assert_gt(before, (uint32_t)0);

    usleep(15000);
    scheduler_tick(&server);
    after = sum_resource(server.map, RESOURCE_THYSTAME);

    cr_assert_eq(after, before * 2);
}
