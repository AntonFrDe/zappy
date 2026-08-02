/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anton <anton@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/07 17:45:47 by anton             #+#    #+#             */
/*   Updated: 2026/08/02 13:11:13 by anton            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "../../include/server/server.h"
#include <errno.h>

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

//check long number
static bool parse_long_nb(const char *str, long min, long max, long *out)
{
    char *end;
    long val;

    if (str == NULL || *str == '\0')
        return false;
    errno = 0;
    val = strtol(str, &end, 10);
    if (*end != '\0' || errno == ERANGE || val < min || val > max)
        return false;
    *out = val;
    return true;
}

// la mm en unsigned
static uint8_t parse_u64_nb(const char *str, uint64_t max, uint64_t *out)
{
    char *end;
    unsigned long long val;

    if (str == NULL || *str == '\0' || str[0] == '-')
        return -1;
    errno = 0;
    val = strtoull(str, &end, 10);

    if (*end != '\0' || errno == ERANGE || val < 1 || val > max)
        return -2;
        
    *out = (uint64_t)val;
    return 1;
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
    long lval;

    if (has_help_flag(argc, argv)) {
        print_usage();
        exit(0);
    }
    flags.freq = 100;
    flags.teams = malloc(sizeof(char *) * (size_t)argc);
    if (flags.teams == NULL)
        return fail("Memory allocation failed", NULL);
    while ((opt = getopt(argc, argv, "p:x:y:n:c:f:")) != -1) {
        switch (opt) {

        // PORT
        case 'p': {
            if (!parse_long_nb(optarg, 1, 65535, &lval))
                return fail("Port given value isn't a valid port", flags.teams);
            flags.port = (int)lval;
            has_port = true;
            break;
        }
        // MAP WIDTH
        case 'x': {
            switch (parse_u64_nb(optarg, MAP_MAX_WIDTH, &flags.width)) {
                case -1: return fail("Map width given value is not a unsigned integer", flags.teams);
                case -2: return fail(strcat("Map width given value is greater than", atoi(MAP_MAX_WIDTH)), flags.teams);
                default: has_width = true;
            }
            break;
        }
        // MAP HEIGHT
        case 'y': {
            switch (parse_u64_nb(optarg, MAP_MAX_HEIGHT, &flags.height)) {
                case -1: return fail("Map height given value is not a unsigned integer", flags.teams);
                case -2: return fail(strcat("Map height given value is greater than", atoi(MAP_MAX_HEIGHT)), flags.teams);
                default: has_height = true;
            }
            break;
        }
        // TEAM NAMES
        case 'n': {
            if (strcmp(optarg, "GRAPHIC") == 0)
                return fail("Team name 'GRAPHIC' is reserved for the GUI", flags.teams);
            flags.teams[flags.teams_nb++] = optarg;

            while (optind < argc && argv[optind][0] != '-') {
                if (strcmp(argv[optind], "GRAPHIC") == 0)
                    return fail("Team name 'GRAPHIC' is reserved for the GUI", flags.teams);
                flags.teams[flags.teams_nb++] = argv[optind];
                optind++;
            }
            has_teams = true;
            break;
        }
        // CLIENTS PER TEAM
        case 'c': {
            if (!parse_long_nb(optarg, 1, UINT16_MAX, &lval))
                return fail("clientsNb given value is not a valid unsigned integer", flags.teams);
            flags.clients_nb = (uint16_t)lval;
            has_clients = true;
            break;
        }
        // FREQUENCY
        case 'f': {
            if (!parse_long_nb(optarg, 1, UINT16_MAX, &lval))
                return fail("freq given value is not a valid unsigned integer", flags.teams);
            flags.freq = (uint16_t)lval;
            break;
        }
        default:
            return fail("Unknown option or missing value for a flag", flags.teams);
        }
    }
    if (!has_port || !has_width || !has_height || !has_teams || !has_clients)
        return fail("Missing mandatory argument (-p, -x, -y, -n or -c)", flags.teams);

    flags_t *result = malloc(sizeof(flags_t));
    if (result == NULL)
        return fail("Memory allocation failed", flags.teams);
    *result = flags;
    rt.error = false;
    rt.value = result;
    return rt;
}

void flags_destroy(flags_t *flags)
{
    if (flags == NULL)
        return;
    free(flags->teams);
    free(flags);
}
