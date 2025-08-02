// server_types.h
#ifndef SERVER_TYPES_H
#define SERVER_TYPES_H

#include <thread>
#include <string>
#include <vector>
#include <mutex>

// One entry per connected client
struct ClientInfo {
    int          id;
    std::string  name;
    int          socket;
    std::thread  th;
    int          color_code;
};

// Globals (defined in server.cpp):
extern const int              MAX_LEN;
extern const int              NUM_COLORS;
extern const std::string      def_col;
extern const std::string      colors[];

extern std::vector<ClientInfo> clients;
extern std::mutex               clients_mtx;
extern std::mutex               cout_mtx;

// Helper functions (also defined in server.cpp):
int  get_client_index(int id);
int  broadcast_message(const std::string& message, int sender_id);
int  broadcast_message(int num, int sender_id);
void shared_print(const std::string& str, bool endLine = true);

#endif // SERVER_TYPES_H
