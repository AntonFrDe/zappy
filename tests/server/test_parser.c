/*
** Unit tests for src/server/parser.c (retour_t parser(int argc, char **argv))
** Framework: Criterion
*/

#include <criterion/criterion.h>
#include <criterion/redirect.h>
#include <server/server.h>

retour_t parser(int argc, char **argv);

static void free_flags(retour_t rt)
{
    flags_destroy(rt.value);
}

Test(parser, valid_full_arguments)
{
    char *argv[] = {
        "zappy_server", "-p", "4242", "-x", "10", "-y", "10",
        "-n", "team1", "team2", "-c", "3", "-f", "50"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    retour_t rt;
    flags_t *flags;

    optind = 1;
    rt = parser(argc, argv);
    cr_assert_eq(rt.error, false);
    flags = rt.value;
    cr_assert_not_null(flags);
    cr_assert_eq(flags->port, 4242);
    cr_assert_eq(flags->width, (uint64_t)10);
    cr_assert_eq(flags->height, (uint64_t)10);
    cr_assert_eq(flags->clients_nb, 3);
    cr_assert_eq(flags->freq, 50);
    cr_assert_eq(flags->teams_nb, 2);
    cr_assert_str_eq(flags->teams[0], "team1");
    cr_assert_str_eq(flags->teams[1], "team2");
    free_flags(rt);
}

Test(parser, default_freq_is_100_when_not_given)
{
    char *argv[] = {
        "zappy_server", "-p", "4242", "-x", "10", "-y", "10",
        "-n", "team1", "-c", "3"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    retour_t rt;
    flags_t *flags;

    optind = 1;
    rt = parser(argc, argv);
    cr_assert_eq(rt.error, false);
    flags = rt.value;
    cr_assert_eq(flags->freq, 100);
    free_flags(rt);
}

Test(parser, missing_port_fails)
{
    char *argv[] = {
        "zappy_server", "-x", "10", "-y", "10", "-n", "team1", "-c", "3"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    retour_t rt;

    optind = 1;
    rt = parser(argc, argv);
    cr_assert_eq(rt.error, true);
}

Test(parser, missing_width_fails)
{
    char *argv[] = {
        "zappy_server", "-p", "4242", "-y", "10", "-n", "team1", "-c", "3"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    retour_t rt;

    optind = 1;
    rt = parser(argc, argv);
    cr_assert_eq(rt.error, true);
}

Test(parser, missing_height_fails)
{
    char *argv[] = {
        "zappy_server", "-p", "4242", "-x", "10", "-n", "team1", "-c", "3"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    retour_t rt;

    optind = 1;
    rt = parser(argc, argv);
    cr_assert_eq(rt.error, true);
}

Test(parser, missing_teams_fails)
{
    char *argv[] = {
        "zappy_server", "-p", "4242", "-x", "10", "-y", "10", "-c", "3"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    retour_t rt;

    optind = 1;
    rt = parser(argc, argv);
    cr_assert_eq(rt.error, true);
}

Test(parser, missing_clients_fails)
{
    char *argv[] = {
        "zappy_server", "-p", "4242", "-x", "10", "-y", "10", "-n", "team1"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    retour_t rt;

    optind = 1;
    rt = parser(argc, argv);
    cr_assert_eq(rt.error, true);
}

Test(parser, port_zero_fails)
{
    char *argv[] = {
        "zappy_server", "-p", "0", "-x", "10", "-y", "10",
        "-n", "team1", "-c", "3"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    retour_t rt;

    optind = 1;
    rt = parser(argc, argv);
    cr_assert_eq(rt.error, true);
}

Test(parser, port_above_65535_fails)
{
    char *argv[] = {
        "zappy_server", "-p", "65536", "-x", "10", "-y", "10",
        "-n", "team1", "-c", "3"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    retour_t rt;

    optind = 1;
    rt = parser(argc, argv);
    cr_assert_eq(rt.error, true);
}

Test(parser, width_zero_fails)
{
    char *argv[] = {
        "zappy_server", "-p", "4242", "-x", "0", "-y", "10",
        "-n", "team1", "-c", "3"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    retour_t rt;

    optind = 1;
    rt = parser(argc, argv);
    cr_assert_eq(rt.error, true);
}

Test(parser, height_zero_fails)
{
    char *argv[] = {
        "zappy_server", "-p", "4242", "-x", "10", "-y", "0",
        "-n", "team1", "-c", "3"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    retour_t rt;

    optind = 1;
    rt = parser(argc, argv);
    cr_assert_eq(rt.error, true);
}

Test(parser, clients_zero_fails)
{
    char *argv[] = {
        "zappy_server", "-p", "4242", "-x", "10", "-y", "10",
        "-n", "team1", "-c", "0"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    retour_t rt;

    optind = 1;
    rt = parser(argc, argv);
    cr_assert_eq(rt.error, true);
}

Test(parser, clients_above_uint16_max_fails)
{
    char *argv[] = {
        "zappy_server", "-p", "4242", "-x", "10", "-y", "10",
        "-n", "team1", "-c", "70000"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    retour_t rt;

    optind = 1;
    rt = parser(argc, argv);
    cr_assert_eq(rt.error, true);
}

Test(parser, freq_zero_fails)
{
    char *argv[] = {
        "zappy_server", "-p", "4242", "-x", "10", "-y", "10",
        "-n", "team1", "-c", "3", "-f", "0"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    retour_t rt;

    optind = 1;
    rt = parser(argc, argv);
    cr_assert_eq(rt.error, true);
}

Test(parser, team_name_graphic_as_first_fails)
{
    char *argv[] = {
        "zappy_server", "-p", "4242", "-x", "10", "-y", "10",
        "-n", "GRAPHIC", "-c", "3"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    retour_t rt;

    optind = 1;
    rt = parser(argc, argv);
    cr_assert_eq(rt.error, true);
}

Test(parser, team_name_graphic_as_second_fails)
{
    char *argv[] = {
        "zappy_server", "-p", "4242", "-x", "10", "-y", "10",
        "-n", "team1", "GRAPHIC", "-c", "3"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    retour_t rt;

    optind = 1;
    rt = parser(argc, argv);
    cr_assert_eq(rt.error, true);
}

Test(parser, unknown_option_fails)
{
    char *argv[] = {
        "zappy_server", "-p", "4242", "-x", "10", "-y", "10",
        "-n", "team1", "-c", "3", "-z", "1"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    retour_t rt;

    optind = 1;
    rt = parser(argc, argv);
    cr_assert_eq(rt.error, true);
}

Test(parser, help_flag_exits_zero, .exit_code = 0, .init = cr_redirect_stdout)
{
    char *argv[] = { "zappy_server", "--help" };
    int argc = sizeof(argv) / sizeof(argv[0]);

    optind = 1;
    parser(argc, argv);
}

/* ---- Valeurs limites valides ---- */

Test(parser, port_min_value_1_is_valid)
{
    char *argv[] = {
        "zappy_server", "-p", "1", "-x", "10", "-y", "10",
        "-n", "team1", "-c", "3"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    retour_t rt;

    optind = 1;
    rt = parser(argc, argv);
    cr_assert_eq(rt.error, false);
    cr_assert_eq(((flags_t *)rt.value)->port, 1);
    free_flags(rt);
}

Test(parser, port_max_value_65535_is_valid)
{
    char *argv[] = {
        "zappy_server", "-p", "65535", "-x", "10", "-y", "10",
        "-n", "team1", "-c", "3"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    retour_t rt;

    optind = 1;
    rt = parser(argc, argv);
    cr_assert_eq(rt.error, false);
    cr_assert_eq(((flags_t *)rt.value)->port, 65535);
    free_flags(rt);
}

Test(parser, clients_nb_max_uint16_is_valid)
{
    char *argv[] = {
        "zappy_server", "-p", "4242", "-x", "10", "-y", "10",
        "-n", "team1", "-c", "65535"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    retour_t rt;

    optind = 1;
    rt = parser(argc, argv);
    cr_assert_eq(rt.error, false);
    cr_assert_eq(((flags_t *)rt.value)->clients_nb, (uint16_t)65535);
    free_flags(rt);
}

Test(parser, freq_max_uint16_is_valid)
{
    char *argv[] = {
        "zappy_server", "-p", "4242", "-x", "10", "-y", "10",
        "-n", "team1", "-c", "3", "-f", "65535"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    retour_t rt;

    optind = 1;
    rt = parser(argc, argv);
    cr_assert_eq(rt.error, false);
    cr_assert_eq(((flags_t *)rt.value)->freq, (uint16_t)65535);
    free_flags(rt);
}

/* ---- Flags repetes / valeur manquante ---- */

Test(parser, repeated_port_flag_last_value_wins)
{
    char *argv[] = {
        "zappy_server", "-p", "4242", "-x", "10", "-y", "10",
        "-n", "team1", "-c", "3", "-p", "8080"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    retour_t rt;

    optind = 1;
    rt = parser(argc, argv);
    cr_assert_eq(rt.error, false);
    cr_assert_eq(((flags_t *)rt.value)->port, 8080);
    free_flags(rt);
}

Test(parser, repeated_clients_flag_last_value_wins)
{
    char *argv[] = {
        "zappy_server", "-p", "4242", "-x", "10", "-y", "10",
        "-n", "team1", "-c", "3", "-c", "5"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    retour_t rt;

    optind = 1;
    rt = parser(argc, argv);
    cr_assert_eq(rt.error, false);
    cr_assert_eq(((flags_t *)rt.value)->clients_nb, (uint16_t)5);
    free_flags(rt);
}

Test(parser, repeated_teams_flag_accumulates_instead_of_overriding)
{
    char *argv[] = {
        "zappy_server", "-p", "4242", "-x", "10", "-y", "10",
        "-n", "team1", "-c", "3", "-n", "team2"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    retour_t rt;
    flags_t *flags;

    optind = 1;
    rt = parser(argc, argv);
    cr_assert_eq(rt.error, false);
    flags = rt.value;
    cr_assert_eq(flags->teams_nb, 2);
    cr_assert_str_eq(flags->teams[0], "team1");
    cr_assert_str_eq(flags->teams[1], "team2");
    free_flags(rt);
}

Test(parser, missing_value_at_end_for_port_fails, .init = cr_redirect_stderr)
{
    char *argv[] = {
        "zappy_server", "-x", "10", "-y", "10",
        "-n", "team1", "-c", "3", "-p"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    retour_t rt;

    optind = 1;
    rt = parser(argc, argv);
    cr_assert_eq(rt.error, true);
}

Test(parser, missing_value_at_end_for_freq_fails, .init = cr_redirect_stderr)
{
    char *argv[] = {
        "zappy_server", "-p", "4242", "-x", "10", "-y", "10",
        "-n", "team1", "-c", "3", "-f"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    retour_t rt;

    optind = 1;
    rt = parser(argc, argv);
    cr_assert_eq(rt.error, true);
}

Test(parser, missing_value_at_end_for_width_fails, .init = cr_redirect_stderr)
{
    char *argv[] = {
        "zappy_server", "-p", "4242", "-y", "10",
        "-n", "team1", "-c", "3", "-x"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    retour_t rt;

    optind = 1;
    rt = parser(argc, argv);
    cr_assert_eq(rt.error, true);
}

Test(parser, missing_value_at_end_for_teams_fails, .init = cr_redirect_stderr)
{
    char *argv[] = {
        "zappy_server", "-p", "4242", "-x", "10", "-y", "10",
        "-c", "3", "-n"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    retour_t rt;

    optind = 1;
    rt = parser(argc, argv);
    cr_assert_eq(rt.error, true);
}

/* ---- Aucun argument / valeurs negatives ---- */

Test(parser, no_arguments_at_all_fails)
{
    char *argv[] = { "zappy_server" };
    int argc = sizeof(argv) / sizeof(argv[0]);
    retour_t rt;

    optind = 1;
    rt = parser(argc, argv);
    cr_assert_eq(rt.error, true);
}

Test(parser, port_negative_fails)
{
    char *argv[] = {
        "zappy_server", "-p", "-1", "-x", "10", "-y", "10",
        "-n", "team1", "-c", "3"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    retour_t rt;

    optind = 1;
    rt = parser(argc, argv);
    cr_assert_eq(rt.error, true);
}

Test(parser, width_negative_fails)
{
    char *argv[] = {
        "zappy_server", "-p", "4242", "-x", "-5", "-y", "10",
        "-n", "team1", "-c", "3"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    retour_t rt;

    optind = 1;
    rt = parser(argc, argv);
    cr_assert_eq(rt.error, true);
}

Test(parser, height_negative_fails)
{
    char *argv[] = {
        "zappy_server", "-p", "4242", "-x", "10", "-y", "-3",
        "-n", "team1", "-c", "3"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    retour_t rt;

    optind = 1;
    rt = parser(argc, argv);
    cr_assert_eq(rt.error, true);
}

Test(parser, clients_negative_fails)
{
    char *argv[] = {
        "zappy_server", "-p", "4242", "-x", "10", "-y", "10",
        "-n", "team1", "-c", "-1"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    retour_t rt;

    optind = 1;
    rt = parser(argc, argv);
    cr_assert_eq(rt.error, true);
}

Test(parser, freq_negative_fails)
{
    char *argv[] = {
        "zappy_server", "-p", "4242", "-x", "10", "-y", "10",
        "-n", "team1", "-c", "3", "-f", "-5"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    retour_t rt;

    optind = 1;
    rt = parser(argc, argv);
    cr_assert_eq(rt.error, true);
}
