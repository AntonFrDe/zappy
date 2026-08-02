#ifndef CLIENT_H_
    #define CLIENT_H_
    #include <stdint.h>
    #include <stdbool.h>
    #include <stddef.h>
    #include <time.h>
    #include "player.h"

    #define CLIENT_RECV_BUF_SIZE 4096
    #define CLIENT_MAX_QUEUED_CMDS 10
    #define CLIENT_MAX_SEND_BUF (16 * 1024 * 1024)
// seuil de remplissage du snapshot: doit rester TRES en dessous du
// plafond, sinon la tuile suivante fait sauter must_disconnect
    #define CLIENT_SEND_WATERMARK (1024 * 1024)
    #define CLIENT_MAX_MAP_ONCE_READ (1024 * 1024)

    typedef struct client_s {
        int fd;
        int id;                 // stable, ne bouge pas quand le tableau est decale
        char recv_buf[CLIENT_RECV_BUF_SIZE];
        size_t recv_len;
        char *send_buf;
        size_t send_len;
        size_t send_sent;
        bool handshake_done;
        bool is_graphic;
        int team_index;
        char *cmd_queue[CLIENT_MAX_QUEUED_CMDS];
        int cmd_count;
        bool busy;              // une commande est en cours d'execution
        struct timespec ready_at; // instant ou elle se termine
        char *executing_cmd;    // commande en cours (possede la string)
        int incantation_owner_id; // 0 sinon id de l'initiateur qui le gele
        player_t *player;       // NULL pour le GUI
        bool must_disconnect;
        uint64_t stoped_tile;
        bool is_tile_stoped;
    } client_t;

    client_t *client_create(int fd);
    void client_destroy(client_t *client);
    int client_read(client_t *client);
    int client_extract_line(client_t *client, char *line, size_t line_size);
    int client_queue_send(client_t *client, const char *msg);
    int client_flush_send(client_t *client);
    int client_push_cmd(client_t *client, const char *cmd);
    char *client_pop_cmd(client_t *client);

#endif /* !CLIENT_H_ */
