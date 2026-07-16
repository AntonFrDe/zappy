/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   main.c                                           .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: anton <anton@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2026/07/07 17:25:21 by anton        #+#   ##    ##    #+#       */
/*   Updated: 2026/07/07 17:48:53 by anton       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

#include "../../include/server/server.h"

int main(int argc, char **argv)
{
    retour_t rt = parser(argc, argv);
    server_t server;

    if (rt.error) {
        fprintf(stderr, "%s\n", rt.error_msg);
        free_error(rt);
        return 84;
    }
    if (server_init(&server, rt.value) < 0) {
        fprintf(stderr, "Failed to initialize the server\n");
        flags_destroy(rt.value);
        return 84;
    }
    server_run(&server);
    server_destroy(&server);
    flags_destroy(rt.value);
    return 0;
}
