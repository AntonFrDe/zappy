/*
** Unit tests for the targeted bct push (#24) : le respawn periodique ne
** doit pousser que les cases qui ont vraiment bouge, jamais toute la map.
** Pas de socket ni de thread ici, server_t monte a la main et
** scheduler_tick() appele direct, on relit client->send_buf.
** Framework: Criterion.
*/

#include <criterion/criterion.h>
#include <server/server.h>
#include <stdlib.h>
#include <string.h>

static server_t *make_server(uint64_t width, uint64_t height)
{
    server_t *server = malloc(sizeof(server_t));
    flags_t *cfg = malloc(sizeof(flags_t));
    char **teams = malloc(sizeof(char *));

    memset(server, 0, sizeof(*server));
    teams[0] = "team1";
    *cfg = (flags_t){
        .port = 0, .width = width, .height = height, .teams = teams,
        .teams_nb = 1, .clients_nb = 3, .freq = 100
    };
    server->cfg = cfg;
    server->map = map_create(width, height);
    cr_assert_not_null(server->map);
    // disconnect_client rend le slot depuis #162: sans ce tableau, toute
    // deconnexion dereference NULL
    server->slots_remaining = malloc(sizeof(uint16_t) * cfg->teams_nb);
    cr_assert_not_null(server->slots_remaining);
    server->slots_remaining[0] = cfg->clients_nb;
    server->listen_fd = -1;
    server->nfds = 1;
    server->next_client_id = 1;
    return server;
}

static void destroy_server(server_t *server)
{
    for (int i = 1; i < server->nfds; i++)
        client_destroy(server->clients[i]);
    map_destroy(server->map);
    free(server->slots_remaining);
    free(server->cfg->teams);
    free(server->cfg);
    free(server);
}

// fd = -1 : client_destroy call pas close(), et rien part sur le reseau
static client_t *add_gui(server_t *server)
{
    client_t *gui = client_create(-1);

    cr_assert_not_null(gui);
    gui->id = server->next_client_id++;
    gui->is_graphic = true;
    gui->handshake_done = true;
    server->clients[server->nfds] = gui;
    server->fds[server->nfds].fd = -1;
    server->nfds++;
    return gui;
}

static const char *sent(client_t *client)
{
    return client->send_buf != NULL ? client->send_buf : "";
}

static void clear_sent(client_t *client)
{
    free(client->send_buf);
    client->send_buf = NULL;
    client->send_len = 0;
    client->send_sent = 0;
}

static int count_bct(client_t *gui)
{
    const char *s = sent(gui);
    int n = 0;

    for (const char *p = strstr(s, "bct "); p != NULL;
        p = strstr(p + 1, "bct "))
        n++;
    return n;
}

// enleve nb unites de r la ou il y en a vraiment
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

// force le respawn au prochain tick (epoch est toujours dans le passe)
static void make_respawn_due(server_t *server)
{
    server->next_resource_spawn = (struct timespec){0, 0};
}

// le test central de #24 : rien ne manque -> aucun bct, surtout pas W*H
Test(gui_push, respawn_pushes_nothing_when_map_is_already_full)
{
    server_t *server = make_server(5, 5);
    client_t *gui = add_gui(server);

    map_spawn_resources(server->map, NULL, 0);
    clear_sent(gui);
    make_respawn_due(server);
    scheduler_tick(server);
    cr_assert_str_eq(sent(gui), "");
    destroy_server(server);
}

Test(gui_push, respawn_pushes_one_bct_per_changed_tile)
{
    server_t *server = make_server(6, 6);
    client_t *gui = add_gui(server);

    map_spawn_resources(server->map, NULL, 0);
    remove_resource(server->map, RESOURCE_FOOD, 1);
    clear_sent(gui);
    make_respawn_due(server);
    scheduler_tick(server);
    cr_assert_eq(count_bct(gui), 1);
    destroy_server(server);
}

// 3 unites retirees -> au plus 3 cases touchees, jamais les 36
Test(gui_push, respawn_never_pushes_the_whole_map_for_a_few_changes)
{
    server_t *server = make_server(6, 6);
    client_t *gui = add_gui(server);
    int n;

    map_spawn_resources(server->map, NULL, 0);
    remove_resource(server->map, RESOURCE_FOOD, 2);
    remove_resource(server->map, RESOURCE_SIBUR, 1);
    clear_sent(gui);
    make_respawn_due(server);
    scheduler_tick(server);
    n = count_bct(gui);
    cr_assert_gt(n, 0);
    cr_assert_leq(n, 3);
    destroy_server(server);
}

// le bct pousse doit refleter le contenu reel de la case
Test(gui_push, pushed_bct_matches_the_tile_content)
{
    server_t *server = make_server(4, 4);
    client_t *gui = add_gui(server);
    char expected[128];
    int64_t x;
    int64_t y;
    tile_t *tile;

    map_spawn_resources(server->map, NULL, 0);
    remove_resource(server->map, RESOURCE_FOOD, 1);
    clear_sent(gui);
    make_respawn_due(server);
    scheduler_tick(server);
    cr_assert_eq(sscanf(sent(gui), "bct %ld %ld", &x, &y), 2);
    tile = map_get_tile(server->map, x, y);
    snprintf(expected, sizeof(expected), "bct %ld %ld %u %u %u %u %u %u %u\n",
        x, y, tile->quantities[0], tile->quantities[1], tile->quantities[2],
        tile->quantities[3], tile->quantities[4], tile->quantities[5],
        tile->quantities[6]);
    cr_assert_str_eq(sent(gui), expected);
    destroy_server(server);
}

