#include "chatserver.h"
#include "EventDemultiplexer.h"
#include "Reactor.h"

#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {

constexpr int kBufferSize = 4096;
constexpr std::size_t kFileChunkSize = 4096;

class ChatListenHandler : public EventHandler
{
public:
    ChatListenHandler(ChatServer *server, Handle fd)
        : m_server(server)
        , m_fd(fd)
    {
    }

    Handle get_handle() const override
    {
        return m_fd;
    }

    void handle_read() override
    {
        m_server->onAcceptReady();
    }

private:
    ChatServer *m_server;
    Handle m_fd;
};

class ChatClientHandler : public EventHandler
{
public:
    ChatClientHandler(ChatServer *server, Handle fd)
        : m_server(server)
        , m_fd(fd)
    {
    }

    Handle get_handle() const override
    {
        return m_fd;
    }

    void handle_read() override
    {
        m_server->onClientReadable(m_fd);
    }

    void handle_error() override
    {
        m_server->onClientClosed(m_fd);
    }

    void handle_close() override
    {
        m_server->onClientClosed(m_fd);
    }

private:
    ChatServer *m_server;
    Handle m_fd;
};

}

ChatServer::ChatServer(int port)
    : m_port(port)
    , m_listenFd(-1)
    , m_fileService("uploads")
{
}

ChatServer::~ChatServer()
{
    cleanup();
}

bool ChatServer::start()
{
    /*
        * 启动服务器：
        * - 确保上传目录存在
        * - 初始化监听套接字
        * - 初始化 Reactor
    */
    return m_fileService.ensureUploadDir() && initListenSocket() && initReactor();
}

void ChatServer::run()
{
    /*
        * 运行服务器：
        * - 输出监听端口信息
        * - 启动 Reactor 事件循环
    */
    std::cout << "Chat server listening on port " << m_port << std::endl;
    Reactor::get_instance().event_loop();
}

void ChatServer::onAcceptReady()
{
    /*
        * 处理接受就绪事件：
        * - 处理新的连接请求
    */
    handleNewConnections();
}

void ChatServer::onClientReadable(int clientFd)
{
    /*
        * 处理客户端可读事件：
        * - 接收客户端发送的数据
    */
    handleClientEvent(clientFd);
}

void ChatServer::onClientClosed(int clientFd)
{
    /*
        * 处理客户端关闭事件：
        * - 清理客户端资源
    */
    closeClient(clientFd);
}

