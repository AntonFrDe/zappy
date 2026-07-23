#include "NetworkClient.hpp"

#include <netdb.h>
#include <poll.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

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

    if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))
    {
        errorMsg = "Connexion perdue avec le serveur";
        disconnect();
        return false;
    }

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

bool NetworkClient::readLineBlocking(std::string &line, std::string &errorMsg)
{
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

        if (!readAvailable(5000, errorMsg))
            return false;
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
    ssize_t sent = send(_socketFd, kGraphicMessage.c_str(), kGraphicMessage.size(), 0);
    if (sent < 0 || static_cast<std::size_t>(sent) != kGraphicMessage.size())
    {
        errorMsg = "Envoi de GRAPHIC echoue";
        disconnect();
        return false;
    }

    return true;
}

bool NetworkClient::readInitialMapSize(int &width, int &height, std::string &errorMsg)
{
    std::string discard;
    if (!readLineBlocking(discard, errorMsg))
        return false;
    std::string sizeLine;
    if (!readLineBlocking(sizeLine, errorMsg))
        return false;
    std::istringstream iss(sizeLine);
    if (!(iss >> width >> height))
    {
        errorMsg = "Taille de map invalide recue du serveur: \"" + sizeLine + "\"";
        return false;
    }
    return true;
}

bool NetworkClient::receiveLines(std::vector<std::string> &lines, std::string &errorMsg)
{
    lines.clear();
    if (!readAvailable(16, errorMsg))
        return false;
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
