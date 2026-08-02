/*
** Integration tests for Incantation (src/server/commands/incantation.c)
** over a real loopback socket.
** Framework: Criterion (each Test() forks, server thread dies with it).
*/

#include <criterion/criterion.h>
#include <server/server.h>
#include <pthread.h>
#include <string.h>
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

Test(incantation, fails_immediately_without_enough_stones)
{
    flags_t *cfg = alloc_single_team_cfg(5401, 10, 10, 1);
    pthread_t thread;
    server_t *server;
    int sock;
    char line[256];
    tile_t *tile;

    server = start_test_server(cfg, &thread);
    sock = connect_to_server(cfg->port);
    do_handshake(sock, line, sizeof(line));

    tile = map_get_tile(server->map, server->clients[1]->player->x,
        server->clients[1]->player->y);
    tile->quantities[RESOURCE_LINEMATE] = 0;

    cr_assert_eq(write(sock, "Incantation\n", 12), 12);
    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "ko");
}

Test(incantation, fails_without_enough_players_for_level_two)
{
    flags_t *cfg = alloc_single_team_cfg(5402, 10, 10, 1);
    pthread_t thread;
    server_t *server;
    int sock;
    char line[256];
    tile_t *tile;

    server = start_test_server(cfg, &thread);
    sock = connect_to_server(cfg->port);
    do_handshake(sock, line, sizeof(line));

    server->clients[1]->player->level = 2;
    tile = map_get_tile(server->map, server->clients[1]->player->x,
        server->clients[1]->player->y);
    tile->quantities[RESOURCE_LINEMATE] = 5;
    tile->quantities[RESOURCE_DERAUMERE] = 5;
    tile->quantities[RESOURCE_SIBUR] = 5;

    cr_assert_eq(write(sock, "Incantation\n", 12), 12);
    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "ko");
}

Test(incantation, succeeds_and_levels_up_when_alone_at_level_one)
{
    flags_t *cfg = alloc_single_team_cfg(5403, 10, 10, 1);
    pthread_t thread;
    server_t *server;
    int sock;
    char line[256];
    tile_t *tile;

    server = start_test_server(cfg, &thread);
    sock = connect_to_server(cfg->port);
    do_handshake(sock, line, sizeof(line));

    tile = map_get_tile(server->map, server->clients[1]->player->x,
        server->clients[1]->player->y);
    tile->quantities[RESOURCE_LINEMATE] = 1;

    cr_assert_eq(write(sock, "Incantation\n", 12), 12);
    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "Elevation underway");

    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "Current level: 2");
    cr_assert_eq(server->clients[1]->player->level, 2);
}

Test(incantation, elevates_other_participant_sharing_the_tile)
{
    flags_t *cfg = alloc_single_team_cfg(5404, 10, 10, 2);
    pthread_t thread;
    server_t *server;
    int sock1, sock2;
    char line[256];
    tile_t *tile;

    server = start_test_server(cfg, &thread);
    sock1 = connect_to_server(cfg->port);
    do_handshake(sock1, line, sizeof(line));
    sock2 = connect_to_server(cfg->port);
    do_handshake(sock2, line, sizeof(line));

    server->clients[1]->player->x = 3;
    server->clients[1]->player->y = 3;
    server->clients[2]->player->x = 3;
    server->clients[2]->player->y = 3;

    tile = map_get_tile(server->map, 3, 3);
    tile->quantities[RESOURCE_LINEMATE] = 1;

    cr_assert_eq(write(sock1, "Incantation\n", 12), 12);
    cr_assert_eq(read_line(sock1, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "Elevation underway");
    cr_assert_eq(read_line(sock1, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "Current level: 2");

    cr_assert_eq(server->clients[1]->player->level, 2);
    cr_assert_eq(server->clients[2]->player->level, 2);
}

// le check a lieu au debut ET a la fin (cf sujet). si les pierres
// disparaissent entre les deux, TOUS les participants doivent recevoir ko,
// pas seulement celui qui a lance: les autres ont recu "Elevation
// underway" et attendent une reponse
Test(incantation, failed_incantation_notifies_every_participant)
{
    flags_t *cfg = alloc_single_team_cfg(5405, 10, 10, 2);
    pthread_t thread;
    server_t *server;
    int sock1, sock2;
    char line[256];
    tile_t *tile;

    server = start_test_server(cfg, &thread);
    sock1 = connect_to_server(cfg->port);
    do_handshake(sock1, line, sizeof(line));
    sock2 = connect_to_server(cfg->port);
    do_handshake(sock2, line, sizeof(line));

    server->clients[1]->player->x = 4;
    server->clients[1]->player->y = 4;
    server->clients[2]->player->x = 4;
    server->clients[2]->player->y = 4;
    tile = map_get_tile(server->map, 4, 4);
    tile->quantities[RESOURCE_LINEMATE] = 1;

    cr_assert_eq(write(sock1, "Incantation\n", 12), 12);
    cr_assert_eq(read_line(sock1, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "Elevation underway");
    cr_assert_eq(read_line(sock2, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "Elevation underway");

    // pendant les 300/1000 = 0.3s du rituel, la pierre est ramassee
    tile->quantities[RESOURCE_LINEMATE] = 0;

    cr_assert_eq(read_line(sock1, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "ko");
    cr_assert_eq(read_line(sock2, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "ko");
    cr_assert_eq(server->clients[1]->player->level, 1);
    cr_assert_eq(server->clients[2]->player->level, 1);
}
