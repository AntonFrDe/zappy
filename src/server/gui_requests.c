#include "../../include/server/gui_requests.h"
#include "../../include/server/gui_protocol.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef void (*gui_handler_t)(server_t *server, client_t *gui,
    const char *arg);

typedef struct {
    const char *name;
    gui_handler_t handler;
} gui_request_t;

// mm decoupage que command.c mais garde en local, une requete GUI coute rien
static void split_verb(const char *line, char *verb, size_t verb_size)
{
    size_t i = 0;

    while (line[i] != '\0' && line[i] != ' ' && i + 1 < verb_size) {
        verb[i] = line[i];
        i++;
    }
    verb[i] = '\0';
}

static const char *split_arg(const char *line)
{
    const char *sp = strchr(line, ' ');

    return sp != NULL ? sp + 1 : "";
}

// "3 4" -> x=3 y=4 ; check aussi le hors carte sinon map_get_tile
// modulo en silence au lieu de renvoyer sbp
static bool parse_coords(server_t *server, const char *arg, int64_t *x,
    int64_t *y)
{
    char *end;
    long long vx;
    long long vy;

    errno = 0;
    vx = strtoll(arg, &end, 10);
    if (end == arg || errno == ERANGE || *end != ' ')
        return false;
    arg = end;
    vy = strtoll(arg, &end, 10);
    if (end == arg || errno == ERANGE)
        return false;
    while (*end == ' ')
        end++;
    if (*end != '\0')
        return false;
    if (vx < 0 || vy < 0 || (uint64_t)vx >= server->map->width
        || (uint64_t)vy >= server->map->height)
        return false;
    *x = vx;
    *y = vy;
    return true;
}

// "#3" ou "3" ; NULL si id inconnu ou si c'est un GUI (pas de player)
static client_t *find_player(server_t *server, const char *arg)
{
    client_t *target;
    char *end;
    long id;

    if (*arg == '#')
        arg++;
    errno = 0;
    id = strtol(arg, &end, 10);
    if (end == arg || errno == ERANGE)
        return NULL;
    while (*end == ' ')
        end++;
    if (*end != '\0')
        return NULL;
    target = find_client_by_id(server, (int)id);
    if (target == NULL || target->player == NULL)
        return NULL;
    return target;
}

static bool parse_freq(const char *arg, uint16_t *out)
{
    char *end;
    long long value;

    if (arg[0] == '\0')
        return false;
    errno = 0;
    value = strtoll(arg, &end, 10);
    if (end == arg || errno == ERANGE)
        return false;
    while (*end == ' ')
        end++;
    if (*end != '\0' || value < 1 || value > UINT16_MAX)
        return false;
    *out = (uint16_t)value;
    return true;
}

static void request_msz(server_t *server, client_t *gui, const char *arg)
{
    char msg[64];

    (void)arg;
    snprintf(msg, sizeof(msg), "msz %llu %llu\n",
        (unsigned long long)server->map->width,
        (unsigned long long)server->map->height);
    client_queue_send(gui, msg);
}

static void request_sgt(server_t *server, client_t *gui, const char *arg)
{
    char msg[32];

    (void)arg;
    snprintf(msg, sizeof(msg), "sgt %u\n", server->cfg->freq);
    client_queue_send(gui, msg);
}

static void request_tna(server_t *server, client_t *gui, const char *arg)
{
    char msg[64];

    (void)arg;
    for (int i = 0; i < server->cfg->teams_nb; i++) {
        snprintf(msg, sizeof(msg), "tna %s\n", server->cfg->teams[i]);
        client_queue_send(gui, msg);
    }
}

static void request_bct(server_t *server, client_t *gui, const char *arg)
{
    int64_t x;
    int64_t y;

    if (!parse_coords(server, arg, &x, &y)) {
        client_queue_send(gui, "sbp\n");
        return;
    }
    gui_send_tile(gui, server->map, x, y);
}

static void request_mct(server_t *server, client_t *gui, const char *arg)
{
    (void)arg;
    for (uint64_t y = 0; y < server->map->height; y++)
        for (uint64_t x = 0; x < server->map->width; x++)
            gui_send_tile(gui, server->map, (int64_t)x, (int64_t)y);
}

static void request_ppo(server_t *server, client_t *gui, const char *arg)
{
    client_t *target = find_player(server, arg);
    char msg[64];

    if (target == NULL) {
        client_queue_send(gui, "sbp\n");
        return;
    }
    snprintf(msg, sizeof(msg), "ppo #%d %lld %lld %d\n", target->id,
        (long long)target->player->x, (long long)target->player->y,
        (int)target->player->orientation);
    client_queue_send(gui, msg);
}

static void request_plv(server_t *server, client_t *gui, const char *arg)
{
    client_t *target = find_player(server, arg);
    char msg[32];

    if (target == NULL) {
        client_queue_send(gui, "sbp\n");
        return;
    }
    snprintf(msg, sizeof(msg), "plv #%d %d\n", target->id,
        target->player->level);
    client_queue_send(gui, msg);
}

static void request_pin(server_t *server, client_t *gui, const char *arg)
{
    client_t *target = find_player(server, arg);
    char msg[128];
    int len;

    if (target == NULL) {
        client_queue_send(gui, "sbp\n");
        return;
    }
    len = snprintf(msg, sizeof(msg), "pin #%d %lld %lld", target->id,
        (long long)target->player->x, (long long)target->player->y);
    for (int r = 0; r < RESOURCE_COUNT && len < (int)sizeof(msg); r++)
        len += snprintf(msg + len, sizeof(msg) - (size_t)len, " %u",
            target->player->inventory[r]);
    snprintf(msg + len, sizeof(msg) - (size_t)len, "\n");
    client_queue_send(gui, msg);
}

// change la freq de toute la partie donc a broadcaster, pas juste au demandeur
static void request_sst(server_t *server, client_t *gui, const char *arg)
{
    char msg[32];
    uint16_t freq;

    if (!parse_freq(arg, &freq)) {
        client_queue_send(gui, "sbp\n");
        return;
    }
    server->cfg->freq = freq;
    snprintf(msg, sizeof(msg), "sst %u\n", server->cfg->freq);
    gui_broadcast(server, msg);
}

static const gui_request_t REQUESTS[] = {
    { "msz", request_msz },
    { "bct", request_bct },
    { "mct", request_mct },
    { "tna", request_tna },
    { "ppo", request_ppo },
    { "plv", request_plv },
    { "pin", request_pin },
    { "sgt", request_sgt },
    { "sst", request_sst },
};
#define REQUESTS_NB (sizeof(REQUESTS) / sizeof(REQUESTS[0]))

void gui_dispatch_request(server_t *server, client_t *client,
    const char *line)
{
    char verb[16];

    split_verb(line, verb, sizeof(verb));
    for (size_t i = 0; i < REQUESTS_NB; i++)
        if (strcmp(REQUESTS[i].name, verb) == 0) {
            REQUESTS[i].handler(server, client, split_arg(line));
            return;
        }
    client_queue_send(client, "suc\n");
}
