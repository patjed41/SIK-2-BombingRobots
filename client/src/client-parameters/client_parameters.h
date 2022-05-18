#ifndef CLIENT_PARAMETERS_H
#define CLIENT_PARAMETERS_H

#include <sys/types.h>
#include <string>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>

struct ClientParameters {
    std::string gui_address;
    uint16_t gui_port;
    std::string player_name;
    uint16_t port;
    std::string server_address;
    uint16_t server_port;
};

ClientParameters read_parameters(int argc, char *argv[]);

#endif //CLIENT_PARAMETERS_H
