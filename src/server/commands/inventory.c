#include "../../../include/server/commands.h"

void command_inventory(server_t *server, client_t *client, const char *arg)
{
    (void)server;
    (void)arg;
    client_queue_send(client,
        "[food 0, linemate 0, deraumere 0, sibur 0, "
        "mendiane 0, phiras 0, thystame 0]\n");
}
