#pragma once

#include "EventDemultiplexer.h"
#include "EventHandler.h"
#include <memory>
#include <unordered_map>

class ReactorImplementation {
public:
    ReactorImplementation();
    ~ReactorImplementation() = default;

    void register_handler(Handle fd, EventHandler* handler, int evt);
    void remove_handler(Handle fd);
    void modify_handler(Handle fd, int evt);
    void event_loop();
    void stop();

private:
    std::unique_ptr<EventDemultiplexer> demux;  // 事件分发器
    std::unordered_map<Handle, EventHandler*> handlers;  // 句柄到事件处理器的映射
    bool running;
};
