#include <cstdlib>
#include <ctime>
#include <iostream>

#include "GameServer.h"

int main() {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    constexpr unsigned short kPort = 8765;

    GameServer server;
    if (!server.start(kPort)) {
        std::cerr << "Failed to start server on port " << kPort << std::endl;
        return 1;
    }

    std::cout << "Shooting game server listening on ws://localhost:" << kPort << std::endl;
    server.run();
    return 0;
}
