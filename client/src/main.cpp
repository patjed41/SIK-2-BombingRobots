#include <iostream>
#include <unistd.h>

#include "client-parameters/client_parameters.h"
#include "net/net.h"
#include "err.h"

int main(int argc, char *argv[]) {
    ClientParameters parameters = read_parameters(argc, argv);

    std::cout << "gui: " << parameters.gui_address << ", " << parameters.gui_port
              << "\nplayer name: " << parameters.player_name
              << "\nport: " << parameters.port
              << "\nserver: " << parameters.server_address << ", " << parameters.server_port
              << "\n";

    if (!is_address_ipv4(parameters.server_address) && !is_address_ipv6(parameters.server_address)) {
        fatal("Incorrect server address %s.", parameters.server_address.c_str());
    }
    int server_fd = open_socket(is_address_ipv4(parameters.server_address), true);
    connect_socket(server_fd, parameters.server_address, parameters.server_port);

    if (!is_address_ipv4(parameters.gui_address) && !is_address_ipv6(parameters.gui_address)) {
        fatal("Incorrect gui address %s.", parameters.gui_address.c_str());
    }
    int gui_rec_fd = open_socket(is_address_ipv4(parameters.gui_address), false);
    bind_socket(gui_rec_fd, parameters.port, is_address_ipv4(parameters.gui_address));
    int gui_send_fd = open_socket(is_address_ipv4(parameters.gui_address), false);
    connect_socket(gui_send_fd, parameters.gui_address, parameters.gui_port);

    char buf[1000];

    ssize_t read_lenght = read(server_fd, buf, 2);
    printf("%d, %ld", (int)buf[0], read_lenght);

    close_socket(server_fd);
    close_socket(gui_rec_fd);
    close_socket(gui_send_fd);
}