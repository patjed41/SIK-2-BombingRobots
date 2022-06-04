#include "server_parameters.h"
#include "../err.h"

#include <cstring>
#include <iostream>
#include <arpa/inet.h>

#define MAX_STRING_LEN 256

// Returns true if "-h" parameter appeared.
static bool help_needed(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            return true;
        }
    }

    return false;
}

static void print_help() {
    std::cout << "USAGE:\n"
              << "    ./robots-server"
              << " -b <bomb_timer> -c <players_count>"
              << " -d <turn_duration> -e <explosion_radius>"
              << " -k <initial_blocks> -l <game_length>"
              << " -n <server_name> -p <port>"
              << " -s <seed> -x <size_x> -y <size_y>"
              << "\n\nOPTIONS\n"
              << "    -b <bomb_timer>\n"
              << "    -c <players_count>\n"
              << "    -d <turn_duration>\n"
              << "    -e <explosion_radius>\n"
              << "    -h <help>\n"
              << "    -k <initial_blocks>\n"
              << "    -l <game_length>\n"
              << "    -n <server_name>\n"
              << "    -p <port>\n"
              << "    -s <seed> (optional)\n"
              << "    -x <size_x>\n"
              << "    -y <size_y>\n";
}

// Checks if uint with [bits] bits represented by [str] is correct.
static bool check_uint(const char *str, uint64_t bits) {
    errno = 0;

    char *end_ptr;
    auto value = (uint64_t) strtoull(str, &end_ptr, 10);

    uint64_t max_value = ((1ull << (bits - 1ull)) - 1ull) * 2ull + 1ull; // 2^bits - 1

    return *end_ptr == '\0' && errno == 0 && value <= max_value;
}

// Reads bomb timer. Changes [parameters] reference.
static void read_bomb_timer(ServerParameters &parameters, const char *bomb_timer) {
    if (parameters.bomb_timer == 0) {
        if (!check_uint(bomb_timer, 16)) {
            fatal("Incorrect bomb timer %s.", bomb_timer);
        }
        parameters.bomb_timer = (uint16_t) strtoull(bomb_timer, nullptr, 10);
        if (parameters.bomb_timer == 0) {
            fatal("Bomb timer must be positive.");
        }
    }
}

// Reads players count. Changes [parameters] reference.
static void read_players_count(ServerParameters &parameters, const char *players_count) {
    if (parameters.players_count == 0) {
        if (!check_uint(players_count, 8)) {
            fatal("Incorrect players count %s.", players_count);
        }
        parameters.players_count = (uint8_t) strtoull(players_count, nullptr, 10);
        if (parameters.players_count == 0) {
            fatal("Players count must be positive.");
        }
    }
}

// Reads turn duration. Changes [parameters] reference.
static void read_turn_duration(ServerParameters &parameters, const char *turn_duration) {
    if (parameters.turn_duration == 0) {
        if (!check_uint(turn_duration, 64)) {
            fatal("Incorrect turn duration %s.", turn_duration);
        }
        parameters.turn_duration = (uint64_t) strtoull(turn_duration, nullptr, 10);
        if (parameters.turn_duration == 0) {
            fatal("Turn duration must be positive.");
        }
    }
}

// Reads explosion radius. Changes [parameters] reference.
static void read_explosion_radius(ServerParameters &parameters, const char *explosion_radius) {
    if (!parameters.read_explosion_radius) {
        if (!check_uint(explosion_radius, 16)) {
            fatal("Incorrect explosion radius %s.", explosion_radius);
        }
        parameters.explosion_radius = (uint16_t) strtoull(explosion_radius, nullptr, 10);
        parameters.read_explosion_radius = true;
    }
}

// Reads initial blocks. Changes [parameters] reference.
static void read_initial_blocks(ServerParameters &parameters, const char *initial_blocks) {
    if (!parameters.read_initial_blocks) {
        if (!check_uint(initial_blocks, 16)) {
            fatal("Incorrect initial blocks %s.", initial_blocks);
        }
        parameters.initial_blocks = (uint16_t) strtoull(initial_blocks, nullptr, 10);
        parameters.read_initial_blocks = true;
    }
}

// Reads game length. Changes [parameters] reference.
static void read_game_length(ServerParameters &parameters, const char *game_length) {
    if (parameters.game_length == 0) {
        if (!check_uint(game_length, 16)) {
            fatal("Incorrect game length %s.", game_length);
        }
        parameters.game_length = (uint16_t) strtoull(game_length, nullptr, 10);
        if (parameters.game_length == 0) {
            fatal("Game length must be positive.");
        }
    }
}

