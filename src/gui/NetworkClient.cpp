#include "NetworkClient.hpp"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <netdb.h>
#include <poll.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

namespace {
constexpr int kHandshakeTimeoutMs = 10000;
constexpr int kMaxDrainIterationsPerFrame = 64;
}

NetworkClient::NetworkClient() : _socketFd(-1), _connected(false)
{
}

NetworkClient::~NetworkClient()
{
    disconnect();
}

void NetworkClient::disconnect()
{
    if (_socketFd >= 0)
        close(_socketFd);
    _socketFd = -1;
    _connected = false;
}

bool NetworkClient::isConnected() const
{
    return _connected;
}

bool NetworkClient::connectToServer(const std::string &host, unsigned short port, std::string &errorMsg)
{
    disconnect();
    struct addrinfo addr{};
    addr.ai_family = AF_UNSPEC;     // IPv4 ou IPv6
    addr.ai_socktype = SOCK_STREAM; // TCP
    struct addrinfo *resolved = nullptr;
    std::string portStr = std::to_string(port);

    int getAi = getaddrinfo(host.c_str(), portStr.c_str(), &addr, &resolved);
    if (getAi != 0)
    {
        errorMsg = "Impossible de resoudre \"" + host + "\": " + gai_strerror(getAi);
        return false;
    }
    int fd = -1;
    for (struct addrinfo *it = resolved; it != nullptr; it = it->ai_next)
    {
        fd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (fd < 0)
            continue;
        if (connect(fd, it->ai_addr, it->ai_addrlen) == 0)
            break;
        close(fd);
        fd = -1;
    }
    freeaddrinfo(resolved);
    if (fd < 0)
    {
        errorMsg = "Connexion impossible a " + host + ":" + portStr;
        return false;
    }
    _socketFd = fd;
    _connected = true;
    _readBuffer.clear();
    return true;
}

bool NetworkClient::readAvailable(int waitMs, std::string &errorMsg)
{
    if (!_connected)
    {
        errorMsg = "Socket non connecte";
        return false;
    }

    struct pollfd pfd{};
    pfd.fd = _socketFd;
    pfd.events = POLLIN;

    int pollResult = poll(&pfd, 1, waitMs);
    if (pollResult < 0)
    {
        errorMsg = "poll() a echoue";
        disconnect();
        return false;
    }
    if (pollResult == 0)
        return true;

    // Le noyau peut lever POLLHUP/POLLERR en meme temps que POLLIN si le
    // serveur a ferme la connexion juste apres avoir envoye ses derniers
    // octets (ex: un "seg" final). On draine donc d'abord les donnees
    // disponibles, et on ne traite l'erreur/le hangup que si recv() ne
    // rapporte plus rien.
    if (pfd.revents & POLLIN)
    {
        char buffer[4096];
        ssize_t received = recv(_socketFd, buffer, sizeof(buffer), 0);
        if (received < 0)
        {
            errorMsg = "recv() a echoue";
            disconnect();
            return false;
        }
        if (received == 0)
        {
            errorMsg = "Le serveur a ferme la connexion";
            disconnect();
            return false;
        }
        _readBuffer.append(buffer, static_cast<std::size_t>(received));
        return true;
    }

    if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))
    {
        errorMsg = "Connexion perdue avec le serveur";
        disconnect();
        return false;
    }

    return true;
}

bool NetworkClient::readLineBlocking(std::string &line, std::string &errorMsg)
{
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(kHandshakeTimeoutMs);

    while (true)
    {
        std::size_t pos = _readBuffer.find('\n');
        if (pos != std::string::npos)
        {
            line = _readBuffer.substr(0, pos);
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            _readBuffer.erase(0, pos + 1);
            return true;
        }

        auto now = std::chrono::steady_clock::now();
        if (now >= deadline)
        {
            errorMsg = "Timeout en attente de donnees du serveur (handshake)";
            disconnect();
            return false;
        }
        // Le poll ne dure jamais plus d'1s a la fois, pour re-verifier
        // regulierement l'echeance globale au lieu de la repousser sans
        // fin comme le faisait un poll() a 5000ms rappele en boucle.
        auto remainingMs = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count();
        int waitMs = static_cast<int>(std::min<long long>(remainingMs, 1000));
        if (!readAvailable(waitMs, errorMsg))
            return false;
    }
}

namespace {

// send() sur un socket TCP n'est pas tenu d'ecrire tout le buffer d'un
// coup (partial write) : il faut boucler jusqu'a avoir tout envoye.
bool sendAll(int fd, const char *data, std::size_t size, std::string &errorMsg)
{
    std::size_t totalSent = 0;

    while (totalSent < size)
    {
        ssize_t sent = send(fd, data + totalSent, size - totalSent, 0);
        if (sent < 0)
        {
            if (errno == EINTR)
                continue;
            errorMsg = "send() a echoue";
            return false;
        }
        if (sent == 0)
        {
            errorMsg = "send() n'a pu ecrire aucune donnee";
            return false;
        }
        totalSent += static_cast<std::size_t>(sent);
    }
    return true;
}

}

bool NetworkClient::performHandshake(std::string &errorMsg)
{
    if (!_connected)
    {
        errorMsg = "Impossible de faire le handshake: pas connecte";
        return false;
    }

    std::string welcomeLine;
    if (!readLineBlocking(welcomeLine, errorMsg))
        return false;
    if (welcomeLine != "WELCOME")
    {
        errorMsg = "Handshake invalide, attendu \"WELCOME\", recu \"" + welcomeLine + "\"";
        disconnect();
        return false;
    }
    static const std::string kGraphicMessage = "GRAPHIC\n";
    if (!sendAll(_socketFd, kGraphicMessage.c_str(), kGraphicMessage.size(), errorMsg))
    {
        disconnect();
        return false;
    }

    return true;
}

bool NetworkClient::readInitialMapSize(int &width, int &height, std::string &errorMsg)
{
    std::string sizeLine;
    std::string command;
    std::string extra;

    if (!readLineBlocking(sizeLine, errorMsg))
        return false;
    std::istringstream iss(sizeLine);
    if (!(iss >> command >> width >> height) || command != "msz"
        || width <= 0 || height <= 0 || (iss >> extra))
    {
        errorMsg = "Commande msz invalide recue du serveur: \"" + sizeLine + "\"";
        return false;
    }
    return true;
}

bool NetworkClient::receiveLines(std::vector<std::string> &lines, std::string &errorMsg)
{
    lines.clear();
    if (!readAvailable(16, errorMsg))
        return false;

    // Draine le reste des donnees deja disponibles sur le socket (sans
    // attendre) : un seul recv() de 4096 octets par frame prenait du
    // retard des qu'une rafale d'evenements plus grosse arrivait (ex:
    // snapshot initial avec beaucoup de pnw/bct/enw d'un coup).
    for (int i = 0; i < kMaxDrainIterationsPerFrame; ++i)
    {
        std::size_t sizeBefore = _readBuffer.size();
        if (!readAvailable(0, errorMsg))
            return false;
        if (_readBuffer.size() == sizeBefore)
            break;
    }

    std::size_t start = 0;
    std::size_t pos;
    while ((pos = _readBuffer.find('\n', start)) != std::string::npos)
    {
        std::string line = _readBuffer.substr(start, pos - start);
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        lines.push_back(std::move(line));
        start = pos + 1;
    }
    _readBuffer.erase(0, start);
    return true;
}
