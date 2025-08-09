#ifndef SERVER_TYPES_H
#define SERVER_TYPES_H

#include <string>

struct ClientInfo {
    int sockfd;
    std::string name;
};

#endif