// Reads server name. Changes [parameters] reference.
static void read_port(ServerParameters &parameters, const char *port) {
    if (!parameters.read_port) {
        if (!check_uint(port, 16)) {
            fatal("Incorrect port %s.", port);
        }
        parameters.port = (uint16_t) strtoull(port, nullptr, 10);
        parameters.read_port = true;
    }
}

// Reads port. Changes [parameters] reference.
static void read_server_name(ServerParameters &parameters, const char *server_name) {
    if (parameters.server_name.empty()) {
        parameters.server_name = std::string(server_name);
        if (parameters.server_name.length() > MAX_STRING_LEN) {
            fatal("Server name cannot contain more that 256 characters.");
        }
    }
}

// Reads seed. Changes [parameters] reference.
static void read_seed(ServerParameters &parameters, const char *seed) {
    if (!parameters.read_seed) {
        if (!check_uint(seed, 32)) {
            fatal("Incorrect seed %s.", seed);
        }
        parameters.seed = (uint16_t) strtoull(seed, nullptr, 10);
        parameters.read_seed = true;
    }
}

// Reads size x. Changes [parameters] reference.
static void read_size_x(ServerParameters &parameters, const char *size_x) {
    if (parameters.size_x == 0) {
        if (!check_uint(size_x, 16)) {
            fatal("Incorrect size x %s.", size_x);
        }
        parameters.size_x = (uint16_t) strtoull(size_x, nullptr, 10);
        if (parameters.size_x == 0) {
            fatal("Size x must be positive.");
        }
    }
}

// Reads size y. Changes [parameters] reference.
static void read_size_y(ServerParameters &parameters, const char *size_y) {
    if (parameters.size_y == 0) {
        if (!check_uint(size_y, 16)) {
            fatal("Incorrect size y %s.", size_y);
        }
        parameters.size_y = (uint16_t) strtoull(size_y, nullptr, 10);
        if (parameters.size_y == 0) {
            fatal("Size y must be positive.");
        }
    }
}

// Processes a single parameter [option] with value [value]. Changes
// [parameters] reference.
static void read_parameter(ServerParameters &parameters, const char *option, const char *value) {
    if (strcmp(option, "-b") == 0) {
        read_bomb_timer(parameters, value);
    }
    else if (strcmp(option, "-c") == 0) {
        read_players_count(parameters, value);
    }
    else if (strcmp(option, "-d") == 0) {
        read_turn_duration(parameters, value);
    }
    else if (strcmp(option, "-e") == 0) {
        read_explosion_radius(parameters, value);
    }
    else if (strcmp(option, "-k") == 0) {
        read_initial_blocks(parameters, value);
    }
    else if (strcmp(option, "-l") == 0) {
        read_game_length(parameters, value);
    }
    else if (strcmp(option, "-n") == 0) {
        read_server_name(parameters, value);
    }
    else if (strcmp(option, "-p") == 0) {
        read_port(parameters, value);
    }
    else if (strcmp(option, "-s") == 0) {
        read_seed(parameters, value);
    }
    else if (strcmp(option, "-x") == 0) {
        read_size_x(parameters, value);
    }
    else if (strcmp(option, "-y") == 0) {
        read_size_y(parameters, value);
    }
    else {
        fatal("Incorrect parameter %s.", option);
    }
}

// Checks if all necessary parameters appeared.
static void ensure_every_parameter_read(const ServerParameters &parameters) {
    if (parameters.bomb_timer == 0) {
        fatal("-b parameter is necessary.");
    }
    if (parameters.players_count == 0) {
        fatal("-c parameter is necessary.");
    }
    if (parameters.turn_duration == 0) {
        fatal("-d parameter is necessary.");
    }
    if (!parameters.read_explosion_radius) {
        fatal("-e parameter is necessary.");
    }
    if (!parameters.read_initial_blocks) {
        fatal("-k parameter is necessary.");
    }
    if (parameters.game_length == 0) {
        fatal("-l parameter is necessary.");
    }
    if (parameters.server_name.empty()) {
        fatal("-n parameter is necessary.");
    }
    if (!parameters.read_port) {
        fatal("-p parameter is necessary.");
    }
    if (parameters.size_x == 0) {
        fatal("-x parameter is necessary.");
    }
    if (parameters.size_y == 0) {
        fatal("-y parameter is necessary.");
    }
}

ServerParameters read_parameters(int argc, char *argv[]) {
    if (help_needed(argc, argv)) {
        print_help();
        exit(0);
    }

    if (argc % 2 == 0) {
        fatal("Every parameter must have value.");
    }

    ServerParameters parameters;
    parameters.seed = (uint32_t) std::chrono::system_clock::now().time_since_epoch().count();

    for (int i = argc - 1; i > 0; i -= 2) {
        read_parameter(parameters, argv[i - 1], argv[i]);
    }

    ensure_every_parameter_read(parameters);

    return parameters;
}
