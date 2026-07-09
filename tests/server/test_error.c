/*
** Unit tests for src/server/error.c (retour_t fail(...) / void free_error(...))
** Framework: Criterion
*/

#include <criterion/criterion.h>
#include <server/server.h>

Test(error, fail_sets_error_true)
{
    retour_t rt = fail("some message", NULL);

    cr_assert_eq(rt.error, true);
}

Test(error, fail_copies_error_message)
{
    retour_t rt = fail("boom", NULL);

    cr_assert_str_eq(rt.error_msg, "boom");
}

Test(error, fail_sets_value_null)
{
    retour_t rt = fail("boom", NULL);

    cr_assert_null(rt.value);
}

Test(error, fail_frees_the_given_pointer)
{
    char *ptr = malloc(16);
    retour_t rt;

    cr_assert_not_null(ptr);
    rt = fail("boom", ptr);
    cr_assert_eq(rt.error, true);
    /* no direct way to assert the pointer was freed without an
     * instrumented allocator; this test mainly documents the
     * contract and is exercised for leaks under valgrind. */
}

Test(error, fail_handles_null_message)
{
    retour_t rt = fail(NULL, NULL);

    cr_assert_eq(rt.error, true);
    cr_assert_eq(rt.error_msg[0], '\0');
}

Test(error, free_error_is_noop_on_success)
{
    retour_t rt = {0};
    int dummy = 42;

    rt.error = false;
    rt.value = &dummy;
    free_error(rt);
    cr_assert_eq(dummy, 42);
}

Test(error, free_error_is_noop_when_value_null)
{
    retour_t rt = fail("boom", NULL);

    free_error(rt);
    cr_assert_eq(rt.error, true);
}

Test(error, free_error_frees_single_value)
{
    retour_t rt = {0};

    rt.error = true;
    rt.value = malloc(32);
    rt.nbPointer = 0;
    cr_assert_not_null(rt.value);
    free_error(rt);
}

Test(error, free_error_frees_pointer_array)
{
    retour_t rt = {0};
    void **arr = malloc(sizeof(void *) * 3);

    arr[0] = malloc(4);
    arr[1] = malloc(4);
    arr[2] = malloc(4);
    rt.error = true;
    rt.value = arr;
    rt.nbPointer = 3;
    free_error(rt);
}
