// author - Patryk Jędrzejczak

#include <iostream>
#include <pthread.h>

#include "client-parameters/client_parameters.h"
#include "net/net.h"
#include "err.h"
#include "client-data/client_data.h"
#include "messages/messages.h"

ClientData data;

// Initiates connections client-GUI and client-server using [parameters].
void initiate_connections(const ClientParameters &parameters) {
    data.server_fd = connect(parameters.server_address, parameters.server_port, true);
    if (data.server_fd == -1) {
        fatal("Could not connect to server. Address %s:%d may be incorrect.",
              parameters.server_address.c_str(), parameters.server_port);
    }
    turn_off_nagle(data.server_fd);

    data.gui_rec_ipv4_fd = bind_udp_socket(parameters.port, true);
    data.gui_rec_ipv6_fd = bind_udp_socket(parameters.port, false);
    data.gui_send_fd = connect(parameters.gui_address, parameters.gui_port, false);
    if (data.gui_send_fd == -1) {
        fatal("Could not connect to GUI. Address %s:%d may be incorrect.",
              parameters.gui_address.c_str(), parameters.gui_port);
    }
}

// Function executed by thread that reads from GUI and sends to server.
[[noreturn]] void *from_gui_to_server(void *thread_data) {
    bool ipv4 = *((bool *) thread_data);
    while (true) {
        uint8_t message_type = read_message_from_gui(data, ipv4);
        send_message_to_server(data, message_type);
    }
}

// Function executed by thread that reads from server and sends to GUI.
[[noreturn]] void from_server_to_gui() {
    send_message_to_gui(data);
    while (true) {
        if (read_message_from_server(data)) {
            send_message_to_gui(data);
        }
    }
}

// Function starting new thread that reads from GUI and sends to server.
// If [ipv4] is false, thread reads messages from ipv6 addresses.
void start_from_gui_to_server_thread(bool ipv4) {
    static bool IPV4 = true;
    static bool IPV6 = false;
    void *thread_data = ipv4 ? &IPV4 : &IPV6;
    pthread_t from_gui_to_server_thread;
    CHECK_ERRNO(pthread_create(&from_gui_to_server_thread, nullptr,
                               from_gui_to_server, thread_data));
    CHECK_ERRNO(pthread_detach(from_gui_to_server_thread));
}

int main(int argc, char *argv[]) {
    ClientParameters parameters = read_parameters(argc, argv);
    data.player_name = parameters.player_name;
    data.init();

    initiate_connections(parameters);
    read_hello(data);

    start_from_gui_to_server_thread(true);
    start_from_gui_to_server_thread(false);

    from_server_to_gui();
}