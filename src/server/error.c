#include "../../include/server/error.h"
#include <stdlib.h>
#include <stdio.h>

retour_t fail(const char *error_msg, void *to_free)
{
    retour_t rt = {0};

    rt.error = true;
    rt.value = NULL;
    rt.nbPointer = 0;
    if (error_msg != NULL)
        snprintf(rt.error_msg, sizeof(rt.error_msg), "%s", error_msg);
    free(to_free);
    return rt;
}

void free_error(retour_t rt)
{
    void **arr;

    if (!rt.error || rt.value == NULL)
        return;
    if (rt.nbPointer > 0) {
        arr = rt.value;
        for (int i = 0; i < rt.nbPointer; i++)
            free(arr[i]);
    }
    free(rt.value);
}
