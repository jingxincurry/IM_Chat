#pragma once
#include "EventDemultiplexer.h"
#include <vector>
#include <sys/epoll.h>

class EpollDemultiplexer : public EventDemultiplexer {
public:
    EpollDemultiplexer();
    ~EpollDemultiplexer() override;

    bool regist(Handle handle, EventHandler* handler, int evt) override;
    bool remove(Handle handle) override;
    int  wait_event(int timeout = 0) override;

private:
    int epoll_fd;
    static constexpr int max_event = 1024;
    std::vector<epoll_event> evs;
};
