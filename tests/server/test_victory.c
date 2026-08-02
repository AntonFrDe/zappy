/*
** Unit tests for the win condition (src/server/victory.c).
** Pas de socket ni de thread: server_t monte a la main, victory_check
** appele direct, on relit client->send_buf des GUI.
** Le dernier test passe par une vraie incantation, seul chemin qui
** declenche victory_check en production (incantation.c).
** Framework: Criterion.
*/

#include <criterion/criterion.h>
#include <server/server.h>
#include <server/victory.h>
#include <stdlib.h>
#include <string.h>

static server_t *make_server(uint16_t teams_nb)
{
    server_t *server = malloc(sizeof(server_t));
    flags_t *cfg = malloc(sizeof(flags_t));
    char **teams = malloc(sizeof(char *) * teams_nb);
    static char *const NAMES[] = {"rouge", "bleu"};

    memset(server, 0, sizeof(*server));
    for (int i = 0; i < teams_nb; i++)
        teams[i] = NAMES[i];
    *cfg = (flags_t){
        .port = 0, .width = 5, .height = 5, .teams = teams,
        .teams_nb = teams_nb, .clients_nb = 10, .freq = 100
    };
    server->cfg = cfg;
    server->map = map_create(5, 5);
    cr_assert_not_null(server->map);
    server->slots_remaining = malloc(sizeof(uint16_t) * teams_nb);
    for (int i = 0; i < teams_nb; i++)
        server->slots_remaining[i] = cfg->clients_nb;
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

// fd = -1 : client_destroy call pas close(), rien part sur le reseau
static client_t *attach(server_t *server)
{
    client_t *client = client_create(-1);

    cr_assert_not_null(client);
    client->id = server->next_client_id++;
    client->handshake_done = true;
    server->clients[server->nfds] = client;
    server->fds[server->nfds].fd = -1;
    server->nfds++;
    return client;
}

static client_t *add_gui(server_t *server)
{
    client_t *gui = attach(server);

    gui->is_graphic = true;
    return gui;
}

static client_t *add_player(server_t *server, int team_index, int level,
    int64_t x, int64_t y)
{
    client_t *client = attach(server);

    client->team_index = team_index;
    client->player = player_create_at(x, y);
    cr_assert_not_null(client->player);
    client->player->level = level;
    client->player->next_food_tick = compute_ready_at(126, server->cfg->freq);
    return client;
}

static void add_players(server_t *server, int team, int nb, int level)
{
    for (int i = 0; i < nb; i++)
        add_player(server, team, level, 0, 0);
}

static const char *sent(client_t *client)
{
    return client->send_buf != NULL ? client->send_buf : "";
}

/* ------------------------------------------------------- seuil des 6 */

Test(victory, five_players_at_max_level_is_not_enough)
{
    server_t *server = make_server(1);
    client_t *gui = add_gui(server);

    add_players(server, 0, 5, 8);
    victory_check(server, 0);
    cr_assert_not(server->game_over);
    cr_assert_str_eq(sent(gui), "");
    destroy_server(server);
}

Test(victory, six_players_at_max_level_ends_the_game)
{
    server_t *server = make_server(1);
    client_t *gui = add_gui(server);

    add_players(server, 0, 6, 8);
    victory_check(server, 0);
    cr_assert(server->game_over);
    cr_assert_str_eq(sent(gui), "seg rouge\n");
    destroy_server(server);
}

// server_run boucle sur !game_over: sans ce drapeau la partie continue
Test(victory, game_over_flag_is_what_stops_the_server)
{
    server_t *server = make_server(1);

    add_gui(server);
    add_players(server, 0, 6, 8);
    cr_assert_not(server->game_over);
    victory_check(server, 0);
    cr_assert(server->game_over);
    destroy_server(server);
}

/* --------------------------------------------------- qui compte ou pas */

// il faut le niveau 8, pas 7
Test(victory, level_seven_players_do_not_count)
{
    server_t *server = make_server(1);
    client_t *gui = add_gui(server);

    add_players(server, 0, 5, 8);
    add_players(server, 0, 1, 7);
    victory_check(server, 0);
    cr_assert_not(server->game_over);
    cr_assert_str_eq(sent(gui), "");
    destroy_server(server);
}

// le test est >= 8, un niveau superieur compte quand meme
Test(victory, levels_above_max_still_count)
{
    server_t *server = make_server(1);

    add_players(server, 0, 6, 9);
    victory_check(server, 0);
    cr_assert(server->game_over);
    destroy_server(server);
}

// 3 rouges + 3 bleus niveau 8 : personne n'a gagne
Test(victory, players_of_another_team_do_not_count)
{
    server_t *server = make_server(2);
    client_t *gui = add_gui(server);

    add_players(server, 0, 3, 8);
    add_players(server, 1, 3, 8);
    victory_check(server, 0);
    victory_check(server, 1);
    cr_assert_not(server->game_over);
    cr_assert_str_eq(sent(gui), "");
    destroy_server(server);
}

Test(victory, the_winning_team_name_is_the_one_announced)
{
    server_t *server = make_server(2);
    client_t *gui = add_gui(server);

    add_players(server, 0, 2, 8);
    add_players(server, 1, 6, 8);
    victory_check(server, 1);
    cr_assert(server->game_over);
    cr_assert_str_eq(sent(gui), "seg bleu\n");
    destroy_server(server);
}

// un GRAPHIC a player == NULL et team_index == -1, il ne doit jamais
// compter dans un effectif
Test(victory, graphic_clients_are_not_counted)
{
    server_t *server = make_server(1);

    add_gui(server);
    add_gui(server);
    add_players(server, 0, 5, 8);
    victory_check(server, 0);
    cr_assert_not(server->game_over);
    destroy_server(server);
}

/* ------------------------------------------------------ pas deux fois */

// victory_check est appele une fois par joueur eleve: le seg ne doit
// partir qu'une seule fois
Test(victory, seg_is_announced_only_once)
{
    server_t *server = make_server(1);
    client_t *gui = add_gui(server);

    add_players(server, 0, 6, 8);
    victory_check(server, 0);
    victory_check(server, 0);
    victory_check(server, 0);
    cr_assert_str_eq(sent(gui), "seg rouge\n");
    destroy_server(server);
}

Test(victory, every_gui_receives_the_end_of_game)
{
    server_t *server = make_server(1);
    client_t *gui1 = add_gui(server);
    client_t *gui2 = add_gui(server);

    add_players(server, 0, 6, 8);
    victory_check(server, 0);
    cr_assert_str_eq(sent(gui1), "seg rouge\n");
    cr_assert_str_eq(sent(gui2), "seg rouge\n");
    destroy_server(server);
}

/* ------------------------------------------ le vrai chemin: 7 -> 8 */

// 7->8 demande 6 joueurs et 2 linemate, 2 deraumere, 2 sibur,
// 2 mendiane, 2 phiras, 1 thystame (cf sujet)
Test(victory, an_incantation_reaching_level_eight_wins_the_game)
{
    server_t *server = make_server(1);
    client_t *gui = add_gui(server);
    client_t *leader = add_player(server, 0, 7, 2, 2);
    tile_t *tile = map_get_tile(server->map, 2, 2);

    for (int i = 0; i < 5; i++)
        add_player(server, 0, 7, 2, 2);
    tile->quantities[RESOURCE_LINEMATE] = 2;
    tile->quantities[RESOURCE_DERAUMERE] = 2;
    tile->quantities[RESOURCE_SIBUR] = 2;
    tile->quantities[RESOURCE_MENDIANE] = 2;
    tile->quantities[RESOURCE_PHIRAS] = 2;
    tile->quantities[RESOURCE_THYSTAME] = 1;

    incantation_freeze_others(server, leader);
    command_incantation(server, leader, "");

    cr_assert_eq(leader->player->level, 8);
    cr_assert(server->game_over);
    cr_assert_not_null(strstr(sent(gui), "seg rouge\n"), "recu: %s",
        sent(gui));
    destroy_server(server);
}

// meme situation mais a 5 joueurs: l'incantation echoue, pas de victoire
Test(victory, five_players_cannot_reach_level_eight)
{
    server_t *server = make_server(1);
    client_t *gui = add_gui(server);
    client_t *leader = add_player(server, 0, 7, 3, 3);
    tile_t *tile = map_get_tile(server->map, 3, 3);

    for (int i = 0; i < 4; i++)
        add_player(server, 0, 7, 3, 3);
    tile->quantities[RESOURCE_LINEMATE] = 2;
    tile->quantities[RESOURCE_DERAUMERE] = 2;
    tile->quantities[RESOURCE_SIBUR] = 2;
    tile->quantities[RESOURCE_MENDIANE] = 2;
    tile->quantities[RESOURCE_PHIRAS] = 2;
    tile->quantities[RESOURCE_THYSTAME] = 1;

    command_incantation(server, leader, "");

    cr_assert_eq(leader->player->level, 7);
    cr_assert_not(server->game_over);
    cr_assert_null(strstr(sent(gui), "seg "));
    destroy_server(server);
}
