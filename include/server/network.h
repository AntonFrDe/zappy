#ifndef NETWORK_H_
    #define NETWORK_H_
    #include <poll.h>
    #include "struct.h"
    #include "client.h"

    #define SERVER_MAX_CLIENTS 128
    #define SERVER_BACKLOG 16

    typedef struct server_s {
        int listen_fd;
        struct pollfd fds[SERVER_MAX_CLIENTS + 1];
        int nfds;
        client_t *clients[SERVER_MAX_CLIENTS + 1];
        flags_t *cfg;
        uint16_t *slots_remaining;
    } server_t;

    int server_init(server_t *server, flags_t *cfg);
    int server_run(server_t *server);
    void server_destroy(server_t *server);

#endif /* !NETWORK_H_ */
