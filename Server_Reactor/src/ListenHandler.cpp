#include "ListenHandler.h"
#include "SockHandler.h"
#include "Reactor.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <stdexcept>
#include <unistd.h>

ListenHandler::ListenHandler(int port) {
    setup(port);
}

ListenHandler::~ListenHandler() {
    close(listen_fd);
}

Handle ListenHandler::get_handle() const { 
    return listen_fd; 
}

void ListenHandler::setup(int port) {
    /*
        * 设置监听套接字：
        * - 创建套接字
        * - 设置套接字选项
    */
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) throw std::runtime_error("socket failed");

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(listen_fd, (sockaddr*)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("bind failed");
    if (listen(listen_fd, SOMAXCONN) < 0)
        throw std::runtime_error("listen failed");
}

void ListenHandler::handle_read() {
    /*
        * 处理读事件：
        * - 接受新的连接请求
        * - 为新连接创建套接字处理器
    */
    int conn_fd = accept(listen_fd, nullptr, nullptr);
    if (conn_fd < 0) return;

    auto* handler = new SockHandler(conn_fd);
    Reactor::get_instance().register_handler(
        conn_fd, handler, static_cast<int>(Event::READ));
}
