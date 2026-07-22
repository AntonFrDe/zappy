#include "../../../include/server/commands.h"




// Attention cette fonc change la direction mais ne fait pas avancer, il faut call forward en plus 
void command_right(server_t *server, client_t *client, const char *arg)
{
    client->player.direction = (client->player.direction + 1) % 4;

    client_queue_send(client, "ok\n");
}
