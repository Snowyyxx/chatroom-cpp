#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <string>

#define BUFFER_SIZE 1024
#define SERVER_PORT 5000

void printColor(int colorCode, const std::string& text) {
    std::cout << "\033[3" << colorCode << "m" << text << "\033[0m";
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: ./client <server_ip>\n";
        return 1;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("[ERROR] Socket creation failed");
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, argv[1], &serverAddr.sin_addr) <= 0) {
        perror("[ERROR] Invalid address");
        return 1;
    }

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("[ERROR] Connection failed");
        return 1;
    }

    std::cout << "[INFO] Connected to server at " << argv[1] << ":" << SERVER_PORT << "\n";

    struct pollfd fds[2];
    fds[0].fd = sock;
    fds[0].events = POLLIN;
    fds[1].fd = STDIN_FILENO;
    fds[1].events = POLLIN;

    char buffer[BUFFER_SIZE];

    while (true) {
        int activity = poll(fds, 2, -1);
        if (activity < 0) {
            perror("[ERROR] poll() failed");
            break;
        }

        // Incoming message from server
        if (fds[0].revents & POLLIN) {
            memset(buffer, 0, BUFFER_SIZE);
            int bytesRead = recv(sock, buffer, BUFFER_SIZE, 0);
            if (bytesRead <= 0) {
                std::cout << "[INFO] Disconnected from server.\n";
                break;
            }
            std::cout << buffer << std::flush;
        }

        // User input
        if (fds[1].revents & POLLIN) {
            std::string input;
            std::getline(std::cin, input);

            if (input.empty()) continue;

            send(sock, input.c_str(), input.size(), 0);

            if (input == "/quit") {
                std::cout << "[INFO] Exiting client.\n";
                break;
            }
        }
    }

    close(sock);
    return 0;
}
