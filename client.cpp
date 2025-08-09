#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <iostream>
#include <string>
#include <cstring>

#define BUFFER_SIZE 1024
#define SERVER_PORT 5000

int sockfd;

void* receiveMessages(void* arg) {
    char buffer[BUFFER_SIZE];
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytesRead = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
        if (bytesRead <= 0) break;
        std::cout << buffer;
    }
    return nullptr;
}

int main() {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

    pthread_t recvThread;
    pthread_create(&recvThread, nullptr, receiveMessages, nullptr);

    std::string msg;
    while (true) {
        std::getline(std::cin, msg);
        send(sockfd, msg.c_str(), msg.size(), 0);
        if (msg == "/quit") break;
    }

    pthread_cancel(recvThread);
    close(sockfd);
    return 0;
}
