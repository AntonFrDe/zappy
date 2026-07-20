#include "../../include/server/command.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef void (*command_handler_t)(server_t *server, client_t *client, const char *arg);

static void handle_ok(server_t *server, client_t *client, const char *arg)
{
    (void)server;
    (void)arg;
    client_queue_send(client, "ok\n");
}

static void handle_look(server_t *server, client_t *client, const char *arg)
{
    (void)server;
    (void)arg;
    client_queue_send(client, "[]\n");
}

static void handle_inventory(server_t *server, client_t *client, const char *arg)
{
    (void)server;
    (void)arg;
    client_queue_send(client,
        "[food 0, linemate 0, deraumere 0, sibur 0, "
        "mendiane 0, phiras 0, thystame 0]\n");
}

static void handle_connect_nbr(server_t *server, client_t *client, const char *arg)
{
    (void)server;
    (void)arg;
    client_queue_send(client, "0\n");
}

static void handle_incantation(server_t *server, client_t *client, const char *arg)
{
    (void)server;
    (void)arg;
    client_queue_send(client, "Elevation underway\n");
}

typedef struct {
    const char *name;
    int cost;
    command_handler_t handler;
} command_entry_t;

static const command_entry_t COMMANDS[] = {
    { "Forward", 7, handle_ok },
    { "Right", 7, handle_ok },
    { "Left", 7, handle_ok },
    { "Look", 7, handle_look },
    { "Inventory", 1, handle_inventory },
    { "Broadcast", 7, handle_ok },
    { "Connect_nbr", 0, handle_connect_nbr },
    { "Fork", 42, handle_ok },
    { "Eject", 7, handle_ok },
    { "Take", 7, handle_ok },
    { "Set", 7, handle_ok },
    { "Incantation", 300, handle_incantation },
};
#define COMMANDS_NB (sizeof(COMMANDS) / sizeof(COMMANDS[0]))

static void extract_verb(const char *cmd, char *verb, size_t verb_size)
{
    size_t i = 0;

    while (cmd[i] != '\0' && cmd[i] != ' ' && i + 1 < verb_size) {
        verb[i] = cmd[i];
        i++;
    }
    verb[i] = '\0';
}

static const char *extract_arg(const char *cmd)
{
    const char *sp = strchr(cmd, ' ');

    return sp != NULL ? sp + 1 : "";
}

static const command_entry_t *find_command(const char *cmd)
{
    char verb[32];

    extract_verb(cmd, verb, sizeof(verb));
    for (size_t i = 0; i < COMMANDS_NB; i++)
        if (strcmp(COMMANDS[i].name, verb) == 0)
            return &COMMANDS[i];
    return NULL;
}

int command_cost(const char *cmd)
{
    const command_entry_t *entry = find_command(cmd);

    return entry != NULL ? entry->cost : -1;
}

static struct timespec compute_ready_at(int cost, uint16_t freq)
{
    struct timespec now;
    double seconds = (double)cost / (double)freq;
    long ns;

    clock_gettime(CLOCK_MONOTONIC, &now);
    ns = now.tv_nsec + (long)(seconds * 1000000000.0);
    now.tv_sec += ns / 1000000000L;
    now.tv_nsec = ns % 1000000000L;
    return now;
}

static bool is_ready(struct timespec ready_at)
{
    struct timespec now;

    clock_gettime(CLOCK_MONOTONIC, &now);
    if (now.tv_sec != ready_at.tv_sec)
        return now.tv_sec > ready_at.tv_sec;
    return now.tv_nsec >= ready_at.tv_nsec;
}

static void start_next_command(server_t *server, client_t *client)
{
    char *cmd = client_pop_cmd(client);
    const command_entry_t *entry;

    if (cmd == NULL)
        return;
    entry = find_command(cmd);
    if (entry == NULL) {
        client_queue_send(client, "ko\n");
        free(cmd);
        return;
    }
    client->executing_cmd = cmd;
    client->ready_at = compute_ready_at(entry->cost, server->cfg->freq);
    client->busy = true;
}

static void finish_command(server_t *server, client_t *client)
{
    const command_entry_t *entry = find_command(client->executing_cmd);

    if (entry != NULL)
        entry->handler(server, client, extract_arg(client->executing_cmd));
    free(client->executing_cmd);
    client->executing_cmd = NULL;
    client->busy = false;
}

void scheduler_tick(server_t *server)
{
    client_t *client;

    for (int i = 1; i < server->nfds; i++) {
        client = server->clients[i];
        if (!client->handshake_done || client->is_graphic)
            continue;
        if (client->busy) {
            if (is_ready(client->ready_at))
                finish_command(server, client);
        } else {
            start_next_command(server, client);
        }
    }
}
