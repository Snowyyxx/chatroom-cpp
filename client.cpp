// client.cpp
#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <signal.h>
#define MAX_LEN 200
#define NUM_COLORS 6

using namespace std;

bool exit_flag = false;
thread t_send, t_recv;
int client_socket;
string def_col = "\033[0m";
string colors[] = {
    "\033[31m", "\033[32m",
    "\033[33m", "\033[34m",
    "\033[35m", "\033[36m"
};

void catch_ctrl_c(int);
void eraseText(int);
void send_message(int);
void recv_message(int);

int main() {
    // 1) socket
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket"); exit(-1);
    }

    // 2) connect
    sockaddr_in server_addr{};
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(10000);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&server_addr.sin_zero, 0);

    if (connect(client_socket,
                (sockaddr*)&server_addr,
                sizeof(server_addr)) == -1) {
        perror("connect"); exit(-1);
    }

    signal(SIGINT, catch_ctrl_c);

    char name[MAX_LEN];
    cout << "Enter your name : ";
    cin.getline(name, MAX_LEN);
    send(client_socket, name, sizeof(name), 0);

    cout << colors[NUM_COLORS-1]
         << "\n\t  ====== Welcome to the chat-room ======   "
         << def_col << endl;
    cout << "Type /help for a list of commands." << endl;

    t_send = thread(send_message, client_socket);
    t_recv = thread(recv_message, client_socket);

    t_send.join();
    t_recv.join();
    return 0;
}

void catch_ctrl_c(int) {
    char exit_cmd[] = "#exit";
    send(client_socket, exit_cmd, sizeof(exit_cmd), 0);
    exit_flag = true;
    t_send.detach();
    t_recv.detach();
    close(client_socket);
    exit(0);
}

void eraseText(int cnt) {
    while (cnt--) cout << '\b';
}

void send_message(int sock) {
    while (!exit_flag) {
        cout << colors[1] << "You : " << def_col;
        char msg[MAX_LEN];
        cin.getline(msg, MAX_LEN);
        send(sock, msg, sizeof(msg), 0);
        if (strcmp(msg, "#exit") == 0) {
            exit_flag = true;
            t_recv.detach();
            close(sock);
            return;
        }
    }
}

void recv_message(int sock) {
    while (!exit_flag) {
        char name[MAX_LEN], msg[MAX_LEN];
        int color_code;
        int bytes = recv(sock, name, sizeof(name), 0);
        if (bytes <= 0) continue;
        recv(sock, &color_code, sizeof(color_code), 0);
        recv(sock, msg, sizeof(msg), 0);

        eraseText(6);
        if (strcmp(name, "#NULL") != 0) {
            cout << colors[color_code]
                 << name << " : " << def_col << msg << endl;
        } else {
            cout << colors[color_code] << msg << endl;
        }
        cout << colors[1] << "You : " << def_col;
        fflush(stdout);
    }
}
