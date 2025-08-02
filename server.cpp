// server.cpp
#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <mutex>

#include "commands.h"
#include "server_types.h"

using namespace std;

// globals (also declared extern in server_types.h)
const int MAX_LEN    = 200;
const int NUM_COLORS = 6;
const string def_col = "\033[0m";
const string colors[NUM_COLORS] = {
    "\033[31m", "\033[32m", "\033[33m",
    "\033[34m", "\033[35m", "\033[36m"
};

vector<ClientInfo> clients;
mutex clients_mtx, cout_mtx;
int seed = 0;

// forward declarations
int  get_client_index(int id);
void shared_print(const string& str, bool endLine=true);
int  broadcast_message(const string& message, int sender_id);
int  broadcast_message(int num, int sender_id);
void end_connection(int id);
void handle_client(int client_socket, int id);

int main() {
    // Prepare command subsystem
    Commands_Init();

    // 1) set up listening socket
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) { perror("socket"); exit(-1); }

    sockaddr_in srv_addr{};
    srv_addr.sin_family      = AF_INET;
    srv_addr.sin_port        = htons(10000);
    srv_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&srv_addr.sin_zero, 0);

    if (bind(server_sock, (sockaddr*)&srv_addr, sizeof(srv_addr)) == -1) {
        perror("bind"); exit(-1);
    }
    if (listen(server_sock, 8) == -1) {
        perror("listen"); exit(-1);
    }

    cout << colors[NUM_COLORS-1]
         << "\n\t  ====== Welcome to the chat-room ======   "
         << def_col << endl;

    // 2) accept loop
    while (true) {
        sockaddr_in cli_addr{};
        socklen_t    len = sizeof(cli_addr);
        int client_sock = accept(server_sock,
                                 (sockaddr*)&cli_addr, &len);
        if (client_sock == -1) {
            perror("accept"); continue;
        }
        seed++;
        thread t(handle_client, client_sock, seed);

        lock_guard<mutex> guard(clients_mtx);
        clients.push_back({ seed,
                            "Anonymous",
                            client_sock,
                            move(t),
                            (seed-1) % NUM_COLORS });
    }

    close(server_sock);
    return 0;
}

int get_client_index(int id) {
    lock_guard<mutex> guard(clients_mtx);
    for (int i = 0; i < (int)clients.size(); i++)
        if (clients[i].id == id) return i;
    return -1;
}

void shared_print(const string& str, bool endLine) {
    lock_guard<mutex> guard(cout_mtx);
    cout << str;
    if (endLine) cout << endl;
}

int broadcast_message(const string& message, int sender_id) {
    char buf[MAX_LEN] = {0};
    strncpy(buf, message.c_str(), MAX_LEN - 1);
    for (auto& c : clients) {
        if (c.id != sender_id)
            send(c.socket, buf, MAX_LEN, 0);
    }
    return 0;
}

int broadcast_message(int num, int sender_id) {
    for (auto& c : clients) {
        if (c.id != sender_id)
            send(c.socket, &num, sizeof(num), 0);
    }
    return 0;
}

void end_connection(int id) {
    lock_guard<mutex> guard(clients_mtx);
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        if (it->id == id) {
            it->th.detach();
            close(it->socket);
            clients.erase(it);
            break;
        }
    }
}

void handle_client(int client_socket, int id) {
    char name_buf[MAX_LEN], str_buf[MAX_LEN];
    // 1) initial name
    recv(client_socket, name_buf, MAX_LEN, 0);
    {
        lock_guard<mutex> guard(clients_mtx);
        int idx = get_client_index(id);
        if (idx >= 0) clients[idx].name = name_buf;
    }
    int idx = get_client_index(id);
    int my_col = clients[idx].color_code;
    string join_msg = clients[idx].name + " has joined";

    broadcast_message("#NULL", id);
    broadcast_message(my_col, id);
    broadcast_message(join_msg, id);
    shared_print(colors[my_col] + join_msg + def_col);

    // 2) message loop
    while (true) {
        int bytes = recv(client_socket, str_buf, MAX_LEN, 0);
        if (bytes <= 0) {
            end_connection(id);
            return;
        }
        string msg(str_buf);

        if (msg == "#exit") {
            string leave = clients[idx].name + " has left";
            broadcast_message("#NULL", id);
            broadcast_message(my_col, id);
            broadcast_message(leave, id);
            shared_print(colors[my_col] + leave + def_col);
            end_connection(id);
            return;
        }

        // Slash‐commands
        if (!msg.empty() && msg[0] == '/') {
            if (Commands_Handle(msg, id))
                continue;
        }

        // Normal broadcast
        idx   = get_client_index(id);
        my_col = clients[idx].color_code;
        broadcast_message(clients[idx].name, id);
        broadcast_message(my_col,             id);
        broadcast_message(msg,               id);
        shared_print(colors[my_col] + clients[idx].name
                     + " : " + def_col + msg);
    }
}
