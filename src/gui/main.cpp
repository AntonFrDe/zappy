#include <iostream>

#include "gui.hpp"
#include "GameState.hpp"
#include "CommandParser.hpp"
#include "NetworkClient.hpp"
#include "Renderer.hpp"

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
    NetworkClient network;
    if (!network.connectToServer(args.host, args.port, errorMsg))
    {
        std::cerr << errorMsg << std::endl;
        return 84;
    }
    if (!network.performHandshake(errorMsg))
    {
        std::cerr << errorMsg << std::endl;
        return 84;
    }
    std::cout << "Connecte au serveur, handshake reussi." << std::endl;

    GameState state;
    int width, height;
    if (!network.readInitialMapSize(width, height, errorMsg))
    {
        std::cerr << errorMsg << std::endl;
        return 84;
    }
    state.resizeMap(width, height);
    std::cout << "Taille de la map: " << width << "x" << height << std::endl;

    CommandParser parser(state);
    Renderer renderer;
    sf::Clock clock;
    std::vector<std::string> lines;

    while (network.isConnected() && renderer.isOpen())
    {
        renderer.pollEvents();
        if (!network.receiveLines(lines, errorMsg))
        {
            std::cerr << "[network] " << errorMsg << std::endl;
            break;
        }
        for (const std::string &line : lines)
            parser.dispatch(line);
        renderer.draw(state, clock.restart().asSeconds());
    }
    std::cout << "Deconnexion du serveur." << std::endl;
    return 0;
}
