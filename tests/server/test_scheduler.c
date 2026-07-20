/*
** Integration tests for the command scheduler (command.c / scheduler_tick)
** over a real loopback socket, exercising the full pipeline:
** client sends a command -> queued -> popped by scheduler_tick ->
** delayed by action/f -> generic "ok\n" response.
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
#include <poll.h>

static void *run_server(void *arg)
{
    server_t *server = arg;

    server_run(server);
    return NULL;
}

static flags_t *alloc_single_team_cfg(int port, uint64_t width,
    uint64_t height, uint16_t clients_nb, uint16_t freq)
{
    char **teams = malloc(sizeof(char *));
    flags_t *cfg = malloc(sizeof(flags_t));

    teams[0] = "team1";
    *cfg = (flags_t){
        .port = port, .width = width, .height = height,
        .teams = teams, .teams_nb = 1, .clients_nb = clients_nb, .freq = freq
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

static bool data_available_within(int sock, int timeout_ms)
{
    struct pollfd pfd = { .fd = sock, .events = POLLIN, .revents = 0 };

    return poll(&pfd, 1, timeout_ms) > 0;
}

static void do_handshake(int sock, char *line, size_t line_size)
{
    cr_assert_eq(read_line(sock, line, line_size), 0);
    cr_assert_eq(write(sock, "team1\n", 6), 6);
    cr_assert_eq(read_line(sock, line, line_size), 0);
    cr_assert_eq(read_line(sock, line, line_size), 0);
}

Test(scheduler, valid_command_eventually_gets_ok)
{
    flags_t *cfg = alloc_single_team_cfg(4701, 10, 10, 1, 1000);
    pthread_t thread;
    int sock;
    char line[256];

    start_test_server(cfg, &thread);
    sock = connect_to_server(cfg->port);
    do_handshake(sock, line, sizeof(line));

    cr_assert_eq(write(sock, "Inventory\n", 10), 10);
    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "ok");
}

Test(scheduler, unknown_command_gets_ko)
{
    flags_t *cfg = alloc_single_team_cfg(4702, 10, 10, 1, 1000);
    pthread_t thread;
    int sock;
    char line[256];

    start_test_server(cfg, &thread);
    sock = connect_to_server(cfg->port);
    do_handshake(sock, line, sizeof(line));

    cr_assert_eq(write(sock, "Nawak\n", 6), 6);
    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "ko");
}

Test(scheduler, response_respects_delay_not_instant)
{
    flags_t *cfg = alloc_single_team_cfg(4703, 10, 10, 1, 2);
    pthread_t thread;
    int sock;
    char line[256];

    start_test_server(cfg, &thread);
    sock = connect_to_server(cfg->port);
    do_handshake(sock, line, sizeof(line));

    cr_assert_eq(write(sock, "Inventory\n", 10), 10);
    cr_assert_eq(data_available_within(sock, 100), false);
    cr_assert_eq(data_available_within(sock, 2000), true);
    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "ok");
}
