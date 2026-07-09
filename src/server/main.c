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
#include <server/server.h>

int main(int argc, char **argv)
{
    retour_t rt = parser(argc, argv);

    if (rt.error) {
        fprintf(stderr, "%s\n", rt.error_msg);
        free_error(rt);
        free_rt(rt);
        return 84;
    }
    return 0;
}
