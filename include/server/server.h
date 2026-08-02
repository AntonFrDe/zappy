#ifndef SERVER_H_
    #define SERVER_H_
#include <unistd.h>
    #include "struct.h"
    #include "error.h"
    #include "map.h"
    #include "player.h"
    #include "client.h"
    #include "network.h"
    #include "command.h"
    #include "commands.h"
    #include <stdlib.h>
    #include <stdio.h>

    #define MAP_MAX_WIDTH  10000
    #define MAP_MAX_HEIGHT 10000
    retour_t parser(int argc, char **argv);

#endif /* !SERVER_H_ */
