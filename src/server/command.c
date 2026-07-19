#include "../../include/server/command.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    const char *name;
    int cost;
} command_cost_entry_t;

// couts en unites de temps, cf tableau des commandes du sujet
static const command_cost_entry_t COMMAND_COSTS[] = {
    { "Forward", 7 },
    { "Right", 7 },
    { "Left", 7 },
    { "Look", 7 },
    { "Inventory", 1 },
    { "Broadcast", 7 },
    { "Connect_nbr", 0 },
    { "Fork", 42 },
    { "Eject", 7 },
    { "Take", 7 },
    { "Set", 7 },
    { "Incantation", 300 },
};
#define COMMAND_COSTS_NB (sizeof(COMMAND_COSTS) / sizeof(COMMAND_COSTS[0]))

// ne garde que le premier mot (le verbe), ex: "Take linemate" -> "Take"
static void extract_verb(const char *cmd, char *verb, size_t verb_size)
{
    size_t i = 0;

    while (cmd[i] != '\0' && cmd[i] != ' ' && i + 1 < verb_size) {
        verb[i] = cmd[i];
        i++;
    }
    verb[i] = '\0';
}

int command_cost(const char *cmd)
{
    char verb[32];

    extract_verb(cmd, verb, sizeof(verb));
    for (size_t i = 0; i < COMMAND_COSTS_NB; i++)
        if (strcmp(COMMAND_COSTS[i].name, verb) == 0)
            return COMMAND_COSTS[i].cost;
    return -1;
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
    int cost;

    if (cmd == NULL)
        return;
    cost = command_cost(cmd);
    if (cost < 0) {
        client_queue_send(client, "ko\n");
        free(cmd);
        return;
    }
    client->executing_cmd = cmd;
    client->ready_at = compute_ready_at(cost, server->cfg->freq);
    client->busy = true;
}

static void finish_command(client_t *client)
{
    // TODO: dispatch reel par commande (Forward/Look/Take/...).
    // pour l'instant: accuse de reception generique, prouve que le
    // mecanisme timing marche de bout en bout.
    client_queue_send(client, "ok\n");
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
                finish_command(client);
        } else {
            start_next_command(server, client);
        }
    }
}
