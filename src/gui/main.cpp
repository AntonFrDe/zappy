#include <iostream>

#include "gui.hpp"

int main(int argc, char **argv)
{
    gui_args_t args;
    std::string errorMsg;
    bool helpRequested = false;

    if (!parseArgs(argc, argv, args, errorMsg, helpRequested))
    {
        if (helpRequested)
        {
            printUsage(argv[0]);
            return 0;
        }
        if (!errorMsg.empty())
            std::cerr << errorMsg << std::endl;
        printUsage(argv[0]);
        return 84;
    }

    std::cout << "Connexion a " << args.host << ":" << args.port << "..." << std::endl;

    std::cout << "GUI initialisee en mode minimal."
        << " L'etape reseau restera a brancher dans un module dedie." << std::endl;
    return 0;
}
