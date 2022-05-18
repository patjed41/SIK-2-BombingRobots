#include "client_parameters.h"
#include "../err.h"

#include <cstring>
#include <iostream>
#include <arpa/inet.h>

#define MIN_PORT 1
#define MAX_PORT 65535

static bool help_needed(int argc, char *argv[]) {
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            return true;
        }
    }

    return false;
}

static void print_help() {
    std::cout << "USAGE:\n"
              << "    ./robots-client"
              << " -d <gui_address:gui_port> -n <player_name>"
              << " -p <port> -s <server_address:server_port>"
              << "\n\nOPTIONS\n"
              << "    -d <gui_address:gui_port>\n"
              << "    -h <help>\n"
              << "    -n <player_name>\n"
              << "    -p <port>\n"
              << "    -s <server_address:server_port>\n";
}

static bool check_port(const char *port_str) {
    errno = 0;

    char *end_ptr;
    uint32_t value = (uint32_t)strtoul(port_str, &end_ptr, 10);

    return *end_ptr == '\0' && errno == 0 && value >= MIN_PORT && value <= MAX_PORT;
}

static bool read_address(const char *address_and_port, std::string &address, uint16_t &port) {
    address.clear();

    size_t divider = 0;
    for (size_t i = 0; i < strlen(address_and_port); i++) {
        if (address_and_port[i] == ':') {
            divider = i;
        }
    }
    if (divider == 0) {
        return false;
    }

    for (size_t i = 0; i < divider; i++) {
        address += address_and_port[i];
    }
    std::string port_str;
    for (size_t i = divider + 1; i < strlen(address_and_port); i++) {
        port_str += address_and_port[i];
    }

    if (!check_port(port_str.c_str())) {
        return false;
    }
    port = (uint16_t)atoi(port_str.c_str());

    return true;
}

static void read_parameter(ClientParameters &parameters, const char *option, const char *value) {
    if (strcmp(option, "-d") == 0) {
        if (!read_address(value, parameters.gui_address, parameters.gui_port)) {
            fatal("Incorrect gui address %s.", value);
        }
    }
    else if (strcmp(option, "-n") == 0) {
        parameters.player_name = std::string(value);
    }
    else if (strcmp(option, "-p") == 0) {
        if (!check_port(value)) {
            fatal("Incorrect port %s, available values: 1-65535.", value);
        }
        parameters.port = (uint16_t)atoi(value);
    }
    else if (strcmp(option, "-s") == 0) {
        if (!read_address(value, parameters.server_address, parameters.server_port)) {
            fatal("Incorrect server address %s.", value);
        }
    }
    else {
        fatal("Incorrect parameter %s", option);
    }
}

static void ensure_every_parameter_read(const ClientParameters &parameters) {
    if (parameters.gui_address.empty()) {
        fatal("-d parameter is necessary.");
    }
    if (parameters.player_name.empty()) {
        fatal("-n parameter is necessary.");
    }
    if (parameters.port == 0) {
        fatal("-p parameter is necessary.");
    }
    if (parameters.server_address.empty()) {
        fatal("-s parameter is necessary.");
    }
}

ClientParameters read_parameters(int argc, char *argv[]) {
    if (help_needed(argc, argv)) {
        print_help();
        exit(0);
    }

    if (argc % 2 == 0) {
        fatal("Every option must have value.");
    }

    ClientParameters parameters;
    for (int i = 1; i < argc; i += 2) {
        read_parameter(parameters, argv[i], argv[i + 1]);
    }

    ensure_every_parameter_read(parameters);

    return parameters;
}
