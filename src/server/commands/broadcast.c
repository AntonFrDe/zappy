#include "../../../include/server/commands.h"
#include <stdio.h>
#include <math.h>

static int64_t shortest_delta(int64_t raw, int64_t size)
{
    int64_t d = ((raw % size) + size) % size;

    if (d > size / 2)
        d -= size;
    return d;
}

static void rotate_to_receiver_frame(direction_t orientation,
    int64_t dx, int64_t dy, int64_t *rx, int64_t *ry)
{
    switch (orientation) {
    case DIR_NORTH:
        *rx = dx;
        *ry = dy;
        break;
    case DIR_EAST:
        *rx = dy;
        *ry = -dx;
        break;
    case DIR_SOUTH:
        *rx = -dx;
        *ry = -dy;
        break;
    case DIR_WEST:
        *rx = -dy;
        *ry = dx;
        break;
    }
}

static int compute_direction(server_t *server, player_t *receiver,
    player_t *emitter)
{
    int64_t dx = shortest_delta(emitter->x - receiver->x,
        (int64_t)server->map->width);
    int64_t dy = shortest_delta(emitter->y - receiver->y,
        (int64_t)server->map->height);
    int64_t rx, ry;
    double angle;

    if (dx == 0 && dy == 0)
        return 0;
    rotate_to_receiver_frame(receiver->orientation, dx, dy, &rx, &ry);
    angle = atan2((double)rx, (double)-ry) * (180.0 / M_PI);
    if (angle < 0)
        angle += 360.0;
    return (int)((angle + 22.5) / 45.0) % 8 + 1;
}

void command_broadcast(server_t *server, client_t *client, const char *arg)
{
    char msg[600];
    client_t *other;

    for (int i = 1; i < server->nfds; i++) {
        other = server->clients[i];
        if (other->player == NULL)
            continue;
        snprintf(msg, sizeof(msg), "message %d, %s\n",
            compute_direction(server, other->player, client->player), arg);
        client_queue_send(other, msg);
    }
    client_queue_send(client, "ok\n");
}
