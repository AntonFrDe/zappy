#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>

#include "../../include/gui/gui.hpp"
#include "../../include/gui/GameState.hpp"
#include "../../include/gui/CommandParser.hpp"
#include "../../include/gui/NetworkClient.hpp"
#include "IsoView.hpp"

int main(int argc, char **argv)
{
    gui_args_t args;
    std::string errorMsg;
    bool helpRequested = false;

    if (!parseArgs(argc, argv, args, errorMsg, helpRequested)) {
        if (helpRequested) {
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
    if (!network.connectToServer(args.host, args.port, errorMsg)) {
        std::cerr << errorMsg << std::endl;
        return 84;
    }
    if (!network.performHandshake(errorMsg)) {
        std::cerr << errorMsg << std::endl;
        return 84;
    }
    std::cout << "Connecte au serveur, handshake reussi." << std::endl;

    GameState state;
    int width, height;
    if (!network.readInitialMapSize(width, height, errorMsg)) {
        std::cerr << errorMsg << std::endl;
        return 84;
    }
    state.resizeMap(width, height);
    std::cout << "Taille de la map: " << width << "x" << height << std::endl;

    CommandParser parser(state);
    IsoGridCache grid;
    grid.rebuildIfNeeded(width, height);
    sf::RenderWindow window(sf::VideoMode(900, 650), "Zappy iso");
    window.setVerticalSyncEnabled(true);

    std::vector<std::string> lines;

    while (network.isConnected() && window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }
        if (!network.receiveLines(lines, errorMsg)) {
            std::cerr << "[network] " << errorMsg << std::endl;
            break;
        }
        for (const std::string &line : lines)
            parser.dispatch(line);

        grid.rebuildIfNeeded(state.mapWidth, state.mapHeight);
        window.clear(sf::Color(16, 16, 24));
        drawState(window, state, grid);
        window.display();
    }

    std::cout << "Deconnexion du serveur." << std::endl;
    return 0;
}
