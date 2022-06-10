#ifndef CLIENT_PARAMETERS_H
#define CLIENT_PARAMETERS_H

#include <sys/types.h>
#include <string>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>

// Struct containing information from command line parameters.
struct ClientParameters {
    std::string gui_address;
    uint16_t gui_port;
    std::string player_name;
    uint16_t port;
    bool read_port = false;
    std::string server_address;
    uint16_t server_port;
};

// Processes command line parameters and returns ClientParameters instance.
// If the same parameter appears more than once, the last occurrence is taken
// into account.
ClientParameters read_parameters(int argc, char *argv[]);

#endif // CLIENT_PARAMETERS_H
