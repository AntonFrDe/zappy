#pragma once

#include <string>
#include <vector>

class NetworkClient
{
    private:
        int _socketFd;           // descripteur du socket TCP, -1 si non connecte
        bool _connected;         // etat courant de la connexion
        std::string _readBuffer; // reliquat de donnees ne formant pas encore une ligne complete
 
        // Lit ce qui est disponible sur le socket et l'ajoute a _readBuffer.
        bool readAvailable(int waitMs, std::string &errorMsg);
 
        // Attend jusqu'a obtenir une ligne complete terminee par '\n'.
        bool readLineBlocking(std::string &line, std::string &errorMsg);
 
    public:
        // Cree un client non connecte.
        NetworkClient();
 
        // Ferme la connexion si besoin.
        ~NetworkClient();
 
        NetworkClient(const NetworkClient &) = delete;
        NetworkClient &operator=(const NetworkClient &) = delete;
 
        // Ouvre la connexion TCP vers host:port.
        bool connectToServer(const std::string &host, unsigned short port, std::string &errorMsg);
 
        // Realise le handshake avec le serveur (WELCOME -> GRAPHIC).
        bool performHandshake(std::string &errorMsg);
 
        // Recupere la taille de la carte envoyee par le serveur au demarrage.
        bool readInitialMapSize(int &width, int &height, std::string &errorMsg);
 
        // Recupere les nouvelles lignes de commande recues du serveur.
        bool receiveLines(std::vector<std::string> &lines, std::string &errorMsg);
 
        // Indique si la connexion est toujours active.
        bool isConnected() const;
 
        // Ferme le socket et reinitialise le client.
        void disconnect();
};
