#ifndef SERVER_PARAMETERS_H
#define SERVER_PARAMETERS_H

#include <sys/types.h>
#include <string>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <chrono>

// Struct containing information from command line parameters.
struct ServerParameters {
    uint16_t bomb_timer = 0;
    uint8_t players_count = 0;
    uint64_t turn_duration = 0;
    uint16_t explosion_radius = false;
    bool read_explosion_radius = 0;
    uint16_t initial_blocks = 0;
    bool read_initial_blocks = false;
    uint16_t game_length = 0;
    std::string server_name;
    uint16_t port = 0;
    bool read_port = false;
    uint32_t seed = 0;
    bool read_seed = false;
    uint16_t size_x = 0;
    uint16_t size_y = 0;
};

// Processes command line parameters and returns ServerParameters instance.
// If the same parameter appears more than once, the last occurrence is taken
// into account.
ServerParameters read_parameters(int argc, char *argv[]);

#endif // SERVER_PARAMETERS_H
