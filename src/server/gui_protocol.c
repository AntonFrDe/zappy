#include "../../include/server/gui_protocol.h"
#include <stdio.h>

void gui_broadcast(server_t *server, const char *msg)
{
    for (int i = 1; i < server->nfds; i++)
        if (server->clients[i]->is_graphic)
            client_queue_send(server->clients[i], msg);
}

static void format_pin(char *msg, size_t size, client_t *client)
{
    player_t *p = client->player;
    int len = snprintf(msg, size, "pin #%d %lld %lld", client->id,
        (long long)p->x, (long long)p->y);

    for (int r = 0; r < RESOURCE_COUNT && len < (int)size; r++)
        len += snprintf(msg + len, size - (size_t)len, " %u", p->inventory[r]);
    snprintf(msg + len, size - (size_t)len, "\n");
}

void gui_notify_inventory(server_t *server, client_t *client)
{
    char msg[128];

    format_pin(msg, sizeof(msg), client);
    gui_broadcast(server, msg);
}

static void format_bct(char *msg, size_t size, map_t *map, int64_t x, int64_t y)
{
    tile_t *tile = map_get_tile(map, x, y);
    int len = snprintf(msg, size, "bct %lld %lld",
        (long long)x, (long long)y);

    for (int r = 0; r < RESOURCE_COUNT && len < (int)size; r++)
        len += snprintf(msg + len, size - (size_t)len, " %u",
            tile->quantities[r]);
    snprintf(msg + len, size - (size_t)len, "\n");
}

void gui_send_tile(client_t *gui, map_t *map, int64_t x, int64_t y)
{
    char msg[128];

    format_bct(msg, sizeof(msg), map, x, y);
    client_queue_send(gui, msg);
}

void gui_notify_tile(server_t *server, int64_t x, int64_t y)
{
    char msg[128];

    format_bct(msg, sizeof(msg), server->map, x, y);
    gui_broadcast(server, msg);
}

void gui_send_snapshot(server_t *server, client_t *gui)
{
    char msg[128];
    client_t *other;

    snprintf(msg, sizeof(msg), "msz %llu %llu\n",(unsigned long long)server->map->width, (unsigned long long)server->map->height);
    client_queue_send(gui, msg);
    snprintf(msg, sizeof(msg), "sgt %u\n", server->cfg->freq);
    client_queue_send(gui, msg);

    for (int i = 0; i < server->cfg->teams_nb; i++) {
        snprintf(msg, sizeof(msg), "tna %s\n", server->cfg->teams[i]);
        client_queue_send(gui, msg);
    }
    
    for (int i = 1; i < server->nfds; i++) {
        other = server->clients[i];
        if (other->player == NULL)
            continue;
        snprintf(msg, sizeof(msg), "pnw #%d %lld %lld %d %d %s\n", other->id,
            (long long)other->player->x, (long long)other->player->y,
            (int)other->player->orientation, other->player->level,
            server->cfg->teams[other->team_index]);
        client_queue_send(gui, msg);
        format_pin(msg, sizeof(msg), other);
        client_queue_send(gui, msg);
    }
    
    for (int i = 0; i < server->eggs.count; i++) {
        snprintf(msg, sizeof(msg), "enw #%d #%d %lld %lld\n",
            server->eggs.eggs[i].id, server->eggs.eggs[i].parent_id,
            (long long)server->eggs.eggs[i].x,
            (long long)server->eggs.eggs[i].y);
        client_queue_send(gui, msg);
    }

    gui->stoped_tile = 0;
    gui->is_tile_stoped = true;
}

void gui_pump_snapshot(server_t *server, client_t *gui)
{
    uint64_t total = server->map->width * server->map->height;

    while (gui->is_tile_stoped && gui->send_len - gui->send_sent < CLIENT_SEND_WATERMARK) {
        
        // Arrete quand le nb de tile en cours d'envoie > au nb de tile totaux
        if (gui->stoped_tile >= total) {
            gui->is_tile_stoped = false;
            return;
        }

        gui_send_tile(gui, server->map,
            (int64_t)(gui->stoped_tile % server->map->width),
            (int64_t)(gui->stoped_tile / server->map->width));
        
        gui->stoped_tile++;
    }
}

