/*
** Unit tests for src/server/client.c (recv/send buffering, cmd queue)
** Framework: Criterion
** Note: fd = -1 is used throughout since these tests exercise pure
** buffer logic and never call client_read()/client_flush_send()
** (the only functions that touch the real fd).
*/

#include <criterion/criterion.h>
#include <server/server.h>
#include <string.h>

Test(client, create_and_destroy)
{
    client_t *client = client_create(-1);

    cr_assert_not_null(client);
    cr_assert_eq(client->fd, -1);
    cr_assert_eq(client->handshake_done, false);
    cr_assert_eq(client->is_graphic, false);
    cr_assert_eq(client->team_index, -1);
    cr_assert_eq(client->cmd_count, 0);
    client_destroy(client);
}

Test(client, extract_single_line)
{
    client_t *client = client_create(-1);
    char line[64];

    memcpy(client->recv_buf, "Forward\n", 8);
    client->recv_len = 8;
    cr_assert_eq(client_extract_line(client, line, sizeof(line)), 1);
    cr_assert_str_eq(line, "Forward");
    cr_assert_eq(client->recv_len, (size_t)0);
    cr_assert_eq(client_extract_line(client, line, sizeof(line)), 0);
    client_destroy(client);
}

Test(client, extract_multiple_lines_in_one_read)
{
    client_t *client = client_create(-1);
    char line[64];
    const char *data = "Forward\nRight\nLeft\n";

    memcpy(client->recv_buf, data, strlen(data));
    client->recv_len = strlen(data);
    cr_assert_eq(client_extract_line(client, line, sizeof(line)), 1);
    cr_assert_str_eq(line, "Forward");
    cr_assert_eq(client_extract_line(client, line, sizeof(line)), 1);
    cr_assert_str_eq(line, "Right");
    cr_assert_eq(client_extract_line(client, line, sizeof(line)), 1);
    cr_assert_str_eq(line, "Left");
    cr_assert_eq(client_extract_line(client, line, sizeof(line)), 0);
    client_destroy(client);
}

Test(client, extract_line_returns_zero_without_newline)
{
    client_t *client = client_create(-1);
    char line[64];

    memcpy(client->recv_buf, "Forwar", 6);
    client->recv_len = 6;
    cr_assert_eq(client_extract_line(client, line, sizeof(line)), 0);
    cr_assert_eq(client->recv_len, (size_t)6);
    client_destroy(client);
}

Test(client, extract_line_strips_trailing_cr)
{
    client_t *client = client_create(-1);
    char line[64];

    memcpy(client->recv_buf, "Forward\r\n", 9);
    client->recv_len = 9;
    cr_assert_eq(client_extract_line(client, line, sizeof(line)), 1);
    cr_assert_str_eq(line, "Forward");
    cr_assert_eq(client->recv_len, (size_t)0);
    client_destroy(client);
}

Test(client, extract_line_truncates_when_destination_too_small)
{
    client_t *client = client_create(-1);
    char line[5];
    const char *data = "0123456789\n";

    memcpy(client->recv_buf, data, strlen(data));
    client->recv_len = strlen(data);
    cr_assert_eq(client_extract_line(client, line, sizeof(line)), 1);
    cr_assert_str_eq(line, "0123");
    cr_assert_eq(client->recv_len, (size_t)0);
    client_destroy(client);
}

Test(client, push_and_pop_cmd_preserves_order)
{
    client_t *client = client_create(-1);
    char *cmd;

    cr_assert_eq(client_push_cmd(client, "Forward"), 0);
    cr_assert_eq(client_push_cmd(client, "Right"), 0);
    cmd = client_pop_cmd(client);
    cr_assert_str_eq(cmd, "Forward");
    free(cmd);
    cmd = client_pop_cmd(client);
    cr_assert_str_eq(cmd, "Right");
    free(cmd);
    cr_assert_null(client_pop_cmd(client));
    client_destroy(client);
}

Test(client, push_cmd_rejects_over_ten)
{
    client_t *client = client_create(-1);
    char *cmd;

    for (int i = 0; i < 10; i++)
        cr_assert_eq(client_push_cmd(client, "Forward"), 0);
    cr_assert_eq(client_push_cmd(client, "Forward"), -1);
    for (int i = 0; i < 10; i++) {
        cmd = client_pop_cmd(client);
        free(cmd);
    }
    client_destroy(client);
}

Test(client, queue_send_appends_to_buffer)
{
    client_t *client = client_create(-1);

    cr_assert_eq(client_queue_send(client, "WELCOME\n"), 0);
    cr_assert_eq(client_queue_send(client, "42\n"), 0);
    cr_assert_str_eq(client->send_buf, "WELCOME\n42\n");
    cr_assert_eq(client->send_len, strlen("WELCOME\n42\n"));
    client_destroy(client);
}
