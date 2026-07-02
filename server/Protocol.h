#pragma once
#include <string>

#include "GameState.h"

// Wire format between client and server. Deliberately not JSON on the way
// in -- the client only ever sends a handful of known shapes, so a tiny
// hand-rolled parser here avoids pulling in a JSON library just for this.
// Server -> client state snapshots are still JSON (see GameSession::toJson).
namespace protocol {

enum class CommandType { Start, Retry, Input, Unknown };

struct Command {
    CommandType type = CommandType::Unknown;
    game::InputState input;
};

// Recognized messages:
//   "START"                          -- begin a new game
//   "RETRY"                          -- restart after game over
//   "INPUT <l><r><u><d><f>"          -- five '0'/'1' digits, e.g. "INPUT 10001"
Command parse(const std::string& message);

} // namespace protocol
