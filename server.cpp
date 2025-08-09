#include "server_types.h"
#include "commands.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <mutex>
#include <queue>

#define MAX_CLIENTS 50
#define BUFFER_SIZE 1024
#define SERVER_PORT 5000

std::vector<ClientInfo> clients;
std::mutex clientsMutex;

// Logging queue + mutex
std::queue<std::string> logQueue;
std::mutex logMutex;
bool running = true;

void* loggingThread(void* arg) {
    std::ofstream logFile("chatroom.log", std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "[ERROR] Could not open log file.\n";
        return nullptr;
    }

    while (running) {
        std::string entry;
        {
            std::lock_guard<std::mutex> lock(logMutex);
            if (!logQueue.empty()) {
                entry = logQueue.front();
                logQueue.pop();
            }
        }
        if (!entry.empty()) {
            logFile << entry << std::endl;
            logFile.flush();
        }
        usleep(10000); // 10ms
    }
    logFile.close();
    return nullptr;
}

void broadcastMessage(const std::string& msg, int senderFd) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    for (auto& client : clients) {
        if (client.fd != senderFd) {
            send(client.fd, msg.c_str(), msg.size(), 0);
        }
    }
}

void removeClient(int fd) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        if (it->fd == fd) {
            close(it->fd);
            clients.erase(it);
            break;
        }
    }
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("[ERROR] Socket creation failed");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(SERVER_PORT);

    if (bind(server_fd, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("[ERROR] Bind failed");
        return 1;
    }

    if (listen(server_fd, 5) < 0) {
        perror("[ERROR] Listen failed");
        return 1;
    }

    std::cout << "[INFO] Server started on port " << SERVER_PORT << "\n";

    // Start logging thread
    pthread_t logThread;
    pthread_create(&logThread, nullptr, loggingThread, nullptr);

    struct pollfd fds[MAX_CLIENTS + 1];
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    int nfds = 1;

    while (true) {
        int activity = poll(fds, nfds, -1);
        if (activity < 0) {
            perror("[ERROR] poll() failed");
            break;
        }

        // New connection
        if (fds[0].revents & POLLIN) {
            sockaddr_in clientAddr{};
            socklen_t clientLen = sizeof(clientAddr);
            int newFd = accept(server_fd, (sockaddr*)&clientAddr, &clientLen);
            if (newFd < 0) {
                perror("[ERROR] accept() failed");
                continue;
            }

            std::string welcome = "Welcome to Chatroom!\n";
            send(newFd, welcome.c_str(), welcome.size(), 0);

            {
                std::lock_guard<std::mutex> lock(clientsMutex);
                clients.push_back({newFd, "User" + std::to_string(newFd), 0});
            }

            fds[nfds].fd = newFd;
            fds[nfds].events = POLLIN;
            nfds++;

            std::string logEntry = "[CONNECT] Client FD " + std::to_string(newFd) + " joined.";
            {
                std::lock_guard<std::mutex> lock(logMutex);
                logQueue.push(logEntry);
            }
        }

        // Client messages
        for (int i = 1; i < nfds; ++i) {
            if (fds[i].revents & POLLIN) {
                char buffer[BUFFER_SIZE] = {0};
                int bytesRead = recv(fds[i].fd, buffer, BUFFER_SIZE, 0);

                if (bytesRead <= 0) {
                    std::string logEntry = "[DISCONNECT] Client FD " + std::to_string(fds[i].fd) + " left.";
                    {
                        std::lock_guard<std::mutex> lock(logMutex);
                        logQueue.push(logEntry);
                    }
                    removeClient(fds[i].fd);
                    fds[i] = fds[nfds - 1];
                    nfds--;
                    i--;
                } else {
                    buffer[bytesRead] = '\0';
                    std::string msg(buffer);

                    if (!msg.empty() && msg.back() == '\n')
                        msg.pop_back();

                    // Process commands
                    if (!msg.empty() && msg[0] == '/') {
                        processCommand(msg, fds[i].fd, clients, clientsMutex);
                    } else {
                        std::string senderName;
                        int colorCode = 0;
                        {
                            std::lock_guard<std::mutex> lock(clientsMutex);
                            for (auto& c : clients) {
                                if (c.fd == fds[i].fd) {
                                    senderName = c.name;
                                    colorCode = c.colorCode;
                                    break;
                                }
                            }
                        }
                        std::string coloredMsg = "\033[3" + std::to_string(colorCode) + "m" + senderName + ": " + msg + "\033[0m\n";

                        std::string logEntry = "[MSG] " + senderName + ": " + msg;
                        {
                            std::lock_guard<std::mutex> lock(logMutex);
                            logQueue.push(logEntry);
                        }
                        broadcastMessage(coloredMsg, fds[i].fd);
                    }
                }
            }
        }
    }

    running = false;
    pthread_join(logThread, nullptr);
    close(server_fd);
    return 0;
}
