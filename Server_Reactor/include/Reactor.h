#pragma once
#include "EventHandler.h"
#include <memory>

class ReactorImplementation;

class Reactor {
public:
    static Reactor& get_instance();

    void register_handler(Handle fd, EventHandler* handler, int evt);
    void remove_handler(Handle fd);
    void modify_handler(Handle fd, int evt);
    void event_loop();
    void stop();

private:
    Reactor();
    ~Reactor();
    Reactor(const Reactor&) = delete;
    Reactor& operator=(const Reactor&) = delete;

    std::unique_ptr<ReactorImplementation> impl;
};
