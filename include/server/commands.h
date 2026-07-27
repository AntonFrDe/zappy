#ifndef COMMANDS_H_
    #define COMMANDS_H_
    #include "client.h"
    #include "network.h"
    #include "gui_protocol.h"

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
// associe et gele les joueurs de meme niveau trouves sur la case
    void incantation_freeze_others(server_t *server, client_t *client);
// libere tous les participants associes a l'incantation de client
    void incantation_unfreeze_players(server_t *server, int owner_id);
// diffuse pic aux GUI (participants = client + mob geles sur la case)
    void incantation_notify_start(server_t *server, client_t *client);

#endif /* !COMMANDS_H_ */
