#include "../../../include/server/commands.h"

void command_set(server_t *server, client_t *client, const char *arg)
{
    int r = resource_from_name(arg);
    tile_t *tile;

    if (r < 0) {
        client_queue_send(client, "ko\n");
        return;
    }

    if (client->player->inventory[r] == 0) {
        client_queue_send(client, "ko\n");
        return;
    }
    
    tile = map_get_tile(server->map, client->player->x, client->player->y);
    client->player->inventory[r]--;
    tile->quantities[r]++;
    client_queue_send(client, "ok\n");
}
