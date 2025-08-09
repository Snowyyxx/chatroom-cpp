#include "commands.h"
#include <sstream>

bool handleCommand(const std::string& msg, int clientSock, std::vector<std::string>& usersList, std::string& response) {
    if (msg == "/users") {
        response = "Online users:\n";
        for (auto& name : usersList) {
            response += name + "\n";
        }
        return true;
    }
    return false; // Not a command we handle here
}
