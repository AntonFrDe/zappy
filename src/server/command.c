#include "../../include/server/command.h"
#include "../../include/server/commands.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef void (*command_handler_t)(server_t *server, client_t *client, const char *arg);

typedef struct {
    const char *name;
    int cost;
    command_handler_t handler;
} command_entry_t;

static const command_entry_t COMMANDS[] = {
    { "Forward", 7, command_forward },
    { "Right", 7, command_right },
    { "Left", 7, command_left },
    { "Look", 7, command_look },
    { "Inventory", 1, command_inventory },
    { "Broadcast", 7, command_broadcast },
    { "Connect_nbr", 0, command_connect_nbr },
    { "Fork", 42, command_fork },
    { "Eject", 7, command_eject },
    { "Take", 7, command_take },
    { "Set", 7, command_set },
    { "Incantation", 300, command_incantation },
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

struct timespec compute_ready_at(int cost, uint16_t freq)
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
    if (entry->handler == command_incantation && !incantation_check(server, client)) {
        client_queue_send(client, "ko\n");
        free(cmd);
        return;
    }
    client->executing_cmd = cmd;
    client->ready_at = compute_ready_at(entry->cost, server->cfg->freq);
    client->busy = true;
    if (entry->handler == command_incantation) {
        client_queue_send(client, "Elevation underway\n");
        incantation_freeze_others(server, client);
        incantation_notify_start(server, client);
    }
}

static void finish_command(server_t *server, client_t *client)
{
    const command_entry_t *entry;

    if (client->executing_cmd == NULL) {
        client->busy = false;
        return;
    }
    entry = find_command(client->executing_cmd);
    if (entry != NULL)
        entry->handler(server, client, extract_arg(client->executing_cmd));
    free(client->executing_cmd);
    client->executing_cmd = NULL;
    client->busy = false;
}

// true si client meurt de faim (deconnecte, tableau decale)
static bool check_food(server_t *server, int idx)
{
    client_t *client = server->clients[idx];
    player_t *player = client->player;

    if (!is_ready(player->next_food_tick))
        return false;
    if (player->inventory[RESOURCE_FOOD] == 0) {
        client_queue_send(client, "dead\n");
        client_flush_send(client);
        gui_notify_death(server, client);
        disconnect_client(server, idx);
        return true;
    }
    player->inventory[RESOURCE_FOOD]--;
    player->next_food_tick = compute_ready_at(126, server->cfg->freq);
    return false;
}

void scheduler_tick(server_t *server)
{
    client_t *client;

    for (int i = 1; i < server->nfds; i++) {
        client = server->clients[i];
        if (!client->handshake_done || client->is_graphic)
            continue;
        if (check_food(server, i)) {
            i--;
            continue;
        }
        if (client->busy) {
            if (is_ready(client->ready_at))
                finish_command(server, client);
        } else {
            start_next_command(server, client);
        }
    }
}
