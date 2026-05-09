#include "SelectDemultiplexer.h"
#include <algorithm>
#include <stdexcept>
#include <unistd.h>

SelectDemultiplexer::SelectDemultiplexer() : max_fd(0) {
    FD_ZERO(&read_set);
    FD_ZERO(&write_set);
    FD_ZERO(&err_set);
}

bool SelectDemultiplexer::regist(Handle handle, EventHandler* handler, int evt) {
    if (evt & static_cast<int>(Event::READ))  FD_SET(handle, &read_set);
    if (evt & static_cast<int>(Event::WRITE)) FD_SET(handle, &write_set);
    FD_SET(handle, &err_set);
    handlers[handle] = handler;
    max_fd = std::max(max_fd, handle);
    return true;
}

bool SelectDemultiplexer::remove(Handle handle) {
    FD_CLR(handle, &read_set);
    FD_CLR(handle, &write_set);
    FD_CLR(handle, &err_set);
    handlers.erase(handle);
    return true;
}

int SelectDemultiplexer::wait_event(int timeout) {
    fd_set r = read_set, w = write_set, e = err_set;
    timeval tv{timeout / 1000, (timeout % 1000) * 1000};
    int n = select(max_fd + 1, &r, &w, &e, timeout >= 0 ? &tv : nullptr);
    if (n <= 0) return n;
    for (auto& [fd, handler] : handlers) {
        if (FD_ISSET(fd, &e)) { handler->handle_error(); continue; }
        if (FD_ISSET(fd, &r)) handler->handle_read();
        if (FD_ISSET(fd, &w)) handler->handle_write();
    }
    return n;
}
