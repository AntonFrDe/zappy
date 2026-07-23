#include "../../../include/server/commands.h"

void command_fork(server_t *server, client_t *client, const char *arg)
{
    int egg_id;

    (void)arg;
    server->slots_remaining[client->team_index]++;
    egg_id = egg_list_add(&server->eggs, &server->next_egg_id,
        client->team_index, client->id, client->player->x, client->player->y);
    gui_notify_egg_laying(server, client);
    gui_notify_new_egg(server, egg_id, client->id, client->player->x,
        client->player->y);
    client_queue_send(client, "ok\n");
}
