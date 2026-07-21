#include "../../include/server/client.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

client_t *client_create(int fd)
{
    client_t *client = malloc(sizeof(client_t));

    if (client == NULL)
        return NULL;
    memset(client, 0, sizeof(client_t));
    client->fd = fd;
    client->team_index = -1;
    return client;
}

void client_destroy(client_t *client)
{
    if (client == NULL)
        return;
    if (client->fd >= 0)
        close(client->fd);
    free(client->send_buf);
    for (int i = 0; i < client->cmd_count; i++)
        free(client->cmd_queue[i]);
    free(client->executing_cmd);
    player_destroy(client->player);
    free(client);
}

int client_read(client_t *client)
{
    ssize_t n;
    size_t space = CLIENT_RECV_BUF_SIZE - client->recv_len - 1;

    if (space == 0)
        return -1;
    n = read(client->fd, client->recv_buf + client->recv_len, space);
    if (n <= 0)
        return (int)n;
    client->recv_len += (size_t)n;
    client->recv_buf[client->recv_len] = '\0';
    return (int)n;
}

int client_extract_line(client_t *client, char *line, size_t line_size)
{
    char *nl = memchr(client->recv_buf, '\n', client->recv_len);
    size_t cut;

    if (nl == NULL)
        return 0;
    cut = (size_t)(nl - client->recv_buf);
    if (cut > 0 && client->recv_buf[cut - 1] == '\r')
        cut--;
    memcpy(line, client->recv_buf, cut < line_size ? cut : line_size - 1);
    line[cut < line_size ? cut : line_size - 1] = '\0';
    cut = (size_t)(nl - client->recv_buf) + 1;
    memmove(client->recv_buf, client->recv_buf + cut, client->recv_len - cut);
    client->recv_len -= cut;
    return 1;
}

int client_queue_send(client_t *client, const char *msg)
{
    size_t msg_len = strlen(msg);
    char *bigger = realloc(client->send_buf, client->send_len + msg_len + 1);

    if (bigger == NULL)
        return -1;
    client->send_buf = bigger;
    memcpy(client->send_buf + client->send_len, msg, msg_len + 1);
    client->send_len += msg_len;
    return 0;
}

int client_flush_send(client_t *client)
{
    ssize_t n;
    size_t remaining = client->send_len - client->send_sent;

    if (remaining == 0)
        return 0;
    n = write(client->fd, client->send_buf + client->send_sent, remaining);
    if (n <= 0)
        return -1;
    client->send_sent += (size_t)n;
    if (client->send_sent == client->send_len) {
        free(client->send_buf);
        client->send_buf = NULL;
        client->send_len = 0;
        client->send_sent = 0;
    }
    return 0;
}

int client_push_cmd(client_t *client, const char *cmd)
{
    if (client->cmd_count >= CLIENT_MAX_QUEUED_CMDS)
        return -1;
    client->cmd_queue[client->cmd_count] = strdup(cmd);
    if (client->cmd_queue[client->cmd_count] == NULL)
        return -1;
    client->cmd_count++;
    return 0;
}

char *client_pop_cmd(client_t *client)
{
    char *cmd;

    if (client->cmd_count == 0)
        return NULL;
    cmd = client->cmd_queue[0];
    for (int i = 1; i < client->cmd_count; i++)
        client->cmd_queue[i - 1] = client->cmd_queue[i];
    client->cmd_count--;
    return cmd;
}
