#include "client_parameters.h"
#include "../../common/err.h"

#include <cstring>
#include <iostream>
#include <arpa/inet.h>

#define MAX_PORT 65535
#define MAX_STRING_LEN 256

// Returns true if "-h" parameter appeared.
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

// Checks if port represented by [port_str] is correct.
static bool check_port(const char *port_str) {
    errno = 0;

    char *end_ptr;
    auto value = (uint32_t) strtoul(port_str, &end_ptr, 10);

    return *end_ptr == '\0' && errno == 0 && value <= MAX_PORT;
}

// Reads ip address in (address):(port) format. Returns false if address
// is incorrect. Puts result in [address] and [port] references.
static bool read_address(const std::string &address_and_port, std::string &address, uint16_t &port) {
    size_t divider = address_and_port.find_last_of(':');
    if (divider == std::string::npos) {
        return false;
    }

    size_t address_border = address_and_port[0] == '[' && address_and_port[divider - 1] == ']' ? 1 : 0;
    address = address_and_port.substr(address_border, divider - 2 * address_border);
    if (address.empty()) {
        return false;
    }

    std::string port_str = address_and_port.substr(divider + 1);
    if (!check_port(port_str.c_str())) {
        return false;
    }
    port = (uint16_t) strtol(port_str.c_str(), nullptr, 10);

    return true;
}

// Processes a single parameter [option] with value [value]. Changes
// [parameters] reference.
static void read_parameter(ClientParameters &parameters, const char *option, const char *value) {
    if (strcmp(option, "-d") == 0) {
        if (parameters.gui_address.empty() && !read_address(value, parameters.gui_address, parameters.gui_port)) {
            fatal("Incorrect gui address %s.", value);
        }
    }
    else if (strcmp(option, "-n") == 0) {
        if (!parameters.player_name.empty()) {
            return;
        }
        parameters.player_name = std::string(value);
        if (parameters.player_name.length() > MAX_STRING_LEN) {
            fatal("Name cannot contain more that 256 characters.");
        }
    }
    else if (strcmp(option, "-p") == 0) {
        if (!parameters.read_port && !check_port(value)) {
            fatal("Incorrect port %s, available values: 0-65535.", value);
        }
        parameters.port = (uint16_t) strtol(value, nullptr, 10);
        parameters.read_port = true;
    }
    else if (strcmp(option, "-s") == 0) {
        if (parameters.server_address.empty() && !read_address(std::string(value),
                parameters.server_address, parameters.server_port)) {
            fatal("Incorrect server address %s.", value);
        }
    }
    else {
        fatal("Incorrect parameter %s.", option);
    }
}

// Checks if all necessary parameters appeared.
static void ensure_every_parameter_read(const ClientParameters &parameters) {
    if (parameters.gui_address.empty()) {
        fatal("-d parameter is necessary.");
    }
    if (parameters.player_name.empty()) {
        fatal("-n parameter is necessary.");
    }
    if (!parameters.read_port) {
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
        fatal("Every parameter must have value.");
    }

    ClientParameters parameters;
    for (int i = argc - 1; i > 0; i -= 2) {
        read_parameter(parameters, argv[i - 1], argv[i]);
    }

    ensure_every_parameter_read(parameters);

    return parameters;
}
