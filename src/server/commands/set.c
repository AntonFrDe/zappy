#include "../../../include/server/commands.h"

void command_set(server_t *server, client_t *client, const char *arg)
{
    (void)server;
    (void)arg;
    client_queue_send(client, "ok\n");
}
