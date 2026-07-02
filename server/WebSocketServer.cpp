#include "WebSocketServer.h"
#include "Base64.h"
#include "Sha1.h"

#include <algorithm>
#include <cctype>
#include <cstring>

#pragma comment(lib, "ws2_32.lib")

namespace {

const char kWebSocketGuid[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

std::string toLower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                    [](unsigned char c) { return std::tolower(c); });
    return out;
}

std::string trim(const std::string& s) {
    size_t begin = s.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(begin, end - begin + 1);
}

// Finds the value of an HTTP header (case-insensitive name) in a raw request.
bool findHeaderValue(const std::string& request, const std::string& name, std::string& outValue) {
    std::string lowerRequest = toLower(request);
    std::string lowerName = toLower(name) + ":";
    size_t pos = lowerRequest.find(lowerName);
    if (pos == std::string::npos) return false;
    size_t lineEnd = request.find("\r\n", pos);
    if (lineEnd == std::string::npos) lineEnd = request.size();
    outValue = trim(request.substr(pos + lowerName.size(), lineEnd - (pos + lowerName.size())));
    return true;
}

std::string encodeFrame(const std::string& payload) {
    std::string frame;
    frame.push_back(static_cast<char>(0x81)); // FIN + text opcode

    size_t len = payload.size();
    if (len <= 125) {
        frame.push_back(static_cast<char>(len));
    } else if (len <= 0xFFFF) {
        frame.push_back(static_cast<char>(126));
        frame.push_back(static_cast<char>((len >> 8) & 0xFF));
        frame.push_back(static_cast<char>(len & 0xFF));
    } else {
        frame.push_back(static_cast<char>(127));
        for (int i = 7; i >= 0; --i) {
            frame.push_back(static_cast<char>((static_cast<uint64_t>(len) >> (i * 8)) & 0xFF));
        }
    }

    frame += payload;
    return frame;
}

std::string encodeCloseFrame() {
    std::string frame;
    frame.push_back(static_cast<char>(0x88));
    frame.push_back(static_cast<char>(0x00));
    return frame;
}

std::string encodePongFrame(const std::string& payload) {
    std::string frame;
    frame.push_back(static_cast<char>(0x8A));
    frame.push_back(static_cast<char>(payload.size() & 0x7F));
    frame += payload;
    return frame;
}

} // namespace

WebSocketServer::WebSocketServer() = default;

WebSocketServer::~WebSocketServer() {
    stop();
}

bool WebSocketServer::start(unsigned short port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return false;
    }
    wsaStarted_ = true;

    listenSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket_ == INVALID_SOCKET) return false;

    int reuse = 1;
    setsockopt(listenSocket_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(listenSocket_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        return false;
    }
    if (listen(listenSocket_, SOMAXCONN) == SOCKET_ERROR) {
        return false;
    }

    return true;
}

void WebSocketServer::stop() {
    for (auto& [id, client] : clients_) {
        closesocket(client.socket);
    }
    clients_.clear();

    if (listenSocket_ != INVALID_SOCKET) {
        closesocket(listenSocket_);
        listenSocket_ = INVALID_SOCKET;
    }
    if (wsaStarted_) {
        WSACleanup();
        wsaStarted_ = false;
    }
}

void WebSocketServer::poll(int timeoutMs) {
    if (listenSocket_ == INVALID_SOCKET) return;

    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(listenSocket_, &readSet);
    for (auto& [id, client] : clients_) {
        FD_SET(client.socket, &readSet);
    }

    timeval tv{};
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;

    int ready = select(0, &readSet, nullptr, nullptr, &tv);
    if (ready <= 0) return;

    if (FD_ISSET(listenSocket_, &readSet)) {
        acceptNewConnection();
    }

    std::vector<int> toClose;
    for (auto& [id, client] : clients_) {
        if (FD_ISSET(client.socket, &readSet)) {
            if (serviceClient(id, client)) {
                toClose.push_back(id);
            }
        }
    }

    for (int id : toClose) {
        closeClient(id);
    }
}

