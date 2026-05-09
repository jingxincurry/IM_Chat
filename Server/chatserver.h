#ifndef CHATSERVER_H
#define CHATSERVER_H

#include "filetransfer.h"
#include "protocol.h"

#include <cstdint>
#include <string>
#include <unordered_map>

struct ClientSession
{
    std::string username;
    std::string buffer;
    FileUploadState upload;
};

class ChatServer
{
public:
    explicit ChatServer(int port = 8888);
    ~ChatServer();

    bool start();
    void run();

private:
    bool initListenSocket();
    bool initEpoll();
    bool setNonBlocking(int fd) const;

    void handleNewConnections();
    void handleClientEvent(int clientFd, unsigned int events);
    void handlePacket(int clientFd, const Protocol::Packet &packet);
    void handleLogin(int clientFd, const std::string &payload);
    void handleChatMessage(int clientFd, const std::string &payload);
    void handleUploadBegin(int clientFd, const std::string &payload);
    void handleUploadChunk(int clientFd, const std::string &payload);
    void handleUploadEnd(int clientFd);
    void handleDownloadRequest(int clientFd, const std::string &payload);
    void closeClient(int clientFd);

    void sendPacket(int clientFd, std::uint16_t cmd, const std::string &payload) const;
    void sendSystemMessage(int clientFd, const std::string &message) const;
    void sendError(int clientFd, const std::string &message) const;
    void broadcastPacket(std::uint16_t cmd, const std::string &payload, int excludeFd = -1) const;
    void sendUserList() const;
    void sendFileList(int clientFd) const;
    void broadcastFileList() const;
    bool usernameExists(const std::string &username, int excludeFd) const;
    void cleanup();

private:
    int m_port;
    int m_listenFd;
    int m_epollFd;
    FileTransferService m_fileService;
    std::unordered_map<int, ClientSession> m_clients;
};

#endif
