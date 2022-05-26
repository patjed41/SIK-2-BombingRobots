#include <iostream>
#include <pthread.h>

#include "client-parameters/client_parameters.h"
#include "net/net.h"
#include "err.h"
#include "client-data/client_data.h"
#include "messages/messages.h"

ClientData data;

void initiate_connections(const ClientParameters &parameters) {
    data.server_fd = connect(parameters.server_address, parameters.server_port, true);
    if (data.server_fd == -1) {
        fatal("Could not connect to server. Address %s:%d may be incorrect.",
              parameters.server_address.c_str(), parameters.server_port);
    }
    turn_off_nagle(data.server_fd);

    data.gui_rec_fd = bind_udp_socket(parameters.port);
    data.gui_send_fd = connect(parameters.gui_address, parameters.gui_port, false);
    if (data.gui_send_fd == -1) {
        fatal("Could not connect to GUI. Address %s:%d may be incorrect.",
              parameters.gui_address.c_str(), parameters.gui_port);
    }
}

[[noreturn]] void *from_gui_to_server([[maybe_unused]] void *thread_data) {
    while (true) {
        uint8_t message_type = read_message_from_gui(data);
        send_message_to_server(data, message_type);
    }
}

[[noreturn]] void from_server_to_gui() {
    send_message_to_gui(data);
    while (true) {
        if (read_message_from_server(data)) {
            send_message_to_gui(data);
        }
    }
}

int main(int argc, char *argv[]) {
    ClientParameters parameters = read_parameters(argc, argv);
    data.player_name = parameters.player_name;
    data.init();

    initiate_connections(parameters);
    read_hello(data);

    pthread_t from_gui_to_server_thread;
    CHECK_ERRNO(pthread_create(&from_gui_to_server_thread, nullptr,
                               from_gui_to_server, nullptr));
    CHECK_ERRNO(pthread_detach(from_gui_to_server_thread));

    from_server_to_gui();
}