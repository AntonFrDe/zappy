#ifndef GUI_REQUESTS_H_
    #define GUI_REQUESTS_H_
    #include "network.h"

// traite une ligne envoyee par un client GRAPHIC ; reponse synchrone,
// pas de cout en unite de temps (contrairement aux commandes IA)
    void gui_dispatch_request(server_t *server, client_t *client,
        const char *line);

#endif /* !GUI_REQUESTS_H_ */
