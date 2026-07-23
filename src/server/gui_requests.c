#include "../../include/server/gui_requests.h"

void gui_dispatch_request(server_t *server, client_t *client,
    const char *line)
{
    (void)server;
    (void)line;
    // TODO phase 1: table de verbes (msz, sgt, tna, bct, mct, ppo, plv,
    // pin, sst) ; en attendant tout est traite comme une commande inconnue
    client_queue_send(client, "suc\n");
}
