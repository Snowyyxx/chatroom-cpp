#ifndef SERVER_TYPES_H
#define SERVER_TYPES_H

#include <string>
#include <vector>
#include <mutex>

struct ClientInfo {
    int fd;
    std::string name;
    int colorCode; // 0-5
};

#endif
