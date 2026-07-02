#pragma once
#include <unordered_map>

#include "GameState.h"
#include "WebSocketServer.h"

// Orchestrates the WebSocket transport and one GameSession per connected
// client. This is the only file that knows both "networking" and "game
// logic" exist -- WebSocketServer and GameState stay independent of each
// other so either can be worked on / replaced without touching the other.
class GameServer {
public:
    bool start(unsigned short port);
    void run(); // blocks forever, driving the tick loop

private:
    WebSocketServer ws_;
    std::unordered_map<int, game::GameSession> sessions_;

    void onConnect(int clientId);
    void onDisconnect(int clientId);
    void onMessage(int clientId, const std::string& message);
    void tick();
};
