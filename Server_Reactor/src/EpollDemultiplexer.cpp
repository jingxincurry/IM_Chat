#include "EpollDemultiplexer.h"
#include <stdexcept>
#include <unistd.h>

EpollDemultiplexer::EpollDemultiplexer() : evs(max_event) {
    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) throw std::runtime_error("epoll_create1 failed");
}

EpollDemultiplexer::~EpollDemultiplexer() {
    close(epoll_fd);
}

bool EpollDemultiplexer::regist(Handle handle, EventHandler* handler, int evt) {
    epoll_event ev{};
    ev.data.fd = handle;
    if (evt & static_cast<int>(Event::READ))  ev.events |= EPOLLIN;
    if (evt & static_cast<int>(Event::WRITE)) ev.events |= EPOLLOUT;

    int op = handlers.count(handle) ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    if (epoll_ctl(epoll_fd, op, handle, &ev) < 0) return false;
    handlers[handle] = handler;
    return true;
}

bool EpollDemultiplexer::remove(Handle handle) {
    if (!handlers.count(handle)) return false;
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, handle, nullptr);
    handlers.erase(handle);
    return true;
}

int EpollDemultiplexer::wait_event(int timeout) {
    int n = epoll_wait(epoll_fd, evs.data(), max_event, timeout);
    for (int i = 0; i < n; ++i) {
        Handle fd = evs[i].data.fd;
        auto it = handlers.find(fd);
        if (it == handlers.end()) continue;
        EventHandler* h = it->second;
        if (evs[i].events & (EPOLLERR | EPOLLHUP)) { h->handle_error(); continue; }
        if (evs[i].events & EPOLLIN)  h->handle_read();
        if (evs[i].events & EPOLLOUT) h->handle_write();
    }
    return n;
}
