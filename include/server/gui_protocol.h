#ifndef GUI_PROTOCOL_H_
    #define GUI_PROTOCOL_H_
    #include "network.h"

// diffuse msg a tous les clients GRAPHIC connectes
    void gui_broadcast(server_t *server, const char *msg);
// dump complet de l'etat (map, equipes, joueurs, oeufs) a un GRAPHIC qui
// vient de se connecter
    void gui_send_snapshot(server_t *server, client_t *gui);

    void gui_notify_new_player(server_t *server, client_t *client);
    void gui_notify_position(server_t *server, client_t *client);
    void gui_notify_level(server_t *server, client_t *client);
    void gui_notify_inventory(server_t *server, client_t *client);
    void gui_notify_expulsion(server_t *server, client_t *client);
    void gui_notify_broadcast(server_t *server, client_t *client,
        const char *text);
    void gui_notify_incantation_start(server_t *server, int64_t x, int64_t y,
        int level, const int *ids, int ids_nb);
    void gui_notify_incantation_end(server_t *server, int64_t x, int64_t y,
        int result);
    void gui_notify_egg_laying(server_t *server, client_t *client);
    void gui_notify_resource_take(server_t *server, client_t *client,
        int resource);
    void gui_notify_resource_drop(server_t *server, client_t *client,
        int resource);
    void gui_notify_death(server_t *server, client_t *client);
    void gui_notify_new_egg(server_t *server, int egg_id, int parent_id,
        int64_t x, int64_t y);
    void gui_notify_egg_hatch(server_t *server, int egg_id);
    void gui_notify_egg_death(server_t *server, int egg_id);

#endif /* !GUI_PROTOCOL_H_ */
