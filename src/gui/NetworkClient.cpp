#include "NetworkClient.hpp"

NetworkClient::NetworkClient() : _socketFd(-1), _connected(false)
{
}

NetworkClient::~NetworkClient()
{
    disconnect();
}

void NetworkClient::disconnect()
{
    _socketFd = -1;
    _connected = false;
}

bool NetworkClient::isConnected() const
{
    return _connected;
}

bool NetworkClient::connectToServer(const std::string &host, unsigned short port, std::string &errorMsg)
{
    (void)host;
    (void)port;
    errorMsg = "connectToServer: pas encore implementé";
    return false;
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
