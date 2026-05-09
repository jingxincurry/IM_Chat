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
    std::unique_ptr<EventDemultiplexer> demux;
    std::unordered_map<Handle, EventHandler*> handlers;
    bool running;
};
