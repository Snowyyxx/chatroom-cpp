// commands.cpp
#include "commands.h"
#include "server_types.h"

#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>  // for send()
#include <mutex>

static const int SYS_COLOR = NUM_COLORS - 1;

void Commands_Init() {
    // no-op
}

static void reply_sys(int sock, const std::string& text) {
    char buf[MAX_LEN] = {0};
    std::strncpy(buf, text.c_str(), MAX_LEN - 1);
    send(sock, "#NULL", MAX_LEN, 0);
    send(sock, &SYS_COLOR, sizeof(SYS_COLOR), 0);
    send(sock, buf, MAX_LEN, 0);
}

bool Commands_Handle(const std::string& input, int sender_id) {
    int idx = get_client_index(sender_id);
    if (idx < 0) return false;
    int sock = clients[idx].socket;

    if (input == "/help") {
        std::string h =
            "Available commands:\n"
            " /help                - Show this list\n"
            " /users               - List online users\n"
            " /rename <new_name>   - Change your name\n"
            " /msg <user> <msg>    - Private message\n"
            " /color <0-5>         - Change your color\n";
        reply_sys(sock, h);
        return true;
    }

    if (input == "/users") {
        std::ostringstream oss;
        oss << "Online users:\n";
        {
            std::lock_guard<std::mutex> guard(clients_mtx);
            for (auto& c : clients)
                oss << "  - " << c.name << "\n";
        }
        reply_sys(sock, oss.str());
        return true;
    }

    if (input.rfind("/rename ", 0) == 0) {
        std::string new_name = input.substr(8);
        if (new_name.empty()) {
            reply_sys(sock, "Usage: /rename <new_name>");
            return true;
        }
        bool taken = false;
        {
            std::lock_guard<std::mutex> guard(clients_mtx);
            for (auto& c : clients)
                if (c.name == new_name) { taken = true; break; }
            if (!taken) {
                std::string old = clients[idx].name;
                clients[idx].name = new_name;
                std::string note = old + " is now " + new_name;
                broadcast_message("#NULL", sender_id);
                broadcast_message(SYS_COLOR, sender_id);
                broadcast_message(note, sender_id);
                shared_print(colors[SYS_COLOR] + note + def_col, true);
                reply_sys(sock, "You are now \"" + new_name + "\"");
            }
        }
        if (taken)
            reply_sys(sock, "Name \"" + new_name + "\" is already taken.");
        return true;
    }

    if (input.rfind("/msg ", 0) == 0) {
        std::istringstream iss(input.substr(5));
        std::string target, word, body;
        iss >> target;
        while (iss >> word) body += word + " ";
        if (target.empty() || body.empty()) {
            reply_sys(sock, "Usage: /msg <user> <message>");
            return true;
        }
        int tgt_idx = -1;
        {
            std::lock_guard<std::mutex> guard(clients_mtx);
            for (int i = 0; i < (int)clients.size(); i++)
                if (clients[i].name == target) { tgt_idx = i; break; }
        }
        if (tgt_idx < 0) {
            reply_sys(sock, "User \"" + target + "\" not found.");
        } else {
            char nb[MAX_LEN] = {0}, mb[MAX_LEN] = {0};
            std::strncpy(nb, clients[idx].name.c_str(), MAX_LEN - 1);
            std::string pref = "(Private) " + body;
            std::strncpy(mb, pref.c_str(), MAX_LEN - 1);
            int col = clients[idx].color_code;

            send(clients[tgt_idx].socket, nb, MAX_LEN, 0);
            send(clients[tgt_idx].socket, &col, sizeof(col), 0);
            send(clients[tgt_idx].socket, mb, MAX_LEN, 0);

            reply_sys(sock, "Sent to " + target + ": " + body);
        }
        return true;
    }

    if (input.rfind("/color ", 0) == 0) {
        try {
            int c = std::stoi(input.substr(7));
            if (c < 0 || c >= NUM_COLORS) throw std::out_of_range("");
            {
                std::lock_guard<std::mutex> guard(clients_mtx);
                clients[idx].color_code = c;
            }
            reply_sys(sock, "Color set to code " + std::to_string(c));
        } catch (...) {
            reply_sys(sock, "Invalid code; use 0–" + std::to_string(NUM_COLORS - 1));
        }
        return true;
    }

    return false;
}
