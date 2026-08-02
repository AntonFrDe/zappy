/*
** Integration tests for egg hatching (src/server/egg.c, plus its use in
** network.c on connect, fork.c on Fork, eject.c on Eject).
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

static flags_t *alloc_multi_team_cfg(int port, uint16_t teams_nb,
    uint16_t clients_nb)
{
    static char *const NAMES[] = {"team1", "team2", "team3"};
    char **teams = malloc(sizeof(char *) * teams_nb);
    flags_t *cfg = malloc(sizeof(flags_t));

    for (int i = 0; i < teams_nb; i++)
        teams[i] = NAMES[i];
    *cfg = (flags_t){
        .port = port, .width = 10, .height = 10,
        .teams = teams, .teams_nb = teams_nb, .clients_nb = clients_nb,
        .freq = 1000
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

Test(egg, fork_adds_one_egg_for_the_team)
{
    flags_t *cfg = alloc_single_team_cfg(5601, 20, 20, 2);
    pthread_t thread;
    server_t *server;
    int sock;
    char line[256];
    int before;

    server = start_test_server(cfg, &thread);
    sock = connect_to_server(cfg->port);
    do_handshake(sock, line, sizeof(line));

    // depuis les oeufs initiaux la liste n'est plus vide au depart, on
    // compte donc le delta et pas la valeur absolue
    before = server->eggs.count;
    cr_assert_eq(write(sock, "Fork\n", 5), 5);
    cr_assert_eq(read_line(sock, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "ok");
    cr_assert_eq(server->eggs.count, before + 1);
    cr_assert_eq(server->eggs.eggs[server->eggs.count - 1].team_index, 0);
}

Test(egg, new_connection_hatches_at_forked_egg_position)
{
    flags_t *cfg = alloc_single_team_cfg(5602, 20, 20, 2);
    pthread_t thread;
    server_t *server;
    int sock1, sock2;
    char line[256];

    server = start_test_server(cfg, &thread);
    sock1 = connect_to_server(cfg->port);
    do_handshake(sock1, line, sizeof(line));

    server->clients[1]->player->x = 7;
    server->clients[1]->player->y = 8;

    // on jette les oeufs initiaux: sinon pop_random pourrait piocher l'un
    // d'eux au lieu de celui du Fork, et le test devient aleatoire
    server->eggs.count = 0;

    cr_assert_eq(write(sock1, "Fork\n", 5), 5);
    cr_assert_eq(read_line(sock1, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "ok");

    sock2 = connect_to_server(cfg->port);
    do_handshake(sock2, line, sizeof(line));

    cr_assert_eq(server->clients[2]->player->x, (int64_t)7);
    cr_assert_eq(server->clients[2]->player->y, (int64_t)8);
    cr_assert_eq(server->eggs.count, 0);
}

Test(egg, eject_destroys_eggs_sharing_the_tile)
{
    flags_t *cfg = alloc_single_team_cfg(5603, 20, 20, 2);
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
    server->clients[2]->player->x = 5;
    server->clients[2]->player->y = 5;

    cr_assert_eq(write(sock1, "Fork\n", 5), 5);
    cr_assert_eq(read_line(sock1, line, sizeof(line)), 0);
    cr_assert_eq(server->eggs.count, 1);

    cr_assert_eq(write(sock1, "Eject\n", 6), 6);
    cr_assert_eq(read_line(sock1, line, sizeof(line)), 0);
    cr_assert_str_eq(line, "ok");
    cr_assert_eq(server->eggs.count, 0);
}

// un slot = un oeuf: clients_nb x teams_nb au demarrage
Test(egg, server_init_creates_one_egg_per_slot)
{
    server_t server;
    flags_t *cfg = alloc_multi_team_cfg(5611, 3, 4);

    cr_assert_eq(server_init(&server, cfg), 0);
    cr_assert_eq(server.eggs.count, 12);
    server_destroy(&server);
}

Test(egg, initial_eggs_are_split_evenly_between_teams)
{
    server_t server;
    flags_t *cfg = alloc_multi_team_cfg(5612, 2, 5);
    int per_team[2] = {0, 0};

    cr_assert_eq(server_init(&server, cfg), 0);
    for (int i = 0; i < server.eggs.count; i++)
        per_team[server.eggs.eggs[i].team_index]++;
    cr_assert_eq(per_team[0], 5);
    cr_assert_eq(per_team[1], 5);
    server_destroy(&server);
}

// positions dans les bornes de la carte, et personne ne les a pondus
Test(egg, initial_eggs_are_inside_the_map_and_have_no_parent)
{
    server_t server;
    flags_t *cfg = alloc_multi_team_cfg(5613, 2, 3);

    cr_assert_eq(server_init(&server, cfg), 0);
    for (int i = 0; i < server.eggs.count; i++) {
        cr_assert_geq(server.eggs.eggs[i].x, (int64_t)0);
        cr_assert_lt(server.eggs.eggs[i].x, (int64_t)10);
        cr_assert_geq(server.eggs.eggs[i].y, (int64_t)0);
        cr_assert_lt(server.eggs.eggs[i].y, (int64_t)10);
        cr_assert_eq(server.eggs.eggs[i].parent_id, INITIAL_EGG_PARENT);
    }
    server_destroy(&server);
}

// next_egg_id doit etre mis a 1 avant la creation, sinon les ids partent de 0
Test(egg, initial_egg_ids_start_at_one)
{
    server_t server;
    flags_t *cfg = alloc_multi_team_cfg(5614, 1, 3);

    cr_assert_eq(server_init(&server, cfg), 0);
    cr_assert_eq(server.eggs.eggs[0].id, 1);
    server_destroy(&server);
}

// un joueur qui rejoint eclot sur un oeuf, plus a une position au hasard
Test(egg, first_player_hatches_from_an_initial_egg)
{
    flags_t *cfg = alloc_single_team_cfg(5615, 20, 20, 3);
    pthread_t thread;
    server_t *server;
    int sock;
    char line[256];
    int64_t ex[8];
    int64_t ey[8];
    int nb;
    bool found = false;

    server = start_test_server(cfg, &thread);
    nb = server->eggs.count;
    cr_assert_eq(nb, 3);
    for (int i = 0; i < nb; i++) {
        ex[i] = server->eggs.eggs[i].x;
        ey[i] = server->eggs.eggs[i].y;
    }
    sock = connect_to_server(cfg->port);
    do_handshake(sock, line, sizeof(line));
    cr_assert_eq(server->eggs.count, nb - 1);
    for (int i = 0; i < nb; i++)
        if (server->clients[1]->player->x == ex[i]
            && server->clients[1]->player->y == ey[i])
            found = true;
    cr_assert(found, "le joueur n'a pas eclos sur un oeuf initial");
}
