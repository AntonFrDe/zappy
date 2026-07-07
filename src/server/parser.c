/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   parser.c                                         .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: anton <anton@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2026/07/07 17:45:47 by anton        #+#   ##    ##    #+#       */
/*   Updated: 2026/07/07 18:11:13 by anton       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */


#include <include/server/server.h>

static void print_usage(void)
{
    printf("USAGE: ./zappy_server -p port -x width -y height "
        "-n name1 name2 ... -c clientsNb -f freq\n");
}

static bool has_help_flag(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0)
            return true;
    }
    return false;
}

retour_t parser(int argc, char **argv)
{
    int opt;
    flags_t flags = {0};
    retour_t rt = {0};
    bool has_port = false;
    bool has_width = false;
    bool has_height = false;
    bool has_teams = false;
    bool has_clients = false;

    if (has_help_flag(argc, argv)) {
        print_usage();
        exit(0);
    }
    flags.freq = 100;
    flags.teams = malloc(sizeof(char *) * (size_t)argc);
    if (flags.teams == NULL) {
        snprintf(rt.error_msg, sizeof(rt.error_msg), "Memory allocation failed");
        rt.error = true;
        return rt;
    }
    while ((opt = getopt(argc, argv, "p:x:y:n:c:f:")) != -1) {
        switch (opt) {

        // PORT
        case 'p': {
            int p = atoi(optarg);
            if (p < 1 || p > 65535) {
                snprintf(rt.error_msg, sizeof(rt.error_msg), "Port given value isn't a valid port");
                rt.error = true;
                return rt;
            }
            flags.port = p;
            has_port = true;
            break;
        }
        // MAP WIDTH
        case 'x': {
            int x = atoi(optarg);
            if (x < 1) {
                snprintf(rt.error_msg, sizeof(rt.error_msg), "Map width given value is not a unsigned integer");
                rt.error = true;
                return rt;
            }
            flags.width = (uint64_t)x;
            has_width = true;
            break;
        }
        // MAP HEIGHT
        case 'y': {
            int y = atoi(optarg);
            if (y < 1) {
                snprintf(rt.error_msg, sizeof(rt.error_msg), "Map height given value is not a unsigned integer");
                rt.error = true;
                return rt;
            }
            flags.height = (uint64_t)y;
            has_height = true;
            break;
        }
        // TEAM NAMES
        case 'n': {
            if (strcmp(optarg, "GRAPHIC") == 0) {
                snprintf(rt.error_msg, sizeof(rt.error_msg), "Team name 'GRAPHIC' is reserved for the GUI");
                rt.error = true;
                return rt;
            }
            flags.teams[flags.teams_nb++] = optarg;
            
            while (optind < argc && argv[optind][0] != '-') {
                if (strcmp(argv[optind], "GRAPHIC") == 0) {
                    snprintf(rt.error_msg, sizeof(rt.error_msg), "Team name 'GRAPHIC' is reserved for the GUI");
                    rt.error = true;
                    return rt;
                }
                flags.teams[flags.teams_nb++] = argv[optind];
                optind++;
            }
            has_teams = true;
            break;
        }
        // CLIENTS PER TEAM
        case 'c': {
            int c = atoi(optarg);
            if (c < 1 || c > UINT16_MAX) {
                snprintf(rt.error_msg, sizeof(rt.error_msg), "clientsNb given value is not a valid unsigned integer");
                rt.error = true;
                return rt;
            }
            flags.clients_nb = (uint16_t)c;
            has_clients = true;
            break;
        }
        // FREQUENCY (optionnel, defaut 100 deja pose avant la boucle)
        case 'f': {
            int f = atoi(optarg);
            if (f < 1 || f > UINT16_MAX) {
                snprintf(rt.error_msg, sizeof(rt.error_msg), "freq given value is not a valid unsigned integer");
                rt.error = true;
                return rt;
            }
            flags.freq = (uint16_t)f;
            break;
        }
        default:
            snprintf(rt.error_msg, sizeof(rt.error_msg), "Unknown option or missing value for a flag");
            rt.error = true;
            return rt;
        }
    }
    if (!has_port || !has_width || !has_height || !has_teams || !has_clients) {
        snprintf(rt.error_msg, sizeof(rt.error_msg), "Missing mandatory argument (-p, -x, -y, -n or -c)");
        rt.error = true;
        return rt;
    }
    flags_t *result = malloc(sizeof(flags_t));
    if (result == NULL) {
        snprintf(rt.error_msg, sizeof(rt.error_msg), "Memory allocation failed");
        rt.error = true;
        return rt;
    }
    *result = flags;
    rt.error = false;
    rt.value = result;
    return rt;
}
