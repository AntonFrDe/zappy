#include "../../../include/server/commands.h"

void command_forward(server_t *server, client_t *client, const char *arg)
{
    (void)arg;
    map_move(server->map, &client->player->x, &client->player->y, client->player->orientation);

    client_queue_send(client, "ok\n");
}
