/*
** Unit tests for src/server/command.c (command_cost lookup)
** Framework: Criterion
*/

#include <criterion/criterion.h>
#include <server/server.h>

Test(command, known_commands_return_correct_cost)
{
    cr_assert_eq(command_cost("Forward"), 7);
    cr_assert_eq(command_cost("Right"), 7);
    cr_assert_eq(command_cost("Left"), 7);
    cr_assert_eq(command_cost("Look"), 7);
    cr_assert_eq(command_cost("Inventory"), 1);
    cr_assert_eq(command_cost("Broadcast"), 7);
    cr_assert_eq(command_cost("Fork"), 42);
    cr_assert_eq(command_cost("Eject"), 7);
    cr_assert_eq(command_cost("Take"), 7);
    cr_assert_eq(command_cost("Set"), 7);
    cr_assert_eq(command_cost("Incantation"), 300);
}

Test(command, commands_with_arguments_use_only_the_verb)
{
    cr_assert_eq(command_cost("Take linemate"), 7);
    cr_assert_eq(command_cost("Set thystame"), 7);
    cr_assert_eq(command_cost("Broadcast hello world"), 7);
}

Test(command, unknown_command_returns_minus_one)
{
    cr_assert_eq(command_cost("Nawak"), -1);
    cr_assert_eq(command_cost(""), -1);
}
