#ifndef COMMANDS_H_
    #define COMMANDS_H_
    #include "client.h"
    #include "network.h"

    void command_forward(server_t *server, client_t *client, const char *arg);
    void command_right(server_t *server, client_t *client, const char *arg);
    void command_left(server_t *server, client_t *client, const char *arg);
    void command_look(server_t *server, client_t *client, const char *arg);
    void command_inventory(server_t *server, client_t *client, const char *arg);
    void command_broadcast(server_t *server, client_t *client, const char *arg);
    void command_connect_nbr(server_t *server, client_t *client, const char *arg);
    void command_fork(server_t *server, client_t *client, const char *arg);
    void command_eject(server_t *server, client_t *client, const char *arg);
    void command_take(server_t *server, client_t *client, const char *arg);
    void command_set(server_t *server, client_t *client, const char *arg);
    void command_incantation(server_t *server, client_t *client, const char *arg);

#endif /* !COMMANDS_H_ */
