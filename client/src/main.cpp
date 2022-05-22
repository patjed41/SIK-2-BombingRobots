#include <iostream>
#include <pthread.h>

#include "client-parameters/client_parameters.h"
#include "net/net.h"
#include "err.h"
#include "client-data/client_data.h"
#include "messages/messages.h"

ClientData data;

void initiate_connections(const ClientParameters &parameters) {
    std::cout << "gui: " << parameters.gui_address << ", " << parameters.gui_port
              << "\nplayer name: " << parameters.player_name
              << "\nport: " << parameters.port
              << "\nserver: " << parameters.server_address << ", " << parameters.server_port
              << "\n";
    /*if (!is_address_ipv4(parameters.server_address) && !is_address_ipv6(parameters.server_address)) {
        fatal("Incorrect server address %s.", parameters.server_address.c_str());
    }
    data.server_fd = open_socket(is_address_ipv4(parameters.server_address), true);
    connect_socket(data.server_fd, parameters.server_address, parameters.server_port);
    turn_off_nagle(data.server_fd);*/

    data.server_fd = connect_to(parameters.server_address, parameters.server_port, true);
    if (data.server_fd == -1) {
        fatal("Could not connect to server. Address %s:%d may be incorrect.",
              parameters.server_address.c_str(), parameters.server_port);
    }
    turn_off_nagle(data.server_fd);

    /*if (!is_address_ipv4(parameters.gui_address) && !is_address_ipv6(parameters.gui_address)) {
        fatal("Incorrect gui address %s:.", parameters.gui_address.c_str());
    }
    data.gui_rec_fd = open_socket(is_address_ipv4(parameters.gui_address), false);
    bind_socket(data.gui_rec_fd, parameters.port, is_address_ipv4(parameters.gui_address));
    data.gui_send_fd = open_socket(is_address_ipv4(parameters.gui_address), false);
    connect_socket(data.gui_send_fd, parameters.gui_address, parameters.gui_port);*/
    data.gui_rec_fd = bind_udp_socket(parameters.port);
    data.gui_send_fd = connect_to(parameters.gui_address, parameters.gui_port, false);
    if (data.gui_send_fd == -1) {
        fatal("Could not connect to GUI. Address %s:%d may be incorrect.",
              parameters.gui_address.c_str(), parameters.gui_port);
    }

    data.init();
    read_hello(data);
}

[[noreturn]] void *from_gui_to_server([[maybe_unused]] void *thread_data) {
    while (true) {
        uint8_t message_type = read_message_from_gui(data);
        send_message_to_server(data, message_type);
    }
}

[[noreturn]] void from_server_to_gui() {
    while (true) {
        send_message_to_gui(data);
        read_message_from_server(data);
    }
}

int main(int argc, char *argv[]) {
    ClientParameters parameters = read_parameters(argc, argv);
    data.player_name = parameters.player_name;

    initiate_connections(parameters);

    pthread_t from_gui_to_server_thread;
    CHECK_ERRNO(pthread_create(&from_gui_to_server_thread, nullptr,
                               from_gui_to_server, nullptr));
    CHECK_ERRNO(pthread_detach(from_gui_to_server_thread));

    from_server_to_gui();
}