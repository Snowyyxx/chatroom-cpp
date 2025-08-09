#ifndef COMMANDS_H
#define COMMANDS_H

#include "server_types.h"
#include <string>
#include <vector>
#include <mutex>

void processCommand(const std::string& cmd, int senderFd,
                    std::vector<ClientInfo>& clients, std::mutex& clientsMutex);

#endif
