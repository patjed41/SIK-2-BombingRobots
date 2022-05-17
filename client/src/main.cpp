#include <iostream>

#include "client-parameters/client_parameters.h"

int main(int argc, char *argv[]) {
    ClientParameters parameters = read_parameters(argc, argv);

    std::cout << "gui: " << parameters.gui_address << ", " << parameters.gui_port
              << "\nplayer name: " << parameters.player_name
              << "\nport: " << parameters.port
              << "\nserver: " << parameters.server_address << ", " << parameters.server_port
              << "\n";

    sockaddr gui_address = get_address(parameters.gui_address, parameters.gui_port);
    sockaddr server_address = get_address(parameters.server_address, parameters.server_port);


}
