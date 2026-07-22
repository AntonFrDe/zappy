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

// verifie joueurs (meme niveau) + pierres sur la case du client
    bool incantation_check(server_t *server, client_t *client);
// gele (meme ready_at) les mob trouves sur la case
    void incantation_freeze_others(server_t *server, client_t *client);

#endif /* !COMMANDS_H_ */
