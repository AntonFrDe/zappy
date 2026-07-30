/*
** Unit tests for GUI request reception (src/server/gui_requests.c).
** Pas de socket ni de thread serveur ici : server_t monte a la main,
** gui_dispatch_request() appele direct, on relit client->send_buf.
** Du coup pas de collision de port et resultat deterministe (map a zero,
** on fixe nous mm les quantites + l'orientation qui est random).
** Framework: Criterion.
*/

#include <criterion/criterion.h>
#include <server/server.h>
#include <server/gui_requests.h>
#include <stdlib.h>
#include <string.h>

static server_t *make_server(uint64_t width, uint64_t height)
{
    server_t *server = malloc(sizeof(server_t));
    flags_t *cfg = malloc(sizeof(flags_t));
    char **teams = malloc(sizeof(char *) * 2);

    memset(server, 0, sizeof(*server));
    teams[0] = "team1";
    teams[1] = "team2";
    *cfg = (flags_t){
        .port = 0, .width = width, .height = height, .teams = teams,
        .teams_nb = 2, .clients_nb = 3, .freq = 100
    };
    server->cfg = cfg;
    server->map = map_create(width, height);
    cr_assert_not_null(server->map);
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
    free(server->cfg->teams);
    free(server->cfg);
    free(server);
}

// fd = -1 : client_destroy call pas close(), et rien part sur le reseau
static client_t *attach(server_t *server, bool is_graphic)
{
    client_t *client = client_create(-1);

    cr_assert_not_null(client);
    client->id = server->next_client_id++;
    client->is_graphic = is_graphic;
    client->handshake_done = true;
    server->clients[server->nfds] = client;
    server->fds[server->nfds].fd = -1;
    server->nfds++;
    return client;
}

static client_t *add_gui(server_t *server)
{
    return attach(server, true);
}

static client_t *add_player(server_t *server, int64_t x, int64_t y,
    int level, direction_t orientation)
{
    client_t *client = attach(server, false);

    client->team_index = 0;
    client->player = player_create_at(x, y);
    cr_assert_not_null(client->player);
    client->player->level = level;
    client->player->orientation = orientation;
    return client;
}

// client_queue_send met toujours le '\0', on lit direct
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

/* ----------------------------------------------------------------- msz */

Test(gui_requests, msz_returns_map_size)
{
    server_t *server = make_server(12, 7);
    client_t *gui = add_gui(server);

    gui_dispatch_request(server, gui, "msz");
    cr_assert_str_eq(sent(gui), "msz 12 7\n");
    destroy_server(server);
}

/* ----------------------------------------------------------------- sgt */

Test(gui_requests, sgt_returns_current_frequency)
{
    server_t *server = make_server(5, 5);
    client_t *gui = add_gui(server);

    gui_dispatch_request(server, gui, "sgt");
    cr_assert_str_eq(sent(gui), "sgt 100\n");
    destroy_server(server);
}

/* ----------------------------------------------------------------- tna */

Test(gui_requests, tna_returns_one_line_per_team)
{
    server_t *server = make_server(5, 5);
    client_t *gui = add_gui(server);

    gui_dispatch_request(server, gui, "tna");
    cr_assert_str_eq(sent(gui), "tna team1\ntna team2\n");
    destroy_server(server);
}

/* ----------------------------------------------------------------- bct */

Test(gui_requests, bct_returns_tile_content)
{
    server_t *server = make_server(5, 5);
    client_t *gui = add_gui(server);

    gui_dispatch_request(server, gui, "bct 3 4");
    cr_assert_str_eq(sent(gui), "bct 3 4 0 0 0 0 0 0 0\n");
    destroy_server(server);
}

Test(gui_requests, bct_reports_actual_quantities)
{
    server_t *server = make_server(5, 5);
    client_t *gui = add_gui(server);
    tile_t *tile = map_get_tile(server->map, 1, 2);

    tile->quantities[RESOURCE_FOOD] = 9;
    tile->quantities[RESOURCE_THYSTAME] = 4;
    gui_dispatch_request(server, gui, "bct 1 2");
    cr_assert_str_eq(sent(gui), "bct 1 2 9 0 0 0 0 0 4\n");
    destroy_server(server);
}

// carte torique mais le protocole veut sbp, pas un modulo
Test(gui_requests, bct_out_of_range_returns_sbp)
{
    server_t *server = make_server(5, 5);
    client_t *gui = add_gui(server);

    gui_dispatch_request(server, gui, "bct 5 0");
    cr_assert_str_eq(sent(gui), "sbp\n");
    clear_sent(gui);
    gui_dispatch_request(server, gui, "bct 0 5");
    cr_assert_str_eq(sent(gui), "sbp\n");
    destroy_server(server);
}

