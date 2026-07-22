#include "../../../include/server/commands.h"

void command_connect_nbr(server_t *server, client_t *client, const char *arg)
{
    (void)server;
    (void)arg;
    client_queue_send(client, "0\n");
}
