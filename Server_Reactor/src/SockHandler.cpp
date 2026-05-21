#include "SockHandler.h"
#include "Reactor.h"
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
SockHandler::SockHandler(Handle fd) : sock_fd(fd) {}

SockHandler::~SockHandler() {
    close(sock_fd);
}

Handle SockHandler::get_handle() const { 
    return sock_fd; 
}

void SockHandler::handle_read() {
    /*
        * 处理读事件：
        * - 接收数据
        * - 处理接收到的数据
    */
    ssize_t n = recv(sock_fd, buf, BUF_SIZE, 0);
    if (n <= 0) {
        handle_close();
        return;
    }
    // echo back
    send(sock_fd, buf, n, 0);
}

void SockHandler::handle_error() {
    /*
        * 处理错误事件：
        * - 关闭连接
    */
    handle_close();
}

void SockHandler::handle_close() {
    /*
        * 处理关闭事件：
        * - 从 Reactor 中移除事件处理器
        * - 关闭套接字
    */
    Reactor::get_instance().remove_handler(sock_fd);
    delete this;
}
