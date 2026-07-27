#include "../../../include/server/commands.h"

static int64_t wrap_coord(int64_t v, int64_t max)
{
    return ((v % max) + max) % max;
}

static void get_forward_lateral(direction_t dir, int64_t *fx, int64_t *fy,
    int64_t *lx, int64_t *ly)
{
    switch (dir) {
    case DIR_NORTH:
        *fx = 0;
        *fy = -1;
        *lx = 1;
        *ly = 0;
        break;

    case DIR_EAST:
        *fx = 1;
        *fy = 0;
        *lx = 0;
        *ly = 1;
        break;

    case DIR_SOUTH:
        *fx = 0;
        *fy = 1;
        *lx = -1;
        *ly = 0;
        break;

    case DIR_WEST:
        *fx = -1;
        *fy = 0;
        *lx = 0;
        *ly = -1;
        break;
        
    }
}

static int count_players_at(server_t *server, int64_t x, int64_t y)
{
    client_t *other;
    int count = 0;

    for (int i = 1; i < server->nfds; i++) {
        other = server->clients[i];

        if (other->player == NULL)
            continue;

        if (other->player->x == x && other->player->y == y)
            count++;
    }
    return count;
}

static void send_tile_content(server_t *server, client_t *self,
    int64_t x, int64_t y)
{
    tile_t *tile = map_get_tile(server->map, x, y);
    bool first = true;
    int player_count = count_players_at(server, x, y);

    for (int n = 0; n < player_count; n++) {
        if (!first)
            client_queue_send(self, " ");
        client_queue_send(self, "player");
        first = false;
    }
    for (int r = 0; r < RESOURCE_COUNT; r++)
        for (int n = 0; n < tile->quantities[r]; n++) {
            if (!first)
                client_queue_send(self, " ");

            client_queue_send(self, resource_name(r));
            first = false;
        }
}

void command_look(server_t *server, client_t *client, const char *arg)
{
    int64_t fx, fy, lx, ly;
    int64_t x, y;
    bool first_tile = true;

    (void)arg;
    get_forward_lateral(client->player->orientation, &fx, &fy, &lx, &ly);
    client_queue_send(client, "[");

    for (int row = 0; row <= client->player->level; row++) {
        for (int lat = -row; lat <= row; lat++) {

            // Recup les coor de case dans les ligne de regard
            x = wrap_coord(client->player->x + row * fx + lat * lx, (int64_t)server->map->width);
            y = wrap_coord(client->player->y + row * fy + lat * ly, (int64_t)server->map->height);
            
            if (!first_tile)
                client_queue_send(client, ",");
            first_tile = false;

            send_tile_content(server, client, x, y);
        }
    }
    client_queue_send(client, "]\n");
}
