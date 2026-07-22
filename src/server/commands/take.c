#include "../../../include/server/commands.h"

void command_take(server_t *server, client_t *client, const char *arg)
{
    int r = resource_from_name(arg);
    tile_t *tile;

    if (r < 0) {
        client_queue_send(client, "ko\n");
        return;
    }

    tile = map_get_tile(server->map, client->player->x, client->player->y);
    
    if (tile->quantities[r] == 0) {
        client_queue_send(client, "ko\n");
        return;
    }
    tile->quantities[r]--;
    client->player->inventory[r]++;
    client_queue_send(client, "ok\n");
}
