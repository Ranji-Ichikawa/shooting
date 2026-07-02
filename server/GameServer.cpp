#include "GameServer.h"
#include "Protocol.h"

namespace {
constexpr int kTickMs = 16; // ~60 Hz
}

bool GameServer::start(unsigned short port) {
    ws_.onConnect([this](int id) { onConnect(id); });
    ws_.onDisconnect([this](int id) { onDisconnect(id); });
    ws_.onMessage([this](int id, const std::string& msg) { onMessage(id, msg); });
    return ws_.start(port);
}

void GameServer::onConnect(int clientId) {
    sessions_[clientId] = game::GameSession();
}

void GameServer::onDisconnect(int clientId) {
    sessions_.erase(clientId);
}

void GameServer::onMessage(int clientId, const std::string& message) {
    auto it = sessions_.find(clientId);
    if (it == sessions_.end()) return;

    protocol::Command cmd = protocol::parse(message);
    switch (cmd.type) {
        case protocol::CommandType::Start:
            it->second.handleStart();
            break;
        case protocol::CommandType::Retry:
            it->second.handleRetry();
            break;
        case protocol::CommandType::Input:
            it->second.setInput(cmd.input);
            break;
        case protocol::CommandType::Unknown:
        default:
            break;
    }
}

void GameServer::tick() {
    for (auto& [clientId, session] : sessions_) {
        session.update();
        ws_.send(clientId, session.toJson());
    }
}

void GameServer::run() {
    while (true) {
        ws_.poll(kTickMs);
        tick();
    }
}
