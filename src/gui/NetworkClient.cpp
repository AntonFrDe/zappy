#include "NetworkClient.hpp"

#include <cerrno>
#include <cstring>
#include <netdb.h>
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
        errorMsg = "Connexion impossible a " + host + ":" + portStr + ": " + std::strerror(errno);
        return false;
    }
    _socketFd = fd;
    _connected = true;
    _readBuffer.clear();
    return true;
}

bool NetworkClient::performHandshake(std::string &errorMsg)
{
    errorMsg = "performHandshake: pas encore implementé";
    return false;
}

bool NetworkClient::receiveLines(std::vector<std::string> &lines, std::string &errorMsg)
{
    lines.clear();
    errorMsg = "receiveLines: pas encore implementé";
    return false;
}