bool ChatServer::initListenSocket()
{
    /*
        * 初始化监听套接字：
        * - 创建套接字
        * - 设置套接字选项
        * - 绑定地址和端口
        * - 开始监听
    */
    m_listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_listenFd == -1) {
        return false;
    }

    int opt = 1;
    setsockopt(m_listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (!setNonBlocking(m_listenFd)) {
        return false;
    }

    sockaddr_in address {};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(m_port);

    if (bind(m_listenFd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == -1) {
        return false;
    }

    return listen(m_listenFd, SOMAXCONN) != -1;
}

bool ChatServer::initReactor()
{
    /*
        * 初始化 Reactor：
        * - 创建监听事件处理器
        * - 注册监听套接字到 Reactor
    */
    m_listenHandler = std::make_unique<ChatListenHandler>(this, m_listenFd);
    Reactor::get_instance().register_handler(m_listenFd,
                                             m_listenHandler.get(),
                                             static_cast<int>(Event::READ));
    return true;
}

bool ChatServer::setNonBlocking(int fd) const
{
    //设置文件描述符为非阻塞模式
    const int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        return false;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}

bool ChatServer::registerClient(int clientFd)
{
    /*
        * 注册客户端：
        * - 创建客户端事件处理器
        * - 注册客户端套接字到 Reactor
    */
    auto handler = std::make_unique<ChatClientHandler>(this, clientFd);
    Reactor::get_instance().register_handler(clientFd,
                                             handler.get(),
                                             static_cast<int>(Event::READ));
    m_clientHandlers[clientFd] = std::move(handler);
    return true;
}

void ChatServer::handleNewConnections()
{
    /*
        * 处理新的连接请求：
        * - 接受新的连接
        * - 为新连接创建事件处理器
    */
    while (true) {
        sockaddr_in clientAddress {};
        socklen_t clientLength = sizeof(clientAddress);
        const int clientFd = accept(m_listenFd,
                                    reinterpret_cast<sockaddr *>(&clientAddress),
                                    &clientLength);
        if (clientFd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            return;
        }

        if (!setNonBlocking(clientFd)) {
            close(clientFd);
            continue;
        }

        if (!registerClient(clientFd)) {
            close(clientFd);
            continue;
        }

        m_clients[clientFd] = ClientSession {};
        sendSystemMessage(clientFd, "Connected. Please login first.");
        sendFileList(clientFd);  // 连接后先发送文件列表
    }
}

void ChatServer::handleClientEvent(int clientFd)
{
    /*
        * 处理客户端可读事件：
        * - 接收客户端发送的数据
    */
    char temp[kBufferSize];
    while (true) {
        const ssize_t bytesRead = recv(clientFd, temp, sizeof(temp), 0);
        if (bytesRead > 0) {
            auto &session = m_clients[clientFd];
            session.buffer.append(temp, static_cast<std::size_t>(bytesRead));

            while (true) {
                Protocol::Packet packet;
                if (!Protocol::Packet::tryParse(session.buffer, packet)) {
                    break;
                }
                handlePacket(clientFd, packet);
                if (m_clients.find(clientFd) == m_clients.end()) {
                    return;
                }
            }
            continue;
        }

        if (bytesRead == 0) {
            closeClient(clientFd);
            return;
        }

        if (errno == EINTR) {
            continue;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }

        closeClient(clientFd);
        return;
    }
}

void ChatServer::handlePacket(int clientFd, const Protocol::Packet &packet)
{
    /*
        * 处理客户端发送的包：
        * - 根据命令类型调用相应的处理函数
    */
    switch (packet.command()) {
    case Protocol::CmdLogin:
        handleLogin(clientFd, packet.payload());
        break;
    case Protocol::CmdChatMessage:
        handleChatMessage(clientFd, packet.payload());
        break;
    case Protocol::CmdFileUploadBegin:
        handleUploadBegin(clientFd, packet.payload());
        break;
    case Protocol::CmdFileUploadChunk:
        handleUploadChunk(clientFd, packet.payload());
        break;
    case Protocol::CmdFileUploadEnd:
        handleUploadEnd(clientFd);
        break;
    case Protocol::CmdFileDownloadRequest:
        handleDownloadRequest(clientFd, packet.payload());
        break;
    default:
        sendError(clientFd, "Unsupported command.");
        break;
    }
}

void ChatServer::handleLogin(int clientFd, const std::string &payload)
{
    /*
        * 处理登录请求：
        * - 解析用户名
        * - 验证用户名合法性
        * - 更新客户端会话状态
        * - 广播登录消息和更新用户列表
    */
    std::string username;
    if (!Protocol::parseStringPayload(payload, username)) {
        sendError(clientFd, "Invalid login packet.");
        return;
    }
    if (username.empty()) {
        sendError(clientFd, "Username is empty.");
        return;
    }
    if (usernameExists(username, clientFd)) {
        sendError(clientFd, "Username already exists.");
        return;
    }

    auto &session = m_clients[clientFd];
    const bool firstLogin = session.username.empty();
    const std::string oldName = session.username;
    session.username = username;

    sendPacket(clientFd, Protocol::CmdLoginOk, Protocol::makeStringPayload(username));
    if (firstLogin) {
        broadcastPacket(Protocol::CmdSystemMessage, Protocol::makeStringPayload(username + " joined the chat"), clientFd);
    } else if (oldName != username) {
        broadcastPacket(Protocol::CmdSystemMessage, Protocol::makeStringPayload(oldName + " renamed to " + username), clientFd);
    }
    sendUserList();
    sendFileList(clientFd);
}

void ChatServer::handleChatMessage(int clientFd, const std::string &payload)
{
    /*
        * 处理聊天消息：
        * - 验证用户登录状态
        * - 解析消息内容
        * - 广播聊天消息
    */
    auto &session = m_clients[clientFd];
    if (session.username.empty()) {
        sendError(clientFd, "Please login first.");
        return;
    }

    std::string message;
    if (!Protocol::parseStringPayload(payload, message) || message.empty()) {
        sendError(clientFd, "Message is empty.");
        return;
    }

    broadcastPacket(Protocol::CmdChatMessage, Protocol::makeChatPayload(session.username, message));
}

void ChatServer::handleUploadBegin(int clientFd, const std::string &payload)
{
    /*
        * 处理文件上传开始请求：
        * - 验证用户登录状态
        * - 解析文件信息
        * - 初始化上传状态
    */
    auto &session = m_clients[clientFd];
    if (session.username.empty()) {
        sendError(clientFd, "Please login before upload.");
        return;
    }

    std::string fileName;
    std::string error;
    if (!m_fileService.beginUpload(session.upload, payload, fileName, error)) {
        sendError(clientFd, error);
        return;
    }

    sendSystemMessage(clientFd, "Start upload: " + fileName);
}

void ChatServer::handleUploadChunk(int clientFd, const std::string &payload)
{
    /*
        * 处理文件上传数据块：
        * - 验证上传状态
        * - 写入数据块到文件
    */
     auto &session = m_clients[clientFd];
     if (session.username.empty()) {
         sendError(clientFd, "Please login before upload.");
         return;
     }

     std::string error;
     if (!m_fileService.writeChunk(session.upload, payload, error)) {
         sendError(clientFd, error);
     }
    std::string error;
    if (!m_fileService.writeChunk(m_clients[clientFd].upload, payload, error)) {
        sendError(clientFd, error);
    }
}

void ChatServer::handleUploadEnd(int clientFd)
{
    /*
        * 处理文件上传结束请求：
        * - 验证上传状态
        * - 完成上传并验证文件完整性
        * - 广播上传完成消息和更新文件列表
    */
    auto &session = m_clients[clientFd];
    std::string fileName;
    std::string error;
    if (!m_fileService.endUpload(session.upload, fileName, error)) {
        sendError(clientFd, error);
        return;
    }

    sendSystemMessage(clientFd, "Upload finished: " + fileName);
    broadcastPacket(Protocol::CmdSystemMessage,
                    Protocol::makeStringPayload(session.username + " uploaded file: " + fileName),
                    clientFd);
    broadcastFileList();
}

void ChatServer::handleDownloadRequest(int clientFd, const std::string &payload)
{
    /*
        * 处理文件下载请求：
        * - 验证用户登录状态
        * - 解析下载请求
        * - 发送文件数据块
    */
    auto &session = m_clients[clientFd];
    if (session.username.empty()) {
        sendError(clientFd, "Please login before download.");
        return;
    }

    std::string fileName;
    if (!Protocol::parseStringPayload(payload, fileName)) {
        sendError(clientFd, "Invalid download request.");
        return;
    }

    std::string cleanName;
    Protocol::QWord fileSize = 0;
    if (!m_fileService.readFileInfo(fileName, cleanName, fileSize)) {
        sendError(clientFd, "File not found.");
        return;
    }

    sendPacket(clientFd, Protocol::CmdFileDownloadBegin, Protocol::makeFileBeginPayload(cleanName, fileSize));

    std::ifstream in;
    if (!m_fileService.openFileForRead(cleanName, in)) {
        sendError(clientFd, "Cannot open file.");
        return;
    }

    std::vector<char> chunk(kFileChunkSize);
    while (in) {
        in.read(chunk.data(), static_cast<std::streamsize>(chunk.size()));
        const std::streamsize count = in.gcount();
        if (count > 0) {
            sendPacket(clientFd,
                       Protocol::CmdFileDownloadChunk,
                       std::string(chunk.data(), static_cast<std::size_t>(count)));
        }
    }

    sendPacket(clientFd, Protocol::CmdFileDownloadEnd, Protocol::makeStringPayload(cleanName));
}

void ChatServer::closeClient(int clientFd)
{
    /*
        * 处理客户端关闭事件：
        * - 清理客户端资源
        * - 广播用户离开消息和更新用户列表
    */
    auto it = m_clients.find(clientFd);
    if (it == m_clients.end()) {
        return;
    }

    if (it->second.upload.file.is_open()) {
        it->second.upload.file.close();
        m_fileService.removeIncompleteUpload(it->second.upload);
    }

    const std::string username = it->second.username;
    Reactor::get_instance().remove_handler(clientFd);
    m_clientHandlers.erase(clientFd);
    close(clientFd);
    m_clients.erase(it);

    if (!username.empty()) {
        broadcastPacket(Protocol::CmdSystemMessage, Protocol::makeStringPayload(username + " left the chat"));
        sendUserList();
    }
}

void ChatServer::sendPacket(int clientFd, std::uint16_t cmd, const std::string &payload) const
{
    /*
        * 发送数据包到客户端：
        * - 构造数据包
        * - 发送数据包
    */
    const std::string data = Protocol::Packet(cmd, payload).serialize();
    const char *ptr = data.data();
    std::size_t left = data.size();

    while (left > 0) {
        const ssize_t sent = send(clientFd, ptr, left, MSG_NOSIGNAL);
        if (sent > 0) {
            ptr += sent;
            left -= static_cast<std::size_t>(sent);
            continue;
        }
        if (sent == -1 && errno == EINTR) {
            continue;
        }
        if (sent == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            continue;
        }
        return;
    }
}

void ChatServer::sendSystemMessage(int clientFd, const std::string &message) const
{
    /*
        * 发送系统消息到客户端：
        * - 构造系统消息数据包
        * - 发送数据包
    */
    sendPacket(clientFd, Protocol::CmdSystemMessage, Protocol::makeStringPayload(message));
}

void ChatServer::sendError(int clientFd, const std::string &message) const
{
    /*
        * 发送错误消息到客户端：
        * - 构造错误消息数据包
        * - 发送数据包
    */
    sendPacket(clientFd, Protocol::CmdError, Protocol::makeStringPayload(message));
}

void ChatServer::broadcastPacket(std::uint16_t cmd, const std::string &payload, int excludeFd) const
{
    /*
        * 广播数据包到所有客户端：
        * - 构造数据包
        * - 发送数据包到所有客户端（可选地排除某个客户端）
    */
    for (const auto &[fd, session] : m_clients) {
        (void)session;
        if (fd != excludeFd) {
            sendPacket(fd, cmd, payload);
        }
    }
}

void ChatServer::sendUserList() const
{
    /*
        * 发送用户列表到所有客户端：
        * - 构造用户列表数据包
        * - 广播用户列表数据包
    */
    std::vector<std::string> users;
    for (const auto &[fd, session] : m_clients) {
        (void)fd;
        if (!session.username.empty()) {
            users.push_back(session.username);
        }
    }
    broadcastPacket(Protocol::CmdUserList, Protocol::makeStringListPayload(users));
}

void ChatServer::sendFileList(int clientFd) const
{
    /*
        * 发送文件列表到指定客户端：
        * - 构造文件列表数据包
        * - 发送数据包到客户端
    */
    sendPacket(clientFd, Protocol::CmdFileList, Protocol::makeStringListPayload(m_fileService.listFiles()));
}

void ChatServer::broadcastFileList() const
{
    broadcastPacket(Protocol::CmdFileList, Protocol::makeStringListPayload(m_fileService.listFiles()));
}

bool ChatServer::usernameExists(const std::string &username, int excludeFd) const
{
    for (const auto &[fd, session] : m_clients) {
        if (fd != excludeFd && session.username == username) {
            return true;
        }
    }
    return false;
}

void ChatServer::cleanup()
{
    /*
        * 清理服务器资源：
        * - 关闭所有客户端连接
        * - 关闭监听套接字
    */
    for (auto &[fd, session] : m_clients) {
        if (session.upload.file.is_open()) {
            session.upload.file.close();
        }
        Reactor::get_instance().remove_handler(fd);
        close(fd);
    }
    m_clients.clear();
    m_clientHandlers.clear();

    if (m_listenFd != -1) {
        Reactor::get_instance().remove_handler(m_listenFd);
        close(m_listenFd);
        m_listenFd = -1;
    }
    m_listenHandler.reset();
}
