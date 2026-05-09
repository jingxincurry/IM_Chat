#include "SockHandler.h"
#include "Reactor.h"
#include <unistd.h>
#include <cstring>

SockHandler::SockHandler(Handle fd) : sock_fd(fd) {}

SockHandler::~SockHandler() {
    close(sock_fd);
}

Handle SockHandler::get_handle() const { return sock_fd; }

void SockHandler::handle_read() {
    ssize_t n = recv(sock_fd, buf, BUF_SIZE, 0);
    if (n <= 0) {
        handle_close();
        return;
    }
    // echo back
    send(sock_fd, buf, n, 0);
}

void SockHandler::handle_error() {
    handle_close();
}

void SockHandler::handle_close() {
    Reactor::get_instance().remove_handler(sock_fd);
    delete this;
}
