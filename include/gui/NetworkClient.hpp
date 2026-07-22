#pragma once

#include <string>
#include <vector>

class NetworkClient
{
    private:
        int _socketFd;
        bool _connected;
        std::string _readBuffer;
        bool readAvailable(int waitMs, std::string &errorMsg);
        bool readLineBlocking(std::string &line, std::string &errorMsg);
    public:
        NetworkClient();
        ~NetworkClient();
        NetworkClient(const NetworkClient &) = delete;
        NetworkClient &operator=(const NetworkClient &) = delete;
        bool connectToServer(const std::string &host, unsigned short port, std::string &errorMsg);
        bool performHandshake(std::string &errorMsg);
        bool receiveLines(std::vector<std::string> &lines, std::string &errorMsg);

        bool isConnected() const;
        void disconnect();
};
