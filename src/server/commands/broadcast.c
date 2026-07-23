#include "../../../include/server/commands.h"
#include <stdio.h>

void command_broadcast(server_t *server, client_t *client, const char *arg)
{
    char msg[600];
    client_t *other;
    int dir;

    for (int i = 1; i < server->nfds; i++) {
        other = server->clients[i];
        if (other->player == NULL)
            continue;
        dir = map_direction(server->map, other->player->x, other->player->y,
            other->player->orientation, client->player->x, client->player->y);
        snprintf(msg, sizeof(msg), "message %d, %s\n", dir, arg);
        client_queue_send(other, msg);
    }
    gui_notify_broadcast(server, client, arg);
    client_queue_send(client, "ok\n");
}
