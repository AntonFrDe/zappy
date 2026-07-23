#include "../../../include/server/commands.h"
#include <stdio.h>

void command_eject(server_t *server, client_t *client, const char *arg)
{
    bool ejected_someone = false;
    client_t *other;
    int64_t old_x, old_y;
    char msg[32];
    int dir;

    (void)arg;
    for (int i = 1; i < server->nfds; i++) {
        other = server->clients[i];
        if (other == client || other->player == NULL)
            continue;
        if (other->player->x != client->player->x
            || other->player->y != client->player->y)
            continue;
        old_x = other->player->x;
        old_y = other->player->y;
        map_move(server->map, &other->player->x, &other->player->y,
            client->player->orientation);
        dir = map_direction(server->map, other->player->x, other->player->y,
            other->player->orientation, old_x, old_y);
        snprintf(msg, sizeof(msg), "eject: %d\n", dir);
        client_queue_send(other, msg);
        ejected_someone = true;
    }
    if (ejected_someone)
        egg_list_destroy_at(&server->eggs, client->player->x,
            client->player->y);
    client_queue_send(client, ejected_someone ? "ok\n" : "ko\n");
}
