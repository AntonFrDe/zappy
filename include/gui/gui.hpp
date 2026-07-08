#ifndef GUI_HPP_
    #define GUI_HPP_

#include <string>

struct gui_args_t {
    unsigned short port = 0;
    std::string host = "localhost";
    bool hasPort = false;
};

void printUsage(const char *prog);
bool parseArgs(int argc, char **argv, gui_args_t &args, std::string &errorMsg, bool &helpRequested);

#endif /* !GUI_HPP_ */
