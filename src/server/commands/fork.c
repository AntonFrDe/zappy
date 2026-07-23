#include "../../../include/server/commands.h"

void command_fork(server_t *server, client_t *client, const char *arg)
{
    (void)arg;
    server->slots_remaining[client->team_index]++;
    egg_list_add(&server->eggs, client->team_index,
        client->player->x, client->player->y);
    client_queue_send(client, "ok\n");
}
