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
#include <algorithm>

#define MAX_CLIENTS 50
#define BUFFER_SIZE 1024
#define SERVER_PORT 5000

std::vector<ClientInfo> clients;
std::mutex clientsMutex;

std::queue<std::string> logQueue;
std::mutex logMutex;
bool running = true;

void* loggingThread(void* arg) {
    std::ofstream logFile("chat_log.txt", std::ios::app);
    while (running) {
        std::string msg;
        {
            std::lock_guard<std::mutex> lock(logMutex);
            if (!logQueue.empty()) {
                msg = logQueue.front();
                logQueue.pop();
            }
        }
        if (!msg.empty()) {
            logFile << msg << "\n";
            logFile.flush();
        }
        usleep(10000);
    }
    return nullptr;
}

void broadcastMessage(const std::string& msg, int excludeFd = -1) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    for (auto& client : clients) {
        if (client.sockfd != excludeFd) {
            send(client.sockfd, msg.c_str(), msg.size(), 0);
        }
    }
}

void* clientHandler(void* arg) {
    int clientSock = *(int*)arg;
    char buffer[BUFFER_SIZE];
    std::string name = "User" + std::to_string(clientSock);

    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        clients.push_back({clientSock, name});
    }

    broadcastMessage(name + " has joined\n", clientSock);

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytesRead = recv(clientSock, buffer, BUFFER_SIZE - 1, 0);
        if (bytesRead <= 0) break;

        std::string msg(buffer);
        std::vector<std::string> userNames;
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            for (auto& c : clients) userNames.push_back(c.name);
        }

        std::string cmdResponse;
        if (handleCommand(msg, clientSock, userNames, cmdResponse)) {
            send(clientSock, cmdResponse.c_str(), cmdResponse.size(), 0);
        } else {
            std::string fullMsg = name + ": " + msg;
            {
                std::lock_guard<std::mutex> lock(logMutex);
                logQueue.push(fullMsg);
            }
            broadcastMessage(fullMsg, clientSock);
        }

        if (msg == "/quit") break;
    }

    close(clientSock);
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        clients.erase(std::remove_if(clients.begin(), clients.end(),
                                     [clientSock](const ClientInfo& c) { return c.sockfd == clientSock; }),
                      clients.end());
    }
    broadcastMessage(name + " has left\n");
    return nullptr;
}

int main() {
    int serverSock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSock, MAX_CLIENTS);

    pthread_t logThread;
    pthread_create(&logThread, nullptr, loggingThread, nullptr);

    std::cout << "Server started on port " << SERVER_PORT << "\n";

    struct pollfd fds[MAX_CLIENTS];
    fds[0].fd = serverSock;
    fds[0].events = POLLIN;
    int nfds = 1;

    while (true) {
        poll(fds, nfds, -1);
        if (fds[0].revents & POLLIN) {
            int clientSock = accept(serverSock, nullptr, nullptr);
            pthread_t t;
            pthread_create(&t, nullptr, clientHandler, &clientSock);
            pthread_detach(t);
        }
    }

    running = false;
    pthread_join(logThread, nullptr);
    close(serverSock);
    return 0;
}