void gui_notify_new_player(server_t *server, client_t *client)
{
    char msg[128];

    snprintf(msg, sizeof(msg), "pnw #%d %lld %lld %d %d %s\n", client->id,
        (long long)client->player->x, (long long)client->player->y,
        (int)client->player->orientation, client->player->level,
        server->cfg->teams[client->team_index]);
    gui_broadcast(server, msg);
    gui_notify_inventory(server, client);
}

void gui_notify_position(server_t *server, client_t *client)
{
    char msg[64];

    snprintf(msg, sizeof(msg), "ppo #%d %lld %lld %d\n", client->id,
        (long long)client->player->x, (long long)client->player->y,
        (int)client->player->orientation);
    gui_broadcast(server, msg);
}

void gui_notify_level(server_t *server, client_t *client)
{
    char msg[32];

    snprintf(msg, sizeof(msg), "plv #%d %d\n", client->id,
        client->player->level);
    gui_broadcast(server, msg);
}

void gui_notify_expulsion(server_t *server, client_t *client)
{
    char msg[32];

    snprintf(msg, sizeof(msg), "pex #%d\n", client->id);
    gui_broadcast(server, msg);
}

void gui_notify_broadcast(server_t *server, client_t *client, const char *text)
{
    char msg[600];

    snprintf(msg, sizeof(msg), "pbc #%d %s\n", client->id, text);
    gui_broadcast(server, msg);
}

void gui_notify_incantation_start(server_t *server, int64_t x, int64_t y, int level, const int *ids, int ids_nb)
{
    char msg[256];
    int len = snprintf(msg, sizeof(msg), "pic %lld %lld %d",
        (long long)x, (long long)y, level);

    for (int i = 0; i < ids_nb && len < (int)sizeof(msg); i++)
        len += snprintf(msg + len, sizeof(msg) - (size_t)len, " #%d",
            ids[i]);
    snprintf(msg + len, sizeof(msg) - (size_t)len, "\n");
    gui_broadcast(server, msg);
}

void gui_notify_incantation_end(server_t *server, int64_t x, int64_t y, int result)
{
    char msg[64];

    snprintf(msg, sizeof(msg), "pie %lld %lld %d\n",
        (long long)x, (long long)y, result);
    gui_broadcast(server, msg);
}

void gui_notify_egg_laying(server_t *server, client_t *client)
{
    char msg[32];

    snprintf(msg, sizeof(msg), "pfk #%d\n", client->id);
    gui_broadcast(server, msg);
}

void gui_notify_resource_take(server_t *server, client_t *client, int resource)
{
    char msg[32];

    snprintf(msg, sizeof(msg), "pgt #%d %d\n", client->id, resource);
    gui_broadcast(server, msg);
}

void gui_notify_resource_drop(server_t *server, client_t *client, int resource)
{
    char msg[32];

    snprintf(msg, sizeof(msg), "pdr #%d %d\n", client->id, resource);
    gui_broadcast(server, msg);
}

void gui_notify_death(server_t *server, client_t *client)
{
    char msg[32];

    snprintf(msg, sizeof(msg), "pdi #%d\n", client->id);
    gui_broadcast(server, msg);
}

void gui_notify_new_egg(server_t *server, int egg_id, int parent_id, int64_t x, int64_t y)
{
    char msg[64];

    snprintf(msg, sizeof(msg), "enw #%d #%d %lld %lld\n", egg_id, parent_id,
        (long long)x, (long long)y);
    gui_broadcast(server, msg);
}

void gui_notify_egg_hatch(server_t *server, int egg_id)
{
    char msg[32];

    snprintf(msg, sizeof(msg), "ebo #%d\n", egg_id);
    gui_broadcast(server, msg);
}

void gui_notify_egg_death(server_t *server, int egg_id)
{
    char msg[32];

    snprintf(msg, sizeof(msg), "edi #%d\n", egg_id);
    gui_broadcast(server, msg);
}
