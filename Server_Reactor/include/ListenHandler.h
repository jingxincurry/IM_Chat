#pragma once
#include "EventHandler.h"

class ListenHandler : public EventHandler {
public:
    ListenHandler(int port);
    ~ListenHandler() override;

    Handle get_handle() const override;
    void handle_read() override;  // accept new connection

private:
    Handle listen_fd;
    void setup(int port);
};