void WebSocketServer::acceptNewConnection() {
    sockaddr_in clientAddr{};
    int addrLen = sizeof(clientAddr);
    SOCKET s = accept(listenSocket_, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
    if (s == INVALID_SOCKET) return;

    int id = nextClientId_++;
    Client client;
    client.socket = s;
    clients_[id] = client;
}

bool WebSocketServer::serviceClient(int id, Client& client) {
    char buf[4096];
    int n = recv(client.socket, buf, sizeof(buf), 0);
    if (n <= 0) {
        return true;
    }
    client.recvBuffer.append(buf, n);

    if (!client.handshakeComplete) {
        HandshakeResult result = tryHandshake(client);
        if (result == HandshakeResult::Failed) return true;
        if (result == HandshakeResult::Success) {
            client.handshakeComplete = true;
            if (connectHandler_) connectHandler_(id);
        }
        return false;
    }

    return processFrames(id, client);
}

WebSocketServer::HandshakeResult WebSocketServer::tryHandshake(Client& client) {
    size_t headerEnd = client.recvBuffer.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        return HandshakeResult::Incomplete;
    }

    std::string request = client.recvBuffer.substr(0, headerEnd);
    client.recvBuffer.erase(0, headerEnd + 4);

    std::string key;
    if (!findHeaderValue(request, "Sec-WebSocket-Key", key)) {
        return HandshakeResult::Failed;
    }

    std::string accept = base64::encode(sha1::digest(key + kWebSocketGuid));

    std::string response =
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: " + accept + "\r\n"
        "\r\n";

    ::send(client.socket, response.data(), static_cast<int>(response.size()), 0);
    return HandshakeResult::Success;
}

bool WebSocketServer::processFrames(int id, Client& client) {
    std::string& buf = client.recvBuffer;

    while (true) {
        if (buf.size() < 2) return false;

        unsigned char byte0 = static_cast<unsigned char>(buf[0]);
        unsigned char byte1 = static_cast<unsigned char>(buf[1]);

        int opcode = byte0 & 0x0F;
        bool masked = (byte1 & 0x80) != 0;
        uint64_t payloadLen = byte1 & 0x7F;

        size_t pos = 2;
        if (payloadLen == 126) {
            if (buf.size() < pos + 2) return false;
            payloadLen = (static_cast<unsigned char>(buf[pos]) << 8) | static_cast<unsigned char>(buf[pos + 1]);
            pos += 2;
        } else if (payloadLen == 127) {
            if (buf.size() < pos + 8) return false;
            payloadLen = 0;
            for (int i = 0; i < 8; ++i) {
                payloadLen = (payloadLen << 8) | static_cast<unsigned char>(buf[pos + i]);
            }
            pos += 8;
        }

        unsigned char maskKey[4] = {0, 0, 0, 0};
        if (masked) {
            if (buf.size() < pos + 4) return false;
            for (int i = 0; i < 4; ++i) {
                maskKey[i] = static_cast<unsigned char>(buf[pos + i]);
            }
            pos += 4;
        }

        if (buf.size() < pos + payloadLen) return false;

        std::string payload = buf.substr(pos, static_cast<size_t>(payloadLen));
        if (masked) {
            for (size_t i = 0; i < payload.size(); ++i) {
                payload[i] = static_cast<char>(static_cast<unsigned char>(payload[i]) ^ maskKey[i % 4]);
            }
        }

        buf.erase(0, pos + static_cast<size_t>(payloadLen));

        if (opcode == 0x8) { // close
            ::send(client.socket, encodeCloseFrame().data(), 2, 0);
            return true;
        } else if (opcode == 0x9) { // ping
            std::string pong = encodePongFrame(payload);
            ::send(client.socket, pong.data(), static_cast<int>(pong.size()), 0);
        } else if (opcode == 0x1) { // text
            if (messageHandler_) messageHandler_(id, payload);
        }
        // opcode 0xA (pong) and others are ignored.
    }
}

void WebSocketServer::send(int clientId, const std::string& text) {
    auto it = clients_.find(clientId);
    if (it == clients_.end() || !it->second.handshakeComplete) return;

    std::string frame = encodeFrame(text);
    ::send(it->second.socket, frame.data(), static_cast<int>(frame.size()), 0);
}

void WebSocketServer::closeClient(int id) {
    auto it = clients_.find(id);
    if (it == clients_.end()) return;

    closesocket(it->second.socket);
    clients_.erase(it);

    if (disconnectHandler_) disconnectHandler_(id);
}
