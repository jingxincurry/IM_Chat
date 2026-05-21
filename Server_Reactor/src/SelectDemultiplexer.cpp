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
    /*
        * 注册或修改事件：
        * - 将句柄添加到相应的 fd_set 中
        * - 更新 handlers 映射和 max_fd
    */
    if (evt & static_cast<int>(Event::READ))  FD_SET(handle, &read_set);
    if (evt & static_cast<int>(Event::WRITE)) FD_SET(handle, &write_set);
    FD_SET(handle, &err_set);
    handlers[handle] = handler;
    max_fd = std::max(max_fd, handle);
    return true;
}

bool SelectDemultiplexer::remove(Handle handle) {
    /*
        * 删除事件：
        * - 从相应的 fd_set 中清除句柄
        * - 从 handlers 映射中删除句柄
    */
    FD_CLR(handle, &read_set);
    FD_CLR(handle, &write_set);
    FD_CLR(handle, &err_set);
    handlers.erase(handle);
    return true;
}

int SelectDemultiplexer::wait_event(int timeout) {
    /*
        * 等待事件：
        * - 使用 select 等待事件发生
        * - 遍历 handlers 映射，调用相应的事件处理器回调
    */
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