Test(gui_requests, bct_negative_coords_return_sbp)
{
    server_t *server = make_server(5, 5);
    client_t *gui = add_gui(server);

    gui_dispatch_request(server, gui, "bct -1 2");
    cr_assert_str_eq(sent(gui), "sbp\n");
    destroy_server(server);
}

Test(gui_requests, bct_missing_second_coord_returns_sbp)
{
    server_t *server = make_server(5, 5);
    client_t *gui = add_gui(server);

    gui_dispatch_request(server, gui, "bct 2");
    cr_assert_str_eq(sent(gui), "sbp\n");
    destroy_server(server);
}

Test(gui_requests, bct_without_any_arg_returns_sbp)
{
    server_t *server = make_server(5, 5);
    client_t *gui = add_gui(server);

    gui_dispatch_request(server, gui, "bct");
    cr_assert_str_eq(sent(gui), "sbp\n");
    destroy_server(server);
}

Test(gui_requests, bct_garbage_returns_sbp)
{
    server_t *server = make_server(5, 5);
    client_t *gui = add_gui(server);

    gui_dispatch_request(server, gui, "bct a b");
    cr_assert_str_eq(sent(gui), "sbp\n");
    clear_sent(gui);
    gui_dispatch_request(server, gui, "bct 1 2 pouet");
    cr_assert_str_eq(sent(gui), "sbp\n");
    destroy_server(server);
}

/* ----------------------------------------------------------------- mct */

Test(gui_requests, mct_returns_every_tile)
{
    server_t *server = make_server(2, 2);
    client_t *gui = add_gui(server);

    gui_dispatch_request(server, gui, "mct");
    cr_assert_str_eq(sent(gui),
        "bct 0 0 0 0 0 0 0 0 0\n"
        "bct 1 0 0 0 0 0 0 0 0\n"
        "bct 0 1 0 0 0 0 0 0 0\n"
        "bct 1 1 0 0 0 0 0 0 0\n");
    destroy_server(server);
}

/* ----------------------------------------------------------------- ppo */

Test(gui_requests, ppo_returns_player_position_and_orientation)
{
    server_t *server = make_server(10, 10);
    client_t *gui = add_gui(server);
    client_t *player = add_player(server, 3, 6, 1, DIR_SOUTH);

    gui_dispatch_request(server, gui, "ppo #2");
    cr_assert_eq(player->id, 2);
    cr_assert_str_eq(sent(gui), "ppo #2 3 6 3\n");
    destroy_server(server);
}

Test(gui_requests, ppo_accepts_id_without_hash)
{
    server_t *server = make_server(10, 10);
    client_t *gui = add_gui(server);

    add_player(server, 1, 1, 1, DIR_NORTH);
    gui_dispatch_request(server, gui, "ppo 2");
    cr_assert_str_eq(sent(gui), "ppo #2 1 1 1\n");
    destroy_server(server);
}

Test(gui_requests, ppo_unknown_id_returns_sbp)
{
    server_t *server = make_server(10, 10);
    client_t *gui = add_gui(server);

    gui_dispatch_request(server, gui, "ppo #77");
    cr_assert_str_eq(sent(gui), "sbp\n");
    destroy_server(server);
}

// un GUI a un id mais pas de player, faut pas confondre
Test(gui_requests, ppo_on_a_gui_id_returns_sbp)
{
    server_t *server = make_server(10, 10);
    client_t *gui = add_gui(server);

    cr_assert_eq(gui->id, 1);
    gui_dispatch_request(server, gui, "ppo #1");
    cr_assert_str_eq(sent(gui), "sbp\n");
    destroy_server(server);
}

Test(gui_requests, ppo_garbage_id_returns_sbp)
{
    server_t *server = make_server(10, 10);
    client_t *gui = add_gui(server);

    add_player(server, 1, 1, 1, DIR_NORTH);
    gui_dispatch_request(server, gui, "ppo #2bis");
    cr_assert_str_eq(sent(gui), "sbp\n");
    destroy_server(server);
}

/* ----------------------------------------------------------------- plv */

Test(gui_requests, plv_returns_player_level)
{
    server_t *server = make_server(10, 10);
    client_t *gui = add_gui(server);

    add_player(server, 0, 0, 6, DIR_EAST);
    gui_dispatch_request(server, gui, "plv #2");
    cr_assert_str_eq(sent(gui), "plv #2 6\n");
    destroy_server(server);
}

Test(gui_requests, plv_unknown_id_returns_sbp)
{
    server_t *server = make_server(10, 10);
    client_t *gui = add_gui(server);

    gui_dispatch_request(server, gui, "plv #42");
    cr_assert_str_eq(sent(gui), "sbp\n");
    destroy_server(server);
}

