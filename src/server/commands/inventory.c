#include "../../../include/server/commands.h"
#include <stdio.h>
#include <string.h>

void command_inventory(server_t *server, client_t *client, const char *arg)
{
    char out[256] = "[";
    char part[32];

    (void)server;
    (void)arg;

    for (int r = 0; r < RESOURCE_COUNT; r++) {
        snprintf(part, sizeof(part), "%s%s %u", r == 0 ? "" : ", ", resource_name(r), client->player->inventory[r]);
        strncat(out, part, sizeof(out) - strlen(out) - 1);
    }
    
    strncat(out, "]\n", sizeof(out) - strlen(out) - 1);
    client_queue_send(client, out);
}
