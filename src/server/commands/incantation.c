#include "../../../include/server/commands.h"
#include <stdio.h>

typedef struct {
    int nb_players;
    uint16_t stones[RESOURCE_COUNT];
} elevation_req_t;

static const elevation_req_t REQUIREMENTS[8] = {
    [1] = {1, { [RESOURCE_LINEMATE] = 1 }},
    [2] = {2, { [RESOURCE_LINEMATE] = 1, [RESOURCE_DERAUMERE] = 1,
        [RESOURCE_SIBUR] = 1 }},
    [3] = {2, { [RESOURCE_LINEMATE] = 2, [RESOURCE_SIBUR] = 1,
        [RESOURCE_PHIRAS] = 2 }},
    [4] = {4, { [RESOURCE_LINEMATE] = 1, [RESOURCE_DERAUMERE] = 1,
        [RESOURCE_SIBUR] = 2, [RESOURCE_PHIRAS] = 1 }},
    [5] = {4, { [RESOURCE_LINEMATE] = 1, [RESOURCE_DERAUMERE] = 2,
        [RESOURCE_SIBUR] = 1, [RESOURCE_MENDIANE] = 3 }},
    [6] = {6, { [RESOURCE_LINEMATE] = 1, [RESOURCE_DERAUMERE] = 2,
        [RESOURCE_SIBUR] = 3, [RESOURCE_PHIRAS] = 1 }},
    [7] = {6, { [RESOURCE_LINEMATE] = 2, [RESOURCE_DERAUMERE] = 2,
        [RESOURCE_SIBUR] = 2, [RESOURCE_MENDIANE] = 2, [RESOURCE_PHIRAS] = 2,
        [RESOURCE_THYSTAME] = 1 }},
};

static bool same_spot_same_level(player_t *a, player_t *b)
{
    return a->x == b->x && a->y == b->y && a->level == b->level;
}

static int count_players_same_level(server_t *server, client_t *client)
{
    int count = 0;
    client_t *other;

    for (int i = 1; i < server->nfds; i++) {
        other = server->clients[i];
        if (other->player == NULL)
            continue;
        if (same_spot_same_level(other->player, client->player))
            count++;
    }
    return count;
}

bool incantation_check(server_t *server, client_t *client)
{
    int level = client->player->level;
    const elevation_req_t *req;
    tile_t *tile;

    if (level < 1 || level > 7)
        return false;
    req = &REQUIREMENTS[level];
    if (count_players_same_level(server, client) < req->nb_players)
        return false;
    tile = map_get_tile(server->map, client->player->x, client->player->y);
    for (int r = 0; r < RESOURCE_COUNT; r++)
        if (tile->quantities[r] < req->stones[r])
            return false;
    return true;
}

void incantation_freeze_others(server_t *server, client_t *client)
{
    client_t *other;

    for (int i = 1; i < server->nfds; i++) {
        other = server->clients[i];
        if (other == client || other->player == NULL)
            continue;
        if (same_spot_same_level(other->player, client->player)) {
            other->busy = true;
            other->ready_at = client->ready_at;
        }
    }
}

static void elevate_players_on_tile(server_t *server, client_t *client)
{
    client_t *other;
    int64_t x = client->player->x;
    int64_t y = client->player->y;
    int level = client->player->level;

    for (int i = 1; i < server->nfds; i++) {
        other = server->clients[i];
        if (other->player == NULL)
            continue;
        if (other->player->x == x && other->player->y == y
            && other->player->level == level) {
            other->player->level++;
            gui_notify_level(server, other);
        }
    }
}

void incantation_notify_start(server_t *server, client_t *client)
{
    int ids[SERVER_MAX_CLIENTS];
    int count = 0;
    client_t *other;

    ids[count++] = client->id;
    for (int i = 1; i < server->nfds; i++) {
        other = server->clients[i];
        if (other == client || other->player == NULL)
            continue;
        if (same_spot_same_level(other->player, client->player))
            ids[count++] = other->id;
    }
    gui_notify_incantation_start(server, client->player->x,
        client->player->y, client->player->level, ids, count);
}

void command_incantation(server_t *server, client_t *client, const char *arg)
{
    char msg[32];
    const elevation_req_t *req;
    tile_t *tile;
    int64_t x = client->player->x;
    int64_t y = client->player->y;

    (void)arg;
    if (!incantation_check(server, client)) {
        client_queue_send(client, "ko\n");
        gui_notify_incantation_end(server, x, y, 0);
        return;
    }
    req = &REQUIREMENTS[client->player->level];
    tile = map_get_tile(server->map, x, y);
    for (int r = 0; r < RESOURCE_COUNT; r++)
        tile->quantities[r] -= req->stones[r];
    elevate_players_on_tile(server, client);
    gui_notify_incantation_end(server, x, y, 1);
    snprintf(msg, sizeof(msg), "Current level: %d\n", client->player->level);
    client_queue_send(client, msg);
}
