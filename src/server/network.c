#include "../../include/server/network.h"
#include "../../include/server/command.h"
#include "../../include/server/gui_protocol.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

static int create_listen_socket(int port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int yes = 1;
    struct sockaddr_in addr = {0};

    if (fd < 0)
        return -1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((uint16_t)port);
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    if (listen(fd, SERVER_BACKLOG) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

int server_init(server_t *server, flags_t *cfg)
{
    memset(server, 0, sizeof(*server));
    server->cfg = cfg;
    server->listen_fd = create_listen_socket(cfg->port);
    if (server->listen_fd < 0)
        return -1;
    server->slots_remaining = malloc(sizeof(uint16_t) * cfg->teams_nb);
    if (server->slots_remaining == NULL) {
        close(server->listen_fd);
        return -1;
    }
    server->map = map_create(cfg->width, cfg->height);
    if (server->map == NULL) {
        free(server->slots_remaining);
        close(server->listen_fd);
        return -1;
    }
    srand((unsigned int)time(NULL));
    map_spawn_resources(server->map);
    egg_list_init(&server->eggs);
    server->next_client_id = 1;
    server->next_egg_id = 1;
    server->next_resource_spawn = compute_ready_at(20, cfg->freq);
    for (int i = 0; i < cfg->teams_nb; i++)
        server->slots_remaining[i] = cfg->clients_nb;
    server->fds[0].fd = server->listen_fd;
    server->fds[0].events = POLLIN;
    server->nfds = 1;
    return 0;
}

void server_destroy(server_t *server)
{
    for (int i = 1; i < server->nfds; i++)
        client_destroy(server->clients[i]);
    if (server->listen_fd >= 0)
        close(server->listen_fd);
    free(server->slots_remaining);
    map_destroy(server->map);
    egg_list_destroy(&server->eggs);
}

static void accept_new_client(server_t *server)
{
    int fd = accept(server->listen_fd, NULL, NULL);
    client_t *client;

    if (fd < 0)
        return;
    if (server->nfds > SERVER_MAX_CLIENTS) {
        close(fd);
        return;
    }
    client = client_create(fd);
    if (client == NULL) {
        close(fd);
        return;
    }
    client->id = server->next_client_id++;
    client_queue_send(client, "WELCOME\n");
    server->clients[server->nfds] = client;
    server->fds[server->nfds].fd = fd;
    server->fds[server->nfds].events = POLLIN;
    server->nfds++;
}

void disconnect_client(server_t *server, int client_idx)
{
    client_destroy(server->clients[client_idx]);
    for (int i = client_idx; i < server->nfds - 1; i++) {
        server->clients[i] = server->clients[i + 1];
        server->fds[i] = server->fds[i + 1];
    }
    server->nfds--;
}

static int find_team_index(server_t *server, const char *name)
{
    for (int i = 0; i < server->cfg->teams_nb; i++)
        if (strcmp(server->cfg->teams[i], name) == 0)
            return i;
    return -1;
}

static void send_map_size(server_t *server, client_t *client)
{
    char msg[64];

    snprintf(msg, sizeof(msg), "%llu %llu\n",
        (unsigned long long)server->cfg->width,
        (unsigned long long)server->cfg->height);
    client_queue_send(client, msg);
}

// renvoie true si le client a ete deconnecte (handshake refuse)
static bool handle_team_name(server_t *server, client_t *client,
    const char *name, int client_idx)
{
    char msg[64];
    //Timmyy !!
    int team_i;
    int64_t egg_x, egg_y;
    int egg_id = -1;

    if (strcmp(name, "GRAPHIC") == 0) {
        client->is_graphic = true;
        client->handshake_done = true;
        client_queue_send(client, "0\n");
        send_map_size(server, client);
        gui_send_snapshot(server, client);
        return false;
    }
    team_i = find_team_index(server, name);
    if (team_i < 0 || server->slots_remaining[team_i] == 0) {
        // DECO LE CLIENT EN CAS DE PROBLEM
        disconnect_client(server, client_idx);
        return true;
    }
    server->slots_remaining[team_i]--;
    client->team_index = team_i;
    client->handshake_done = true;
    if (egg_list_pop_random(&server->eggs, team_i, &egg_x, &egg_y, &egg_id))
        client->player = player_create_at(egg_x, egg_y);
    else
        client->player = player_create(server->map);
    client->player->next_food_tick = compute_ready_at(126, server->cfg->freq);
    snprintf(msg, sizeof(msg), "%u\n", server->slots_remaining[team_i]);
    client_queue_send(client, msg);
    send_map_size(server, client);
    gui_notify_new_player(server, client);
    if (egg_id >= 0)
        gui_notify_egg_hatch(server, egg_id);
    return false;
}

// renvoie 1 si le client a ete deco pendant le traitement (handshake refuse)
static int process_client_input(server_t *server, int client_idx)
{
    client_t *client = server->clients[client_idx];
    char line[512];

    while (client_extract_line(client, line, sizeof(line))) {
        if (!client->handshake_done) {
            if (handle_team_name(server, client, line, client_idx))
                return 1;
        } else {
            client_push_cmd(client, line);
        }
    }
    return 0;
}

static void update_poll_events(server_t *server)
{
    for (int i = 1; i < server->nfds; i++)
        server->fds[i].events = server->clients[i]->send_len
            > server->clients[i]->send_sent ? (POLLIN | POLLOUT) : POLLIN;
}

// 1 si deco (tableau decale), 0 sinon
static int handle_client_events(server_t *server, int i)
{
    int n;

    if (server->fds[i].revents & POLLIN) {
        n = client_read(server->clients[i]);
        if (n <= 0) {
            disconnect_client(server, i);
            return 1;
        }
        if (process_client_input(server, i))
            return 1;
    }
    if (server->fds[i].revents & POLLOUT)
        client_flush_send(server->clients[i]);
    return 0;
}

// Boucle server
int server_run(server_t *server)
{
    int ready;

    while (true) {
        update_poll_events(server);
        ready = poll(server->fds, (nfds_t)server->nfds, 100);

        if (ready < 0)
            return -1;
        
        if (server->fds[0].revents & POLLIN)
            accept_new_client(server);

        for (int i = 1; i < server->nfds; i++)
            if (handle_client_events(server, i))
                i--;
        scheduler_tick(server);
    }
    return 0;
}
