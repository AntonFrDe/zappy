/*
** Integration tests for the dynamic poll() timeout (network.c next_timeout).
** Le serveur doit dormir jusqu'a la prochaine echeance et pas plus:
** avant le fix, poll() etait fige a 100ms, donc chaque commande attendait
** un tour de boucle complet au lieu de son cout reel (action / f).
** Ces tests sont chronometriques, avec des marges larges: ils detectent un
** facteur 5-10, pas quelques ms de jitter.
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
#include <time.h>

static void *run_server(void *arg)
{
    server_t *server = arg;

    server_run(server);
    return NULL;
}

static flags_t *alloc_cfg(int port, uint16_t freq)
{
    char **teams = malloc(sizeof(char *));
    flags_t *cfg = malloc(sizeof(flags_t));

    teams[0] = "team1";
    *cfg = (flags_t){
        .port = port, .width = 10, .height = 10, .teams = teams,
        .teams_nb = 1, .clients_nb = 5, .freq = freq
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

static int connect_player(int port)
{
    int sock = connect_to_server(port);
    char line[256];

    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    cr_assert_eq(write(sock, "team1\n", 6), 6);
    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    return sock;
}

static long long now_ms(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

// envoie nb Inventory a la suite (cout 1 u.t. chacun) et rend le temps
// total en ms ; une seule commande en vol a la fois, comme l'IA
static long long time_n_inventories(int port, int nb)
{
    int sock = connect_player(port);
    char line[512];
    long long start = now_ms();

    for (int i = 0; i < nb; i++) {
        cr_assert_eq(write(sock, "Inventory\n", 10), 10);
        cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    }
    close(sock);
    return now_ms() - start;
}

// 10 Inventory a f=100 = 10 x 1/100s = 100ms attendus.
// Avant le fix chaque commande attendait le poll(100) -> ~1000ms.
Test(timeout, commands_are_not_capped_by_a_fixed_poll_delay)
{
    pthread_t thread;
    flags_t *cfg = alloc_cfg(5951, 100);
    long long elapsed;

    start_test_server(cfg, &thread);
    elapsed = time_n_inventories(5951, 10);
    cr_assert_lt(elapsed, 400,
        "10 Inventory a f=100 ont pris %lldms, attendu ~100ms "
        "(1000ms = poll() fige)", elapsed);
}

// le vrai test de conformite au sujet: monter -f doit vraiment accelerer.
// avant le fix les deux mesures etaient noyees dans le poll(100).
Test(timeout, higher_frequency_really_is_faster)
{
    pthread_t slow_thread;
    pthread_t fast_thread;
    long long slow;
    long long fast;

    start_test_server(alloc_cfg(5952, 20), &slow_thread);
    start_test_server(alloc_cfg(5953, 500), &fast_thread);
    slow = time_n_inventories(5952, 10);
    fast = time_n_inventories(5953, 10);
    cr_assert_gt(slow, fast * 3,
        "f=20 a pris %lldms et f=500 %lldms: la frequence n'est pas "
        "respectee", slow, fast);
}

// une commande longue ne doit pas etre servie plus vite que son cout
Test(timeout, a_command_still_waits_for_its_own_cost)
{
    pthread_t thread;
    flags_t *cfg = alloc_cfg(5954, 100);
    int sock;
    char line[512];
    long long start;
    long long elapsed;

    start_test_server(cfg, &thread);
    sock = connect_player(5954);
    start = now_ms();
    cr_assert_eq(write(sock, "Forward\n", 8), 8);
    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    elapsed = now_ms() - start;
    cr_assert_str_eq(line, "ok");
    cr_assert_geq(elapsed, 60,
        "Forward (7/100 = 70ms) a repondu en %lldms, trop tot", elapsed);
    close(sock);
}

// enchainement de commandes de couts differents: on veut la somme des
// couts, pas nb_commandes x delai fixe
Test(timeout, mixed_costs_add_up_to_their_real_total)
{
    pthread_t thread;
    flags_t *cfg = alloc_cfg(5955, 100);
    int sock;
    char line[512];
    long long start;
    long long elapsed;

    start_test_server(cfg, &thread);
    sock = connect_player(5955);
    start = now_ms();
    for (int i = 0; i < 5; i++) {
        cr_assert_eq(write(sock, "Inventory\n", 10), 10);
        cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    }
    cr_assert_eq(write(sock, "Forward\n", 8), 8);
    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    elapsed = now_ms() - start;
    cr_assert_lt(elapsed, 400,
        "5 Inventory + 1 Forward a f=100 = ~120ms attendus, pris %lldms",
        elapsed);
    close(sock);
}
