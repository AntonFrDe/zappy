/*
** Integration tests for GUI protocol emission (src/server/gui_protocol.c
** plus its use in network.c and every command that changes game state).
** Framework: Criterion (each Test() forks, server thread dies with it).
*/

#include <criterion/criterion.h>
#include <server/server.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static void *run_server(void *arg)
{
    server_t *server = arg;

    server_run(server);
    return NULL;
}

static flags_t *alloc_single_team_cfg(int port, uint64_t width,
    uint64_t height, uint16_t clients_nb)
{
    char **teams = malloc(sizeof(char *));
    flags_t *cfg = malloc(sizeof(flags_t));

    teams[0] = "team1";
    *cfg = (flags_t){
        .port = port, .width = width, .height = height,
        .teams = teams, .teams_nb = 1, .clients_nb = clients_nb, .freq = 1000
    };
    return cfg;
}

static server_t *start_test_server(flags_t *cfg, pthread_t *thread)
{
    server_t *server = malloc(sizeof(server_t));

    cr_assert_eq(server_init(server, cfg), 0);
    cr_assert_eq(pthread_create(thread, NULL, run_server, server), 0);
    pthread_detach(*thread);
    return server;
}

static int connect_to_server(int port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {0};

    cr_assert_geq(sock, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    cr_assert_eq(connect(sock, (struct sockaddr *)&addr, sizeof(addr)), 0);
    return sock;
}

static int read_line(int sock, char *out, size_t out_size)
{
    size_t len = 0;
    char c;
    ssize_t n;

    while (len + 1 < out_size) {
        n = read(sock, &c, 1);
        if (n <= 0)
            return -1;
        if (c == '\n')
            break;
        out[len++] = c;
    }
    out[len] = '\0';
    return 0;
}

static void do_handshake(int sock, char *line, size_t line_size)
{
    cr_assert_eq(read_line(sock, line, line_size), 0);
    cr_assert_eq(write(sock, "team1\n", 6), 6);
    cr_assert_eq(read_line(sock, line, line_size), 0);
    cr_assert_eq(read_line(sock, line, line_size), 0);
}

// un GRAPHIC ne recoit plus CLIENT-NUM ni "W H": le snapshot commence
// directement par msz (cf gui_send_snapshot)
static int connect_graphic(int port, char *line, size_t line_size)
{
    int sock = connect_to_server(port);

    cr_assert_eq(read_line(sock, line, line_size), 0);
    cr_assert_str_eq(line, "WELCOME");
    cr_assert_eq(write(sock, "GRAPHIC\n", 8), 8);
    return sock;
}

// lit jusqu'a une ligne commencant par prefix. le serveur intercale des
// bct (respawn periodique, pierres consommees par une incantation) qu'on
// ne veut pas compter: chaque test verifie l'evenement qui l'interesse.
// compter les lignes une par une rendait ces tests faux des qu'on ajoutait
// une ligne au snapshot (sgt, puis enw des oeufs initiaux)
static void read_until(int sock, const char *prefix, char *line,
    size_t line_size)
{
    size_t len = strlen(prefix);

    for (int i = 0; i < 2048; i++) {
        cr_assert_eq(read_line(sock, line, line_size), 0);
        if (strncmp(line, prefix, len) == 0)
            return;
    }
    cr_assert_fail("ligne commencant par '%s' jamais recue", prefix);
}

// connecte un GRAPHIC apres des joueurs; le snapshot est saute par les
// read_until de chaque test, on rend juste la socket
static int connect_graphic_after_players(flags_t *cfg, int players_nb)
{
    char line[256];

    (void)players_nb;
    return connect_graphic(cfg->port, line, sizeof(line));
}

Test(gui_protocol, snapshot_reports_map_size_team_and_every_tile)
{
    flags_t *cfg = alloc_single_team_cfg(5701, 3, 3, 1);
    pthread_t thread;
    int gsock;
    char line[256];

    start_test_server(cfg, &thread);
    gsock = connect_graphic(cfg->port, line, sizeof(line));

    cr_assert_eq(read_line(gsock, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "msz 3 3");

    cr_assert_eq(read_line(gsock, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "sgt 1000");

    cr_assert_eq(read_line(gsock, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "tna team1");

    for (int i = 0; i < 9; i++) {
        cr_assert_eq(read_line(gsock, line, sizeof(line)), 0);
        cr_assert_eq(strncmp(line, "bct ", 4), 0);
    }

    // un oeuf initial par slot (clients_nb x teams_nb)
    cr_assert_eq(read_line(gsock, line, sizeof(line)), 0);
    cr_assert_eq(strncmp(line, "enw ", 4), 0);
}

Test(gui_protocol, new_player_join_is_broadcast_as_pnw_and_pin)
{
    flags_t *cfg = alloc_single_team_cfg(5702, 3, 3, 1);
    pthread_t thread;
    int gsock, sock;
    char line[256];
    int id, x, y, o, level;
    char team[32];
    int q[7];

    start_test_server(cfg, &thread);
    gsock = connect_graphic(cfg->port, line, sizeof(line));

    sock = connect_to_server(cfg->port);
    do_handshake(sock, line, sizeof(line));

    read_until(gsock, "pnw ", line, sizeof(line));
    cr_assert_eq(sscanf(line, "pnw #%d %d %d %d %d %31s",
        &id, &x, &y, &o, &level, team), 6);
    cr_assert_eq(id, 2);
    cr_assert_eq(level, 1);
    cr_assert_str_eq(team, "team1");
    cr_assert(o >= 1 && o <= 4);
    cr_assert(x >= 0 && x < 3);
    cr_assert(y >= 0 && y < 3);

    read_until(gsock, "pin ", line, sizeof(line));
    cr_assert_eq(sscanf(line, "pin #%d %d %d %d %d %d %d %d %d %d",
        &id, &x, &y, &q[0], &q[1], &q[2], &q[3], &q[4], &q[5], &q[6]), 10);
    cr_assert_eq(id, 2);
    cr_assert_eq(q[0], 10);
    for (int i = 1; i < 7; i++)
        cr_assert_eq(q[i], 0);
}

Test(gui_protocol, forward_is_broadcast_as_ppo)
{
    flags_t *cfg = alloc_single_team_cfg(5703, 5, 5, 1);
    pthread_t thread;
    server_t *server;
    int gsock, sock;
    char line[256];

    server = start_test_server(cfg, &thread);
    sock = connect_to_server(cfg->port);
    do_handshake(sock, line, sizeof(line));
    server->clients[1]->player->x = 1;
    server->clients[1]->player->y = 1;
    server->clients[1]->player->orientation = DIR_NORTH;
    gsock = connect_graphic_after_players(cfg, 1);

    cr_assert_eq(write(sock, "Forward\n", 8), 8);
    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "ok");

    read_until(gsock, "ppo ", line, sizeof(line));
    cr_assert_str_eq(line, "ppo #1 1 0 1");
}

Test(gui_protocol, take_and_set_are_broadcast_as_pgt_and_pdr)
{
    flags_t *cfg = alloc_single_team_cfg(5704, 5, 5, 1);
    pthread_t thread;
    server_t *server;
    int gsock, sock;
    char line[256];
    tile_t *tile;

    server = start_test_server(cfg, &thread);
    sock = connect_to_server(cfg->port);
    do_handshake(sock, line, sizeof(line));
    tile = map_get_tile(server->map, server->clients[1]->player->x,
        server->clients[1]->player->y);
    tile->quantities[RESOURCE_LINEMATE] = 1;
    gsock = connect_graphic_after_players(cfg, 1);

    cr_assert_eq(write(sock, "Take linemate\n", 14), 14);
    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "ok");
    read_until(gsock, "pgt ", line, sizeof(line));
    cr_assert_str_eq(line, "pgt #1 1");

    cr_assert_eq(write(sock, "Set linemate\n", 13), 13);
    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "ok");
    read_until(gsock, "pdr ", line, sizeof(line));
    cr_assert_str_eq(line, "pdr #1 1");
}

Test(gui_protocol, broadcast_is_relayed_as_pbc)
{
    flags_t *cfg = alloc_single_team_cfg(5705, 5, 5, 1);
    pthread_t thread;
    int gsock, sock;
    char line[256];

    start_test_server(cfg, &thread);
    sock = connect_to_server(cfg->port);
    do_handshake(sock, line, sizeof(line));
    gsock = connect_graphic_after_players(cfg, 1);

    cr_assert_eq(write(sock, "Broadcast hello\n", 16), 16);
    read_until(gsock, "pbc ", line, sizeof(line));
    cr_assert_str_eq(line, "pbc #1 hello");
}

Test(gui_protocol, fork_is_broadcast_as_pfk_and_enw)
{
    flags_t *cfg = alloc_single_team_cfg(5706, 5, 5, 1);
    pthread_t thread;
    server_t *server;
    int gsock, sock;
    char line[256];
    int64_t px, py;
    int egg_id, parent;
    long long ex, ey;

    server = start_test_server(cfg, &thread);
    sock = connect_to_server(cfg->port);
    do_handshake(sock, line, sizeof(line));
    px = server->clients[1]->player->x;
    py = server->clients[1]->player->y;
    gsock = connect_graphic_after_players(cfg, 1);

    cr_assert_eq(write(sock, "Fork\n", 5), 5);
    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "ok");

    read_until(gsock, "pfk ", line, sizeof(line));
    cr_assert_str_eq(line, "pfk #1");

    // l'id depend des oeufs initiaux deja distribues, on verifie le
    // parent et la position
    read_until(gsock, "enw ", line, sizeof(line));
    cr_assert_eq(sscanf(line, "enw #%d #%d %lld %lld", &egg_id, &parent,
        &ex, &ey), 4);
    cr_assert_eq(parent, 1);
    cr_assert_eq(ex, (long long)px);
    cr_assert_eq(ey, (long long)py);
}

