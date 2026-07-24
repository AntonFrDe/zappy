#ifndef VICTORY_H_
    #define VICTORY_H_
    #include "network.h"

// verifie si team_index a atteint 6 joueurs au niveau max ; si oui,
// diffuse "seg <team>\n" aux GUI et arrete proprement la partie
    void victory_check(server_t *server, int team_index);

#endif /* !VICTORY_H_ */
