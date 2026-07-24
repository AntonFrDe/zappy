#include "../../include/server/victory.h"
#include "../../include/server/gui_protocol.h"
#include <stdio.h>

#define WINNING_LEVEL 8
#define WINNING_PLAYERS 6

static int count_players_at_max_level(server_t *server, int team_index)
{
    int count = 0;
    client_t *other;

    for (int i = 1; i < server->nfds; i++) {
        other = server->clients[i];
        if (other->player == NULL || other->team_index != team_index)
            continue;
        if (other->player->level >= WINNING_LEVEL)
            count++;
    }
    return count;
}

void victory_check(server_t *server, int team_index)
{
    char msg[128];

    if (server->game_over)
        return;
    if (count_players_at_max_level(server, team_index) < WINNING_PLAYERS)
        return;
    server->game_over = true;
    snprintf(msg, sizeof(msg), "seg %s\n", server->cfg->teams[team_index]);
    gui_broadcast(server, msg);
}
