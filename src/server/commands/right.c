#include "../../../include/server/commands.h"




// Attention cette fonc change la direction mais ne fait pas avancer, il faut call forward en plus
void command_right(server_t *server, client_t *client, const char *arg)
{
    (void)arg;
    client->player->orientation = client->player->orientation % 4 + 1;
    gui_notify_position(server, client);

    client_queue_send(client, "ok\n");
}
