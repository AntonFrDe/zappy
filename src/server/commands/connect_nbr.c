#include "../../../include/server/commands.h"
#include <stdio.h>

void command_connect_nbr(server_t *server, client_t *client, const char *arg)
{
    char msg[32];

    (void)arg;
    snprintf(msg, sizeof(msg), "%u\n", server->slots_remaining[client->team_index]);
    client_queue_send(client, msg);
}
