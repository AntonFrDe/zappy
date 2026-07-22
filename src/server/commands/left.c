#include "../../../include/server/commands.h"


// Attention cette fonc change la direction mais ne fait pas avancer, il faut call forward en plus
void command_left(server_t *server, client_t *client, const char *arg)
{
    (void)server;
    (void)arg;
    client->player->orientation = (client->player->orientation + 2) % 4 + 1;

    client_queue_send(client, "ok\n");
}
