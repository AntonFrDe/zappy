#ifndef COMMAND_H_
    #define COMMAND_H_
    #include "client.h"
    #include "network.h"

// -1 si commande inconnue, sinon cout en unites de temps (cf sujet)
    int command_cost(const char *cmd);

// a appeler une fois par tour de server_run: depile/demarre les
// commandes prêtes, termine celles dont le delai est ecoule
    void scheduler_tick(server_t *server);

#endif /* !COMMAND_H_ */
