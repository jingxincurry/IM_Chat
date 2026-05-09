#pragma once
#include "EventHandler.h"
#include <string>

class SockHandler : public EventHandler {
public:
    explicit SockHandler(Handle fd);
    ~SockHandler() override;

    Handle get_handle() const override;
    void handle_read() override;
    void handle_error() override;
    void handle_close() override;

private:
    Handle sock_fd;
    static constexpr int BUF_SIZE = 1024;
    char buf[BUF_SIZE];
};