/* ----------------------------------------------------------------- pin */

// player_create_at donne 10 food et 0 partout ailleurs
Test(gui_requests, pin_returns_position_and_inventory)
{
    server_t *server = make_server(10, 10);
    client_t *gui = add_gui(server);
    client_t *player = add_player(server, 4, 5, 1, DIR_WEST);

    player->player->inventory[RESOURCE_SIBUR] = 2;
    gui_dispatch_request(server, gui, "pin #2");
    cr_assert_str_eq(sent(gui), "pin #2 4 5 10 0 0 2 0 0 0\n");
    destroy_server(server);
}

Test(gui_requests, pin_unknown_id_returns_sbp)
{
    server_t *server = make_server(10, 10);
    client_t *gui = add_gui(server);

    gui_dispatch_request(server, gui, "pin #9");
    cr_assert_str_eq(sent(gui), "sbp\n");
    destroy_server(server);
}

/* ----------------------------------------------------------------- sst */

Test(gui_requests, sst_updates_frequency)
{
    server_t *server = make_server(5, 5);
    client_t *gui = add_gui(server);

    gui_dispatch_request(server, gui, "sst 250");
    cr_assert_eq(server->cfg->freq, 250);
    cr_assert_str_eq(sent(gui), "sst 250\n");
    destroy_server(server);
}

// sst change toute la partie, les autres GUI doivent savoir
Test(gui_requests, sst_is_broadcast_to_every_gui)
{
    server_t *server = make_server(5, 5);
    client_t *gui1 = add_gui(server);
    client_t *gui2 = add_gui(server);

    gui_dispatch_request(server, gui1, "sst 3");
    cr_assert_str_eq(sent(gui1), "sst 3\n");
    cr_assert_str_eq(sent(gui2), "sst 3\n");
    destroy_server(server);
}

// par contre un sbp concerne que le demandeur
Test(gui_requests, sst_rejection_is_not_broadcast)
{
    server_t *server = make_server(5, 5);
    client_t *gui1 = add_gui(server);
    client_t *gui2 = add_gui(server);

    gui_dispatch_request(server, gui1, "sst 0");
    cr_assert_str_eq(sent(gui1), "sbp\n");
    cr_assert_str_eq(sent(gui2), "");
    cr_assert_eq(server->cfg->freq, 100);
    destroy_server(server);
}

Test(gui_requests, sst_out_of_range_returns_sbp)
{
    server_t *server = make_server(5, 5);
    client_t *gui = add_gui(server);

    gui_dispatch_request(server, gui, "sst 70000");
    cr_assert_str_eq(sent(gui), "sbp\n");
    cr_assert_eq(server->cfg->freq, 100);
    destroy_server(server);
}

Test(gui_requests, sst_garbage_returns_sbp)
{
    server_t *server = make_server(5, 5);
    client_t *gui = add_gui(server);

    gui_dispatch_request(server, gui, "sst pouet");
    cr_assert_str_eq(sent(gui), "sbp\n");
    clear_sent(gui);
    gui_dispatch_request(server, gui, "sst");
    cr_assert_str_eq(sent(gui), "sbp\n");
    cr_assert_eq(server->cfg->freq, 100);
    destroy_server(server);
}

/* -------------------------------------------------------------- erreurs */

Test(gui_requests, unknown_verb_returns_suc)
{
    server_t *server = make_server(5, 5);
    client_t *gui = add_gui(server);

    gui_dispatch_request(server, gui, "pouet");
    cr_assert_str_eq(sent(gui), "suc\n");
    destroy_server(server);
}

Test(gui_requests, empty_line_returns_suc)
{
    server_t *server = make_server(5, 5);
    client_t *gui = add_gui(server);

    gui_dispatch_request(server, gui, "");
    cr_assert_str_eq(sent(gui), "suc\n");
    destroy_server(server);
}

// une commande IA envoyee par un GUI c'est pas une requete valide
Test(gui_requests, player_command_from_a_gui_returns_suc)
{
    server_t *server = make_server(5, 5);
    client_t *gui = add_gui(server);

    gui_dispatch_request(server, gui, "Forward");
    cr_assert_str_eq(sent(gui), "suc\n");
    destroy_server(server);
}

// plusieurs requetes d'affilee s'empilent dans l'ordre
Test(gui_requests, consecutive_requests_queue_in_order)
{
    server_t *server = make_server(4, 3);
    client_t *gui = add_gui(server);

    gui_dispatch_request(server, gui, "msz");
    gui_dispatch_request(server, gui, "sgt");
    cr_assert_str_eq(sent(gui), "msz 4 3\nsgt 100\n");
    destroy_server(server);
}
