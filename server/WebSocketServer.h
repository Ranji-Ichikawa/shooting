#pragma once

#include <winsock2.h>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

// Minimal RFC 6455 WebSocket server built directly on Winsock2.
// No external dependencies (no Boost/uWebSockets) so it builds with a bare
// MinGW g++ toolchain. Handles the HTTP upgrade handshake and text frame
// (de)framing; everything above "a stream of text messages per client" is
// left to the caller (see GameServer).
class WebSocketServer {
public:
    using MessageHandler = std::function<void(int clientId, const std::string& message)>;
    using ConnectHandler = std::function<void(int clientId)>;
    using DisconnectHandler = std::function<void(int clientId)>;

    WebSocketServer();
    ~WebSocketServer();

    bool start(unsigned short port);
    void stop();

    // Services socket I/O once; blocks for at most timeoutMs waiting for activity.
    void poll(int timeoutMs);

    void send(int clientId, const std::string& text);

    void onMessage(MessageHandler handler) { messageHandler_ = std::move(handler); }
    void onConnect(ConnectHandler handler) { connectHandler_ = std::move(handler); }
    void onDisconnect(DisconnectHandler handler) { disconnectHandler_ = std::move(handler); }

private:
    enum class HandshakeResult { Incomplete, Success, Failed };

    struct Client {
        SOCKET socket = INVALID_SOCKET;
        bool handshakeComplete = false;
        std::string recvBuffer;
    };

    SOCKET listenSocket_ = INVALID_SOCKET;
    std::unordered_map<int, Client> clients_;
    int nextClientId_ = 1;
    bool wsaStarted_ = false;

    MessageHandler messageHandler_;
    ConnectHandler connectHandler_;
    DisconnectHandler disconnectHandler_;

    void acceptNewConnection();
    // Returns true if the client should be disconnected.
    bool serviceClient(int id, Client& client);
    HandshakeResult tryHandshake(Client& client);
    // Returns true if the client requested a close.
    bool processFrames(int id, Client& client);
    void closeClient(int id);
};