Test(gui_protocol, eject_is_broadcast_as_pex_ppo_and_edi)
{
    flags_t *cfg = alloc_single_team_cfg(5707, 5, 5, 2);
    pthread_t thread;
    server_t *server;
    int gsock, sock1, sock2;
    char line[256];
    char expected[64];
    int egg_id;

    server = start_test_server(cfg, &thread);
    sock1 = connect_to_server(cfg->port);
    do_handshake(sock1, line, sizeof(line));
    sock2 = connect_to_server(cfg->port);
    do_handshake(sock2, line, sizeof(line));
    server->clients[1]->player->x = 2;
    server->clients[1]->player->y = 2;
    server->clients[1]->player->orientation = DIR_NORTH;
    server->clients[2]->player->x = 2;
    server->clients[2]->player->y = 2;
    gsock = connect_graphic_after_players(cfg, 2);

    cr_assert_eq(write(sock1, "Fork\n", 5), 5);
    cr_assert_eq(read_line(sock1, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "ok");
    read_until(gsock, "pfk ", line, sizeof(line));
    read_until(gsock, "enw ", line, sizeof(line));
    cr_assert_eq(sscanf(line, "enw #%d", &egg_id), 1);

    cr_assert_eq(write(sock1, "Eject\n", 6), 6);
    cr_assert_eq(read_line(sock1, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "ok");
    cr_assert_eq(read_line(sock2, line, sizeof(line)), 0);
    cr_assert_eq(strncmp(line, "eject: ", 7), 0);

    read_until(gsock, "pex ", line, sizeof(line));
    cr_assert_str_eq(line, "pex #2");
    read_until(gsock, "ppo ", line, sizeof(line));
    cr_assert_eq(strncmp(line, "ppo #2 ", 7), 0);
    read_until(gsock, "edi ", line, sizeof(line));
    snprintf(expected, sizeof(expected), "edi #%d", egg_id);
    cr_assert_str_eq(line, expected);
}

Test(gui_protocol, incantation_is_broadcast_as_pic_then_pie)
{
    flags_t *cfg = alloc_single_team_cfg(5708, 5, 5, 1);
    pthread_t thread;
    server_t *server;
    int gsock, sock;
    char line[256];
    char expected[64];
    tile_t *tile;
    int64_t px, py;

    server = start_test_server(cfg, &thread);
    sock = connect_to_server(cfg->port);
    do_handshake(sock, line, sizeof(line));
    px = server->clients[1]->player->x;
    py = server->clients[1]->player->y;
    tile = map_get_tile(server->map, px, py);
    tile->quantities[RESOURCE_LINEMATE] = 1;
    gsock = connect_graphic_after_players(cfg, 1);

    cr_assert_eq(write(sock, "Incantation\n", 12), 12);
    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "Elevation underway");

    read_until(gsock, "pic ", line, sizeof(line));
    snprintf(expected, sizeof(expected), "pic %lld %lld 1 #1",
        (long long)px, (long long)py);
    cr_assert_str_eq(line, expected);

    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "Current level: 2");

    // un bct s'intercale ici depuis #24: les pierres consommees
    read_until(gsock, "plv ", line, sizeof(line));
    cr_assert_str_eq(line, "plv #1 2");

    read_until(gsock, "pie ", line, sizeof(line));
    snprintf(expected, sizeof(expected), "pie %lld %lld 1",
        (long long)px, (long long)py);
    cr_assert_str_eq(line, expected);
}

Test(gui_protocol, starvation_death_is_broadcast_as_pdi)
{
    flags_t *cfg = alloc_single_team_cfg(5709, 5, 5, 1);
    pthread_t thread;
    server_t *server;
    int gsock, sock;
    char line[256];

    server = start_test_server(cfg, &thread);
    sock = connect_to_server(cfg->port);
    do_handshake(sock, line, sizeof(line));
    gsock = connect_graphic_after_players(cfg, 1);
    server->clients[1]->player->inventory[RESOURCE_FOOD] = 0;

    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "dead");

    read_until(gsock, "pdi ", line, sizeof(line));
    cr_assert_str_eq(line, "pdi #1");
}
