#ifndef COMMAND_H_
    #define COMMAND_H_
    #include "client.h"
    #include "network.h"
    #include <time.h>

// -1 si commande inconnue, sinon cout en unites de temps (cf sujet)
    int command_cost(const char *cmd);

// a appeler une fois par tour de server_run: depile/demarre les
// commandes prêtes, termine celles dont le delai est ecoule
    void scheduler_tick(server_t *server);

// instant = maintenant + cost/freq secondes (unites de temps du sujet)
    struct timespec compute_ready_at(int cost, uint16_t freq);

#endif /* !COMMAND_H_ */
