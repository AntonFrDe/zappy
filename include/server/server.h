#ifndef SERVER_H_
    #define SERVER_H_
#include <unistd.h>
    #include "struct.h"
    #include "error.h"
    #include "client.h"
    #include "map.h"
    #include "network.h"
    #include "command.h"
    #include <stdlib.h>
    #include <stdio.h>

    retour_t parser(int argc, char **argv);

#endif /* !SERVER_H_ */
