#pragma once
#include "EventDemultiplexer.h"
#include <sys/select.h>

class SelectDemultiplexer : public EventDemultiplexer {
public:
    SelectDemultiplexer();
    bool regist(Handle handle, EventHandler* handler, int evt) override;
    bool remove(Handle handle) override;
    int  wait_event(int timeout = 0) override;

private:
    fd_set read_set, write_set, err_set;
    int max_fd;
};
