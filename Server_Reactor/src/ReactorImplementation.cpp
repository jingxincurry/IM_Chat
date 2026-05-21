#include "ReactorImplementation.h"
#include "EpollDemultiplexer.h"
#include "SelectDemultiplexer.h"

ReactorImplementation::ReactorImplementation() : running(false) {
#ifdef USE_SELECT
    demux = std::make_unique<SelectDemultiplexer>();
#else
    demux = std::make_unique<EpollDemultiplexer>();
#endif
}

void ReactorImplementation::register_handler(Handle fd, EventHandler* handler, int evt) {
    // 注册事件处理器
    handlers[fd] = handler;
    demux->regist(fd, handler, evt);
}

void ReactorImplementation::remove_handler(Handle fd) {
    // 删除事件处理器
    demux->remove(fd);
    handlers.erase(fd);
}

void ReactorImplementation::modify_handler(Handle fd, int evt) {
    // 修改事件处理器
    auto it = handlers.find(fd);
    if (it != handlers.end())
        demux->regist(fd, it->second, evt);
}

void ReactorImplementation::event_loop() {
    // 事件循环
    running = true;
    while (running)
        demux->wait_event(-1);
}

void ReactorImplementation::stop() {
    // 停止事件循环
    running = false;
}