static client_t *add_player(server_t *server, int64_t x, int64_t y)
{
    client_t *client = client_create(-1);

    cr_assert_not_null(client);
    client->id = server->next_client_id++;
    client->handshake_done = true;
    client->team_index = 0;
    client->player = player_create_at(x, y);
    cr_assert_not_null(client->player);
    client->player->level = 1;
    // player_create_at laisse next_food_tick non initialise (handle_team_name
    // le pose en prod). sans ca scheduler_tick declenche une faim au hasard
    client->player->next_food_tick = compute_ready_at(126, server->cfg->freq);
    server->clients[server->nfds] = client;
    server->fds[server->nfds].fd = -1;
    server->nfds++;
    return client;
}

// 1->2 demande 1 joueur et 1 linemate (cf sujet)
Test(gui_push, incantation_pushes_bct_without_the_consumed_stones)
{
    server_t *server = make_server(4, 4);
    client_t *gui = add_gui(server);
    client_t *player = add_player(server, 2, 3);
    tile_t *tile = map_get_tile(server->map, 2, 3);

    tile->quantities[RESOURCE_LINEMATE] = 1;
    tile->quantities[RESOURCE_FOOD] = 4;
    clear_sent(gui);
    incantation_freeze_others(server, player);
    command_incantation(server, player, "");
    cr_assert_eq(player->player->level, 2);
    cr_assert_eq(tile->quantities[RESOURCE_LINEMATE], (uint16_t)0);
    cr_assert_not_null(strstr(sent(gui), "bct 2 3 4 0 0 0 0 0 0\n"));
    destroy_server(server);
}

// echec -> rien n'est consomme donc rien a pousser
Test(gui_push, failed_incantation_pushes_no_bct)
{
    server_t *server = make_server(4, 4);
    client_t *gui = add_gui(server);
    client_t *player = add_player(server, 1, 1);

    map_get_tile(server->map, 1, 1)->quantities[RESOURCE_LINEMATE] = 0;
    clear_sent(gui);
    command_incantation(server, player, "");
    cr_assert_eq(player->player->level, 1);
    cr_assert_eq(count_bct(gui), 0);
    destroy_server(server);
}

// le respawn ne doit pas se redeclencher tant que 20 u.t. sont pas passees
Test(gui_push, respawn_does_not_fire_twice_in_a_row)
{
    server_t *server = make_server(5, 5);
    client_t *gui = add_gui(server);

    map_spawn_resources(server->map, NULL, 0);
    remove_resource(server->map, RESOURCE_FOOD, 1);
    make_respawn_due(server);
    scheduler_tick(server);
    clear_sent(gui);
    scheduler_tick(server);
    cr_assert_str_eq(sent(gui), "");
    destroy_server(server);
}

// force l'echeance de faim dans le passe
static void make_food_tick_due(client_t *client)
{
    client->player->next_food_tick = (struct timespec){0, 0};
}

// absorbe le respawn du premier tick, sinon des bct polluent la mesure
static void settle(server_t *server, client_t *gui)
{
    map_spawn_resources(server->map, NULL, 0);
    make_respawn_due(server);
    scheduler_tick(server);
    clear_sent(gui);
}

// la faim baisse en continu: sans pin l'inventaire affiche par la GUI
// derive toute la partie
Test(gui_push, food_tick_pushes_pin_with_the_new_count)
{
    server_t *server = make_server(5, 5);
    client_t *gui = add_gui(server);
    client_t *player = add_player(server, 2, 3);
    char expected[128];

    settle(server, gui);
    make_food_tick_due(player);
    scheduler_tick(server);

    snprintf(expected, sizeof(expected), "pin #%d 2 3 9 0 0 0 0 0 0\n",
        player->id);
    cr_assert_not_null(strstr(sent(gui), expected), "recu: %s", sent(gui));
    cr_assert_eq(player->player->inventory[RESOURCE_FOOD], (uint16_t)9);
    destroy_server(server);
}

// le joueur meurt: disconnect_client emet deja pdi, un pin ici decrirait
// l'inventaire d'un client qui n'existe plus
Test(gui_push, starvation_death_pushes_pdi_but_no_pin)
{
    server_t *server = make_server(5, 5);
    client_t *gui = add_gui(server);
    client_t *player = add_player(server, 1, 1);

    settle(server, gui);
    player->player->inventory[RESOURCE_FOOD] = 0;
    make_food_tick_due(player);
    scheduler_tick(server);

    cr_assert_not_null(strstr(sent(gui), "pdi "), "recu: %s", sent(gui));
    cr_assert_null(strstr(sent(gui), "pin "),
        "pin envoye pour un joueur mort: %s", sent(gui));
    destroy_server(server);
}

// un tick de faim ne doit pas relancer un pin tant que 126 u.t. ne sont
// pas repassees
Test(gui_push, no_pin_before_the_next_food_tick)
{
    server_t *server = make_server(5, 5);
    client_t *gui = add_gui(server);
    client_t *player = add_player(server, 0, 0);

    settle(server, gui);
    make_food_tick_due(player);
    scheduler_tick(server);
    clear_sent(gui);

    scheduler_tick(server);
    cr_assert_str_eq(sent(gui), "");
    cr_assert_eq(player->player->inventory[RESOURCE_FOOD], (uint16_t)9);
    destroy_server(server);
}
