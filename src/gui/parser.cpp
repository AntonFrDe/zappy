#include <cstdlib>
#include <iostream>

#include "gui.hpp"

void printUsage(const char *prog)
{
    std::cerr << "USAGE: " << prog << " -p port -h machine" << std::endl;
    std::cerr << "  -p port     port number" << std::endl;
    std::cerr << "  -h machine  hostname of the server (default: localhost)" << std::endl;
}

bool parseArgs(int argc, char **argv, gui_args_t &args, std::string &errorMsg, bool &helpRequested)
{
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-p" && i + 1 < argc) {
            int port = std::atoi(argv[++i]);

            if (port < 1 || port > 65535) {
                errorMsg = "Port given value isn't a valid port";
                return false;
            }
            args.port = static_cast<unsigned short>(port);
            args.hasPort = true;
        } else if (arg == "-h" && i + 1 < argc) {
            args.host = argv[++i];
        } else if (arg == "--help") {
            helpRequested = true;
            return false;
        } else {
            errorMsg = "Unknown option or missing value for a flag";
            return false;
        }
    }
    if (!args.hasPort) {
        errorMsg = "Missing mandatory argument (-p)";
        return false;
    }
    return true;
}