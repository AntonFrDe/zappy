#include "../../../include/server/commands.h"

void command_forward(server_t *server, client_t *client, const char *arg)
{
    map_moove(server->map, &client->player.x, FORWARD);

    client_queue_send(client, "ok\n");
}
