/*
** Integration tests for Take/Set (src/server/commands/take.c, set.c)
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
        .teams = teams, .teams_nb = 1, .clients_nb = clients_nb, .freq = 100
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

Test(takeset, take_succeeds_when_resource_present_on_tile)
{
    flags_t *cfg = alloc_single_team_cfg(5001, 10, 10, 1);
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
    tile->quantities[RESOURCE_LINEMATE] = 3;

    cr_assert_eq(write(sock, "Take linemate\n", 15), 15);
    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "ok");
    cr_assert_eq(tile->quantities[RESOURCE_LINEMATE], (uint16_t)2);
    cr_assert_eq(server->clients[1]->player->inventory[RESOURCE_LINEMATE],
        (uint16_t)1);
}

Test(takeset, take_fails_when_resource_absent_on_tile)
{
    flags_t *cfg = alloc_single_team_cfg(5002, 10, 10, 1);
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

    cr_assert_eq(write(sock, "Take linemate\n", 15), 15);
    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "ko");
}

Test(takeset, take_fails_for_unknown_resource_name)
{
    flags_t *cfg = alloc_single_team_cfg(5003, 10, 10, 1);
    pthread_t thread;
    int sock;
    char line[256];

    start_test_server(cfg, &thread);
    sock = connect_to_server(cfg->port);
    do_handshake(sock, line, sizeof(line));

    cr_assert_eq(write(sock, "Take nawak\n", 11), 11);
    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "ko");
}

Test(takeset, set_succeeds_when_player_has_resource)
{
    flags_t *cfg = alloc_single_team_cfg(5004, 10, 10, 1);
    pthread_t thread;
    server_t *server;
    int sock;
    char line[256];
    tile_t *tile;
    uint16_t before;

    server = start_test_server(cfg, &thread);
    sock = connect_to_server(cfg->port);
    do_handshake(sock, line, sizeof(line));

    server->clients[1]->player->inventory[RESOURCE_SIBUR] = 2;
    tile = map_get_tile(server->map, server->clients[1]->player->x,
        server->clients[1]->player->y);
    before = tile->quantities[RESOURCE_SIBUR];

    cr_assert_eq(write(sock, "Set sibur\n", 10), 10);
    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "ok");
    cr_assert_eq(server->clients[1]->player->inventory[RESOURCE_SIBUR],
        (uint16_t)1);
    cr_assert_eq(tile->quantities[RESOURCE_SIBUR], (uint16_t)(before + 1));
}

Test(takeset, set_fails_when_player_lacks_resource)
{
    flags_t *cfg = alloc_single_team_cfg(5005, 10, 10, 1);
    pthread_t thread;
    server_t *server;
    int sock;
    char line[256];

    server = start_test_server(cfg, &thread);
    sock = connect_to_server(cfg->port);
    do_handshake(sock, line, sizeof(line));

    server->clients[1]->player->inventory[RESOURCE_PHIRAS] = 0;

    cr_assert_eq(write(sock, "Set phiras\n", 11), 11);
    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "ko");
}

Test(takeset, set_fails_for_unknown_resource_name)
{
    flags_t *cfg = alloc_single_team_cfg(5006, 10, 10, 1);
    pthread_t thread;
    int sock;
    char line[256];

    start_test_server(cfg, &thread);
    sock = connect_to_server(cfg->port);
    do_handshake(sock, line, sizeof(line));

    cr_assert_eq(write(sock, "Set nawak\n", 10), 10);
    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "ko");
}
