// 事件分发器接口，负责事件的检测和分发
#pragma once
#include "EventHandler.h"
#include <unordered_map>

enum class Event { READ = 1, WRITE = 2, ERROR = 4 };

class EventDemultiplexer {
public:
    virtual ~EventDemultiplexer() = default;
    virtual bool regist(Handle handle, EventHandler* handler, int evt) = 0;
    virtual bool remove(Handle handle) = 0;
    // Returns number of events dispatched; calls handler callbacks internally
    virtual int wait_event(int timeout = 0) = 0;

protected:
    std::unordered_map<Handle, EventHandler*> handlers;
};
