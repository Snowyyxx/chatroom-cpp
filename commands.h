#ifndef COMMANDS_H
#define COMMANDS_H

#include <string>
#include <vector>

bool handleCommand(const std::string& msg, int clientSock, std::vector<std::string>& usersList, std::string& response);

#endif
