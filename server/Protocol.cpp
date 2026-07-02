#include "Protocol.h"

namespace protocol {

Command parse(const std::string& message) {
    Command cmd;

    if (message == "START") {
        cmd.type = CommandType::Start;
        return cmd;
    }
    if (message == "RETRY") {
        cmd.type = CommandType::Retry;
        return cmd;
    }

    const std::string prefix = "INPUT ";
    if (message.rfind(prefix, 0) == 0 && message.size() >= prefix.size() + 5) {
        const char* bits = message.c_str() + prefix.size();
        cmd.type = CommandType::Input;
        cmd.input.left = bits[0] == '1';
        cmd.input.right = bits[1] == '1';
        cmd.input.up = bits[2] == '1';
        cmd.input.down = bits[3] == '1';
        cmd.input.fire = bits[4] == '1';
        return cmd;
    }

    return cmd;
}

} // namespace protocol
