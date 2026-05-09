#include "Reactor.h"
#include "ReactorImplementation.h"

Reactor::Reactor() {
    impl = std::make_unique<ReactorImplementation>();
}

Reactor& Reactor::get_instance() {
    static Reactor instance;
    return instance;
}

Reactor::~Reactor() = default;

void Reactor::register_handler(Handle fd, EventHandler* handler, int evt) {
    impl->register_handler(fd, handler, evt);
}

void Reactor::remove_handler(Handle fd) {
    impl->remove_handler(fd);
}

void Reactor::modify_handler(Handle fd, int evt) {
    impl->modify_handler(fd, evt);
}

void Reactor::event_loop() {
    impl->event_loop();
}

void Reactor::stop() {
    impl->stop();
}
