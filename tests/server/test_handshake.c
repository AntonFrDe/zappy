/*
** Integration tests for the real handshake over a loopback TCP socket
** (accept_new_client / handle_team_name / disconnect_client in network.c).
** Framework: Criterion (each Test() runs in its own forked process, so
** the server thread started here dies with the process at test end).
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

static void start_test_server(server_t *server, flags_t *cfg, pthread_t *thread)
{
    cr_assert_eq(server_init(server, cfg), 0);
    cr_assert_eq(pthread_create(thread, NULL, run_server, server), 0);
    pthread_detach(*thread);
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

Test(handshake, valid_team_gets_client_num_and_map_size)
{
    char *teams[] = { "team1" };
    flags_t cfg = {
        .port = 4601, .width = 10, .height = 10,
        .teams = teams, .teams_nb = 1, .clients_nb = 2, .freq = 100
    };
    server_t server;
    pthread_t thread;
    int sock;
    char line[256];

    start_test_server(&server, &cfg, &thread);
    sock = connect_to_server(cfg.port);

    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "WELCOME");

    cr_assert_eq(write(sock, "team1\n", 6), 6);

    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "1");

    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "10 10");

    close(sock);
}

Test(handshake, unknown_team_disconnects_client)
{
    char *teams[] = { "team1" };
    flags_t cfg = {
        .port = 4602, .width = 8, .height = 8,
        .teams = teams, .teams_nb = 1, .clients_nb = 1, .freq = 100
    };
    server_t server;
    pthread_t thread;
    int sock;
    char line[256];
    char c;

    start_test_server(&server, &cfg, &thread);
    sock = connect_to_server(cfg.port);

    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "WELCOME");

    cr_assert_eq(write(sock, "nawak\n", 6), 6);

    cr_assert_eq(read(sock, &c, 1), 0);

    close(sock);
}

Test(handshake, graphic_gets_client_num_zero_and_map_size)
{
    char *teams[] = { "team1" };
    flags_t cfg = {
        .port = 4603, .width = 15, .height = 20,
        .teams = teams, .teams_nb = 1, .clients_nb = 1, .freq = 100
    };
    server_t server;
    pthread_t thread;
    int sock;
    char line[256];

    start_test_server(&server, &cfg, &thread);
    sock = connect_to_server(cfg.port);

    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "WELCOME");

    cr_assert_eq(write(sock, "GRAPHIC\n", 8), 8);

    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "0");

    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "15 20");

    close(sock);
}

Test(handshake, full_team_disconnects_next_client)
{
    char *teams[] = { "team1" };
    flags_t cfg = {
        .port = 4604, .width = 5, .height = 5,
        .teams = teams, .teams_nb = 1, .clients_nb = 1, .freq = 100
    };
    server_t server;
    pthread_t thread;
    int sock1, sock2;
    char line[256];
    char c;

    start_test_server(&server, &cfg, &thread);

    sock1 = connect_to_server(cfg.port);
    cr_assert_eq(read_line(sock1, line, sizeof(line)), 0);
    cr_assert_eq(write(sock1, "team1\n", 6), 6);
    cr_assert_eq(read_line(sock1, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "0");

    sock2 = connect_to_server(cfg.port);
    cr_assert_eq(read_line(sock2, line, sizeof(line)), 0);
    cr_assert_eq(write(sock2, "team1\n", 6), 6);
    cr_assert_eq(read(sock2, &c, 1), 0);

    close(sock1);
    close(sock2);
}

Test(handshake, crlf_team_name_is_accepted)
{
    char *teams[] = { "team1" };
    flags_t cfg = {
        .port = 4605, .width = 12, .height = 12,
        .teams = teams, .teams_nb = 1, .clients_nb = 1, .freq = 100
    };
    server_t server;
    pthread_t thread;
    int sock;
    char line[256];

    start_test_server(&server, &cfg, &thread);
    sock = connect_to_server(cfg.port);

    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    cr_assert_eq(write(sock, "team1\r\n", 7), 7);

    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "0");
}
