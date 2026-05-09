#include "chatserver.h"

#include <iostream>

int main()
{
    ChatServer server(8888);
    if (!server.start()) {
        std::cerr << "server start failed" << std::endl;
        return 1;
    }

    server.run();
    return 0;
}
