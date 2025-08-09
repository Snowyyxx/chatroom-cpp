#include "commands.h"
#include <sys/socket.h>
#include <algorithm>
#include <sstream>
#include <iostream>

static void sendToClient(int fd, const std::string& msg) {
    send(fd, msg.c_str(), msg.size(), 0);
}

void processCommand(const std::string& cmd, int senderFd,
                    std::vector<ClientInfo>& clients, std::mutex& clientsMutex) {
    std::istringstream iss(cmd);
    std::string command;
    iss >> command;

    if (command == "/help") {
        sendToClient(senderFd, "Commands: /help /users /rename <name> /msg <user> <message> /color <0-5> /quit\n");
    }
    else if (command == "/users") {
        std::lock_guard<std::mutex> lock(clientsMutex);
        std::string list = "Online users:\n";
        for (auto& c : clients) {
            list += "- " + c.name + "\n";
        }
        sendToClient(senderFd, list);
    }
    else if (command == "/rename") {
        std::string newName;
        iss >> newName;
        if (!newName.empty()) {
            std::lock_guard<std::mutex> lock(clientsMutex);
            for (auto& c : clients) {
                if (c.fd == senderFd) {
                    c.name = newName;
                    sendToClient(senderFd, "Name changed successfully.\n");
                    return;
                }
            }
        }
    }
    else if (command == "/msg") {
        std::string targetUser;
        iss >> targetUser;
        std::string message;
        std::getline(iss, message);
        message.erase(0, message.find_first_not_of(" "));

        std::lock_guard<std::mutex> lock(clientsMutex);
        auto it = std::find_if(clients.begin(), clients.end(),
            [&](const ClientInfo& c) { return c.name == targetUser; });
        if (it != clients.end()) {
            sendToClient(it->fd, "[PM from " + std::to_string(senderFd) + "]: " + message + "\n");
        } else {
            sendToClient(senderFd, "User not found.\n");
        }
    }
    else if (command == "/color") {
        int color;
        iss >> color;
        if (color >= 0 && color <= 5) {
            std::lock_guard<std::mutex> lock(clientsMutex);
            for (auto& c : clients) {
                if (c.fd == senderFd) {
                    c.colorCode = color;
                    sendToClient(senderFd, "Color changed.\n");
                    return;
                }
            }
        }
    }
    else if (command == "/quit") {
        sendToClient(senderFd, "Goodbye!\n");
        shutdown(senderFd, SHUT_RDWR);
        close(senderFd);
    }
    else {
        sendToClient(senderFd, "Unknown command. Type /help for help.\n");
    }
}
