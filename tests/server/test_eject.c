/*
** Integration tests for Eject (src/server/commands/eject.c)
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

Test(eject, fails_when_alone_on_tile)
{
    flags_t *cfg = alloc_single_team_cfg(5301, 10, 10, 1);
    pthread_t thread;
    int sock;
    char line[256];

    start_test_server(cfg, &thread);
    sock = connect_to_server(cfg->port);
    do_handshake(sock, line, sizeof(line));

    cr_assert_eq(write(sock, "Eject\n", 6), 6);
    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "ko");
}

Test(eject, pushes_player_sharing_the_tile_and_notifies_them)
{
    flags_t *cfg = alloc_single_team_cfg(5302, 20, 20, 2);
    pthread_t thread;
    server_t *server;
    int sock1, sock2;
    char line[256];

    server = start_test_server(cfg, &thread);
    sock1 = connect_to_server(cfg->port);
    do_handshake(sock1, line, sizeof(line));
    sock2 = connect_to_server(cfg->port);
    do_handshake(sock2, line, sizeof(line));

    server->clients[1]->player->x = 5;
    server->clients[1]->player->y = 5;
    server->clients[1]->player->orientation = DIR_NORTH;
    server->clients[2]->player->x = 5;
    server->clients[2]->player->y = 5;
    server->clients[2]->player->orientation = DIR_NORTH;

    cr_assert_eq(write(sock1, "Eject\n", 6), 6);
    cr_assert_eq(read_line(sock1, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "ok");

    cr_assert_eq(read_line(sock2, line, sizeof(line)), 0);
    cr_assert_eq(strncmp(line, "eject: ", 7), 0);

    cr_assert_eq(server->clients[2]->player->x, (int64_t)5);
    cr_assert_eq(server->clients[2]->player->y, (int64_t)4);
}

Test(eject, does_not_affect_player_on_a_different_tile)
{
    flags_t *cfg = alloc_single_team_cfg(5303, 20, 20, 2);
    pthread_t thread;
    server_t *server;
    int sock1, sock2;
    char line[256];

    server = start_test_server(cfg, &thread);
    sock1 = connect_to_server(cfg->port);
    do_handshake(sock1, line, sizeof(line));
    sock2 = connect_to_server(cfg->port);
    do_handshake(sock2, line, sizeof(line));

    server->clients[1]->player->x = 5;
    server->clients[1]->player->y = 5;
    server->clients[2]->player->x = 10;
    server->clients[2]->player->y = 10;

    cr_assert_eq(write(sock1, "Eject\n", 6), 6);
    cr_assert_eq(read_line(sock1, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "ko");

    cr_assert_eq(server->clients[2]->player->x, (int64_t)10);
    cr_assert_eq(server->clients[2]->player->y, (int64_t)10);
}
