CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra
LDLIBS = -lws2_32

SOURCES = main.cpp GameServer.cpp GameState.cpp Protocol.cpp WebSocketServer.cpp Sha1.cpp Base64.cpp
TARGET = game_server.exe

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET) $(LDLIBS)

clean:
	rm -f $(TARGET)

.PHONY: clean
