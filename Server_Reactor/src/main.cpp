#include "Reactor.h"
#include "ListenHandler.h"
#include <iostream>

int main() {
    const int PORT = 8080;
    ListenHandler acceptor(PORT);
    Reactor& reactor = Reactor::get_instance();
    reactor.register_handler(acceptor.get_handle(), &acceptor,
                             static_cast<int>(Event::READ));
    std::cout << "Echo server listening on port " << PORT << "\n";
    reactor.event_loop();
    return 0;
}
